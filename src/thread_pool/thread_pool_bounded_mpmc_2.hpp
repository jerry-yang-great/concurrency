#pragma once
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "bounded_mpmc_queue.hpp"
#include "thread_pool_utility.hpp"

class ThreadPoolBoundedMPMC2 {
public:
    ThreadPoolBoundedMPMC2(size_t buffer_size):buffer_size_(buffer_size) { }
    ~ThreadPoolBoundedMPMC2();

    bool Init(int thread_count, std::vector<int>& cpu_cores);
    bool Init(int thread_count);
    void Release();

    bool PushTask(std::function<void()>&& task);
    bool Empty();

private:
    struct alignas(64) Thread {
        mpmc_bounded_queue<std::function<void()>> deque_;
        std::condition_variable cond_;
        std::mutex mutex_;
        std::thread thread_;
        Thread(size_t buffer_size): deque_(buffer_size) {
        }
    };

    void ThreadRun(int thread_index);


    std::vector<std::unique_ptr<Thread>> threads_;
    bool stop_{false};
    std::atomic_int64_t seq_{0};
    size_t buffer_size_;
    const static uint64_t SPIN_MAX{100000};
};

ThreadPoolBoundedMPMC2::~ThreadPoolBoundedMPMC2() {
    Release();
}

bool ThreadPoolBoundedMPMC2::Init(int thread_count) {
    std::vector<int> cpu_cores;
    return Init(thread_count, cpu_cores);
}

bool ThreadPoolBoundedMPMC2::Init(int thread_count, std::vector<int>& cpu_cores) {
    stop_ = false;
    threads_.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        std::unique_ptr<Thread> ptr = std::make_unique<Thread>(buffer_size_);
        threads_.emplace_back(std::move(ptr));
        Thread* thread_ptr = threads_.back().get();
        thread_ptr->thread_ = std::thread(std::bind(&ThreadPoolBoundedMPMC2::ThreadRun, this, i));

        if (i < cpu_cores.size()) {
            ThreadPoolUtility::SetThreadAffinity(thread_ptr->thread_, cpu_cores[i]);
        }
    }

    return true;
}

void ThreadPoolBoundedMPMC2::Release() {
    stop_ = true;
    for (auto& t: threads_) {
        t->cond_.notify_all();
    }

    for (auto& t: threads_) {
        if (t->thread_.joinable()) {
            t->thread_.join();
        }
    }
    threads_.clear();
}

void ThreadPoolBoundedMPMC2::ThreadRun(int thread_index) {
    Thread& t = *(threads_[thread_index]);
    std::function<void ()> task;
    uint64_t spin_counter = 0;
    while(likely(!stop_)) {
        task = nullptr;
        if (likely(t.deque_.dequeue(task))) {
            task();
            spin_counter = 0;
            continue;
        }

        ++ spin_counter;
        if (likely(spin_counter < SPIN_MAX)) {
            std::this_thread::yield();
            continue;
        }
        spin_counter = 0;
        
        std::unique_lock<std::mutex> lock(t.mutex_);
        t.cond_.wait(lock, [&] () {
            return stop_ || !t.deque_.empty();
        });
    }
}

bool ThreadPoolBoundedMPMC2::PushTask(std::function<void()>&& task) {
    int64_t index = seq_.fetch_add(1);
    Thread& t = *threads_[index%threads_.size()];
    while(!t.deque_.enqueue(std::move(task)));
    t.cond_.notify_one();
    return true;
}

bool ThreadPoolBoundedMPMC2::Empty() {
    bool has_data = false;
    for (auto& t: threads_) {
        std::lock_guard<std::mutex> lg(t->mutex_);
        has_data = has_data || !t->deque_.empty();
    }
    return !has_data;
}
