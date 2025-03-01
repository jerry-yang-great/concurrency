#pragma once
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "unbounded_mpsc_queue.hpp"
#include "thread_pool_utility.hpp"

class ThreadPoolUnboundedMPSC {
public:
    ThreadPoolUnboundedMPSC() { }
    ~ThreadPoolUnboundedMPSC();

    bool Init(int thread_count, std::vector<int>& cpu_cores);
    bool Init(int thread_count);
    void Release();

    bool PushTask(std::function<void()>&& task);
    bool Empty();

private:
    struct alignas(64) Thread {
        MPSCQueue<std::function<void()>, true> deque_;
        std::condition_variable cond_;
        std::mutex mutex_;
        std::thread thread_;
    };

    void ThreadRun(int thread_index);

    void SetThreadAffinity(std::thread& thread, int cpu_core) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);          // 清空 CPU 集合
        CPU_SET(cpu_core, &cpuset);  // 将指定的 CPU 核心添加到集合中

        // 获取 std::thread 的本地句柄
        pthread_t nativeHandle = thread.native_handle();

        // 设置线程的 CPU 亲和性
        int result = pthread_setaffinity_np(nativeHandle, sizeof(cpu_set_t), &cpuset);
    }

    std::vector<std::unique_ptr<Thread>> threads_;
    bool stop_{false};
    std::atomic_int64_t seq_{0};
    const static uint64_t SPIN_MAX{100000};
};

ThreadPoolUnboundedMPSC::~ThreadPoolUnboundedMPSC() {
    Release();
}

bool ThreadPoolUnboundedMPSC::Init(int thread_count) {
    std::vector<int> cpu_cores;
    return Init(thread_count, cpu_cores);
}

bool ThreadPoolUnboundedMPSC::Init(int thread_count, std::vector<int>& cpu_cores) {
    stop_ = false;
    threads_.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        std::unique_ptr<Thread> ptr = std::make_unique<Thread>();
        threads_.emplace_back(std::move(ptr));
        Thread* thread_ptr = threads_.back().get();
        thread_ptr->thread_ = std::thread(std::bind(&ThreadPoolUnboundedMPSC::ThreadRun, this, i));

        if (i < cpu_cores.size()) {
            ThreadPoolUtility::SetThreadAffinity(thread_ptr->thread_, cpu_cores[i]);
        }
    }
    return true;
}

void ThreadPoolUnboundedMPSC::Release() {
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

void ThreadPoolUnboundedMPSC::ThreadRun(int thread_index) {
    Thread& t = *(threads_[thread_index]);
    std::function<void ()> task;
    uint64_t spin_counter = 0;
    while(likely(!stop_)) {
        task = nullptr;
        t.deque_.Pop(task);
        if (likely(task)) {
            (task)();
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
            return stop_ || !t.deque_.Empty();
        });
    }
}

bool ThreadPoolUnboundedMPSC::PushTask(std::function<void()>&& task) {
    int64_t index = seq_.fetch_add(1);
    Thread& t = *threads_[index%threads_.size()];
    t.deque_.Push(std::move(task));
    t.cond_.notify_one();
    return true;
}

bool ThreadPoolUnboundedMPSC::Empty() {
    bool has_data = false;
    for (auto& t: threads_) {
        std::lock_guard<std::mutex> lg(t->mutex_);
        has_data = has_data || !t->deque_.Empty();
    }
    return !has_data;
}
