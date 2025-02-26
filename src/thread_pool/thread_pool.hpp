#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

class ThreadPool {
public:
    ThreadPool() { }
    ~ThreadPool();

    bool Init(int thread_count);
    void Release();

    bool PushTask(std::function<void()>&& task);
    bool Empty();

private:
    void ThreadRun(int thread_index);

    std::deque<std::function<void()>> deque_;
    std::vector<std::thread> threads_;
    bool stop_{false};
    std::condition_variable cond_;
    std::mutex mutex_;
};

ThreadPool::~ThreadPool() {
    Release();
}

bool ThreadPool::Init(int thread_count) {
    stop_ = false;
    threads_.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        threads_.emplace_back(std::bind(&ThreadPool::ThreadRun, this, i));
    }
    return true;
}

void ThreadPool::Release() {
    stop_ = true;
    cond_.notify_all();

    for (auto& t: threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    threads_.clear();
    deque_.clear();
}

void ThreadPool::ThreadRun(int thread_index) {
    std::function<void()> task;
    while(likely(!stop_)) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [&] () {
                return stop_ || !deque_.empty();
            });

            if (unlikely(stop_)) {
                break;
            }

            task = std::move(deque_.front());
            deque_.pop_front();
        }

        if (likely(task)) {
            task();
        }
        task = nullptr;
    }
}

bool ThreadPool::PushTask(std::function<void()>&& task) {
    {
        std::lock_guard<std::mutex> lg(mutex_);
        deque_.emplace_back(std::move(task));
    }
    cond_.notify_one();
    return true;
}

bool ThreadPool::Empty() {
    std::lock_guard<std::mutex> lg(mutex_);
    return deque_.empty();
}
