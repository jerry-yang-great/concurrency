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

class ThreadPoolBoundedMPMC {
public:
    ThreadPoolBoundedMPMC(size_t buffer_size):deque_(buffer_size) { }
    ~ThreadPoolBoundedMPMC();

    bool Init(int thread_count, std::vector<int>& cpu_cores);
    bool Init(int thread_count);
    void Release();

    bool PushTask(std::function<void()>&& task);
    bool Empty();

private:
    void ThreadRun(int thread_index);

    std::vector<std::thread> threads_;
    mpmc_bounded_queue<std::function<void()>> deque_;
    std::condition_variable cond_;
    std::mutex mutex_;
    bool stop_{false};
    const static uint64_t SPIN_MAX{100000};
};

ThreadPoolBoundedMPMC::~ThreadPoolBoundedMPMC() {
    Release();
}

bool ThreadPoolBoundedMPMC::Init(int thread_count) {
    std::vector<int> cpu_cores;
    return Init(thread_count, cpu_cores);
}

bool ThreadPoolBoundedMPMC::Init(int thread_count, std::vector<int>& cpu_cores) {
    stop_ = false;
    threads_.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        threads_.emplace_back(std::bind(&ThreadPoolBoundedMPMC::ThreadRun, this, i));

        if (i < cpu_cores.size()) {
            ThreadPoolUtility::SetThreadAffinity(threads_.back(), cpu_cores[i]);
        }
    }
    return true;
}

void ThreadPoolBoundedMPMC::Release() {
    stop_ = true;
    cond_.notify_all();

    for (auto& t: threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    threads_.clear();
}

void ThreadPoolBoundedMPMC::ThreadRun(int thread_index) {
    std::function<void ()> task;
    uint64_t spin_counter = 0;
    while(likely(!stop_)) {
        task = nullptr;
        if (likely(deque_.dequeue(task))) {
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
        
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [&] () {
            return stop_ || !deque_.empty();
        });
    }
}

bool ThreadPoolBoundedMPMC::PushTask(std::function<void()>&& task) {
    while(!deque_.enqueue(std::move(task))) {
        //std::this_thread::yield();
    };
    cond_.notify_one();
    return true;
}

bool ThreadPoolBoundedMPMC::Empty() {
    return deque_.empty();
}
