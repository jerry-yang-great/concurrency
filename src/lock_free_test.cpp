
// test
// g++ unbounded_spsc_queue.cpp -lpthread -g
#include <atomic>
#include <iostream>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>

// #define TEST_SPSC_QUEUE 1
// #define TEST_MPMC_BOUNDED_QUEUE 1
// #define TEST_UNBOUNDED_SPSC_QUEUE 1

#ifdef TEST_SPSC_QUEUE
#include "spsc_queue.hpp"
#elif TEST_MPMC_BOUNDED_QUEUE
#include "bounded_mpmc_queue.hpp"
#elif TEST_UNBOUNDED_SPSC_QUEUE
#include "unbounded_spsc_queue.hpp"
#else
#include "mpsc_queue.hpp"
#endif

int main() {
    const int DATACOUNT = 10000 * 10000;
#ifdef TEST_SPSC_QUEUE
    rigtorp::SPSCQueue<std::function<void()>> q(102400);
    std::cout << "TEST_BOUNDED_SPSC_QUEUE" << std::endl;
#elif TEST_MPMC_BOUNDED_QUEUE
    mpmc_bounded_queue<std::function<void()>> q(131072*128);
    std::cout << "TEST_MPMC_BOUNDED_QUEUE" << std::endl;
#elif TEST_UNBOUNDED_SPSC_QUEUE
    spsc_queue<std::function<void()>> q;
    std::cout << "TEST_UNBOUNDED_SPSC_QUEUE" << std::endl;
#else
    MPSCQueue<std::function<void()>> q;
    std::cout << "TEST_UNBOUNDED_MPSC_QUEUE" << std::endl;
#endif

    alignas(64) std::atomic_int64_t sum(0);
    std::atomic<bool> start(false);
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    // std::mutex

    auto producer_func = [&]() {
        auto increase_func = [&]() {
            sum.fetch_add(1, std::memory_order_relaxed);
        };

        while (!start.load(std::memory_order_acquire)) {
            // std::this_thread::yield();
        }

        start_time = std::chrono::system_clock::now();
        for (int i = 0; i < DATACOUNT; ++i) {
#ifdef TEST_SPSC_QUEUE
            q.emplace(increase_func);
            //
#elif TEST_MPMC_BOUNDED_QUEUE
            while (!q.enqueue(increase_func))
                ;
#elif TEST_UNBOUNDED_SPSC_QUEUE
            q.enqueue(increase_func);
#else
            auto tmp = increase_func;
            q.Push(tmp);
#endif
        }
        start.store(false, std::memory_order_release);
    };

    auto comsumer_func = [&]() {
        while (!start.load(std::memory_order_acquire))
        {
            // std::this_thread::yield();
        }

#ifdef TEST_SPSC_QUEUE
        std::function<void()>* task;
#else
        std::function<void()> task;
#endif
        while (start.load(std::memory_order_relaxed) || task)
        {
            task = nullptr;
#ifdef TEST_SPSC_QUEUE
            if (task = q.front()) {
                (*task)();
                q.pop();
                continue;
            }
#elif TEST_MPMC_BOUNDED_QUEUE
            if (q.dequeue(task)) {
                task();
                continue;
            }
#elif TEST_UNBOUNDED_SPSC_QUEUE
            if(q.dequeue(task)) {
                task();
                continue;
            }
#else
            if (q.Pop(task)) {
                task();
                continue;
            }
#endif
            
        }
        end_time = std::chrono::system_clock::now();
    };

    std::thread consumer(comsumer_func);
    std::thread producer(producer_func);
    start.store(true, std::memory_order_release);

    if (producer.joinable())
    {
        producer.join();
    }

    if (consumer.joinable())
    {
        consumer.join();
    }

    auto duration = end_time - start_time;
    std::cout << "input count:\t" << DATACOUNT << std::endl;
    std::cout << "output count:\t" << sum << std::endl;
    std::cout << "cost time: " << duration.count() / DATACOUNT << " ns/per" << std::endl;
}