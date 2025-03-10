#pragma once
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "thread_pool_utility.hpp"

class ThreadPool2 {
public:
    ThreadPool2() { }
    ~ThreadPool2();

    bool Init(int thread_count, std::vector<int>& cpu_cores);
    bool Init(int thread_count);
    void Release();

    bool PushTask(std::function<void()>&& task);
    bool Empty();

private:
    struct alignas(64) Thread {
        std::deque<std::function<void()>> deque_;
        std::thread thread_;
        std::condition_variable cond_;
        std::mutex mutex_;
    };

    void ThreadRun(int thread_index);

    std::vector<std::unique_ptr<Thread>> threads_;
    bool stop_{false};
    std::atomic_int64_t seq_{0};
};

ThreadPool2::~ThreadPool2() {
    Release();
}

bool ThreadPool2::Init(int thread_count) {
    std::vector<int> cpu_cores;
    return Init(thread_count, cpu_cores);
}

bool ThreadPool2::Init(int thread_count, std::vector<int>& cpu_cores) {
    stop_ = false;
    threads_.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        std::unique_ptr<Thread> ptr = std::make_unique<Thread>();
        threads_.emplace_back(std::move(ptr));
        Thread* thread_ptr = threads_.back().get();
        thread_ptr->thread_ = std::thread(std::bind(&ThreadPool2::ThreadRun, this, i));

        if (i < cpu_cores.size()) {
            ThreadPoolUtility::SetThreadAffinity(thread_ptr->thread_, cpu_cores[i]);
        }
    }
    return true;
}

void ThreadPool2::Release() {
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

void ThreadPool2::ThreadRun(int thread_index) {
    Thread& t = *threads_[thread_index];
    std::function<void()> task;
    while(likely(!stop_)) {
        {
            std::unique_lock<std::mutex> lock(t.mutex_);
            t.cond_.wait(lock, [&] () {
                return stop_ || !t.deque_.empty();
            });

            if (unlikely(stop_)) {
                break;
            }

            task = std::move(t.deque_.front());
            t.deque_.pop_front();
        }

        if (likely(task)) {
            task();
        }
        task = nullptr;
    }
}

bool ThreadPool2::PushTask(std::function<void()>&& task) {
    int64_t index = seq_.fetch_add(1);
    Thread& t = *threads_[index%threads_.size()];
    {
        std::lock_guard<std::mutex> lg(t.mutex_);
        t.deque_.emplace_back(std::move(task));
    }
    t.cond_.notify_one();
    return true;
}

bool ThreadPool2::Empty() {
    bool has_data = false;
    for (auto& t: threads_) {
        std::lock_guard<std::mutex> lg(t->mutex_);
        has_data = has_data || !t->deque_.empty();
    }
    return !has_data;
}
