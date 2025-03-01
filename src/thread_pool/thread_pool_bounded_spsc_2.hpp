#pragma once
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "bounded_spsc_queue.hpp"
#include "thread_pool_utility.hpp"

class ThreadPoolBoundedSPSC2 {
public:
    ThreadPoolBoundedSPSC2(size_t capacity): capacity_(capacity) { }
    ~ThreadPoolBoundedSPSC2();

    bool Init(int producer_count, int consumer_count, std::vector<int>& cpu_cores);
    bool Init(int producer_count, int consumer_count);
    void Release();

    bool PushTask(std::function<void()>&& task, int producer_idx);
    bool Empty();

private:
    struct alignas(64) Thread {
        std::vector<std::unique_ptr<rigtorp::SPSCQueue<std::function<void()>>>> deque_;
        std::condition_variable cond_;
        std::mutex mutex_;
        std::thread thread_;
        Thread(int producer_count, size_t capacity) {
            for (int i = 0; i < producer_count; ++i) {
                deque_.emplace_back(
                    std::make_unique<rigtorp::SPSCQueue<std::function<void()>>>(capacity));
            }
        }
    };

    void ThreadRun(int thread_index);

    std::vector<std::unique_ptr<Thread>> threads_;
    bool stop_{false};
    std::atomic_int64_t seq_{0};
    size_t capacity_;
    const static uint64_t SPIN_MAX{100000};
};

ThreadPoolBoundedSPSC2::~ThreadPoolBoundedSPSC2() {
    Release();
}

bool ThreadPoolBoundedSPSC2::Init(int producer_count, int consumer_count) {
    std::vector<int> cpu_cores;
    return Init(producer_count, consumer_count, cpu_cores);
}

bool ThreadPoolBoundedSPSC2::Init(int producer_count, int consumer_count, std::vector<int>& cpu_cores) {
    stop_ = false;
    threads_.reserve(consumer_count);
    for (int i = 0; i < consumer_count; ++i) {
        std::unique_ptr<Thread> ptr = std::make_unique<Thread>(producer_count, capacity_);
        threads_.emplace_back(std::move(ptr));
        Thread* thread_ptr = threads_.back().get();
        thread_ptr->thread_ = std::thread(std::bind(&ThreadPoolBoundedSPSC2::ThreadRun, this, i));

        if (i < cpu_cores.size()) {
            ThreadPoolUtility::SetThreadAffinity(thread_ptr->thread_, cpu_cores[i]);
        }
    }
    return true;
}

void ThreadPoolBoundedSPSC2::Release() {
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

void ThreadPoolBoundedSPSC2::ThreadRun(int thread_index) {
    Thread& t = *(threads_[thread_index]);
    auto& deque_ = t.deque_;
    int deque_size = deque_.size();
    std::function<void ()>* task;
    uint64_t spin_counter = 0;
    while(likely(!stop_)) {
        bool empty = true;
        int i = 0;
        for (; likely(i < deque_size); ++i) {
            auto& deq = *deque_[i];
            task = deq.front();
            if (likely(task)) {
                (*task)();
                deq.pop();
                spin_counter = 0;
                empty = false;
                --i;
                continue;
            }
        }

        if (likely(!empty)) {
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
            if (stop_) return stop_;
            auto& deque_ = t.deque_;
            for (int i = 0; i < deque_size; ++i) {
                if (!deque_[i]->empty()) {
                    return true;
                }
            }
            return false;
        });
    }
}

bool ThreadPoolBoundedSPSC2::PushTask(std::function<void()>&& task, int producer_idx) {
    int64_t index = seq_.fetch_add(1);
    Thread& t = *threads_[index%threads_.size()];
    t.deque_[producer_idx]->push(std::move(task));
    t.cond_.notify_one();
    return true;
}

bool ThreadPoolBoundedSPSC2::Empty() {
    for (auto& t: threads_) {
        auto& deque_ = t->deque_;
        int deque_size = deque_.size();
        std::lock_guard<std::mutex> lg(t->mutex_);
        for (int i = 0; i < deque_size; ++i) {
            if (!deque_[i]->empty()) {
                return true;
            }
        }
    }
    return true;
}
