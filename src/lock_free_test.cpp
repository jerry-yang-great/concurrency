
// test
// g++ unbounded_spsc_queue.cpp -lpthread -g
#include <atomic>
#include <iostream>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <deque>

#include "bounded_spsc_queue.hpp"
#include "bounded_mpmc_queue.hpp"
#include "unbounded_spsc_queue.hpp"
#include "unbounded_mpsc_queue.hpp"

const int DATACOUNT = 10000 * 1000;
const int BUFFSIZE = 131072*128;

template<class T>
class BaseQueue {
public:
    void Enqueue(T data) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(data);
    }

    bool Dequeue(T& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.size() > 0) {
            data = std::move(queue_.front());
            queue_.pop_front();
            return true;
        }
        return false;
    }

private:
    std::deque<T> queue_;
    std::mutex mutex_;
};

template<class T>
void function_spsc_test(T &q) {
    std::atomic_int64_t sum{0};
    std::atomic<bool> start(false);
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;

    std::vector<MPSCQueue<std::function<void()>, false>::Node> unbounded_mpsc_queue_nodes(DATACOUNT);

    auto producer_func = [&]() {
        int index = 0;
        auto increase_func = [&]() {
            sum.fetch_add(1, std::memory_order_relaxed);
        };

        while (!start.load(std::memory_order_acquire)) {
            // std::this_thread::yield();
        }

        start_time = std::chrono::system_clock::now();
        for (int i = 0; i < DATACOUNT; ++i) {
            if constexpr (std::is_same<BaseQueue<std::function<void()>>, T>::value) {
                q.Enqueue(increase_func);
            }
            if constexpr (std::is_same<rigtorp::SPSCQueue<std::function<void()>>, T>::value) {
                q.emplace(increase_func);
            }
            if constexpr (std::is_same<mpmc_bounded_queue<std::function<void()>>, T>::value) {
                while (!q.enqueue(increase_func))
                    ;
            }
            if constexpr (std::is_same<spsc_queue<std::function<void()>>, T>::value) {
                q.enqueue(increase_func);
            }
            if constexpr (std::is_same<MPSCQueue<std::function<void()>, false>, T>::value) {
                auto tmp = &unbounded_mpsc_queue_nodes[i];
                tmp->data = increase_func;
                q.Push(&unbounded_mpsc_queue_nodes[i]);
            }
            if constexpr (std::is_same<MPSCQueue<std::function<void()>, true>, T>::value) {
                q.Push(increase_func);
            }
        }
        start.store(false, std::memory_order_release);
    };

    auto comsumer_func = [&]() {
        while (!start.load(std::memory_order_acquire)) {
            // std::this_thread::yield();
        }

        using TASKType = typename std::conditional<
            std::is_same<rigtorp::SPSCQueue<std::function<void()>>, T>::value,
            std::function<void()>*,
            std::function<void()>>::type;
        
        using TASKType1 = typename std::conditional<
            std::is_same<MPSCQueue<std::function<void()>, false>, T>::value,
            MPSCQueue<std::function<void()>, false>::Node*,
            TASKType>::type;
        TASKType1 task;

        while (start.load(std::memory_order_relaxed) || task) {
            task = nullptr;
            if constexpr (std::is_same<BaseQueue<std::function<void()>>, T>::value) {
                if (q.Dequeue(task)) {
                    task();
                    continue;
                }
            }
            if constexpr (std::is_same<rigtorp::SPSCQueue<std::function<void()>>, T>::value) {
                if (task = q.front()) {
                    (*task)();
                    q.pop();
                    continue;
                }
            }
            else if constexpr (std::is_same<mpmc_bounded_queue<std::function<void()>>, T>::value) {
                if (q.dequeue(task)) {
                    task();
                    continue;
                }
            }
            else if constexpr (std::is_same<spsc_queue<std::function<void()>>, T>::value) {
                if(q.dequeue(task)) {
                    task();
                    continue;
                }
            }
            else if constexpr (std::is_same<MPSCQueue<std::function<void()>, false>, T>::value) {
                if (task = q.Pop()) {
                    task->data();
                    continue;
                }
            }
            else if constexpr (std::is_same<MPSCQueue<std::function<void()>, true>, T>::value) {
                if (q.Pop(task)) {
                    task();
                    continue;
                }
            }
        }
        end_time = std::chrono::system_clock::now();
    };

    std::thread consumer(comsumer_func);
    std::thread producer(producer_func);
    start.store(true, std::memory_order_release);

    if (producer.joinable()) {
        producer.join();
    }

    if (consumer.joinable()) {
        consumer.join();
    }

    auto duration = end_time - start_time;
    std::cout << "input count:\t" << DATACOUNT << std::endl;
    std::cout << "output count:\t" << sum << std::endl;
    std::cout << "cost time: " << duration.count() / DATACOUNT << " ns/per" << std::endl;
    assert(DATACOUNT== sum);
}


template<class T>
void int64_spsc_test(T &q) {
    // alignas(64) std::atomic_int64_t sum(0);
    std::atomic<bool> start(false);
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::vector<MPSCQueue<int64_t, false>::Node> unbounded_mpsc_queue_nodes(DATACOUNT);

    auto producer_func = [&]() {
        while (!start.load(std::memory_order_acquire)) {
            // std::this_thread::yield();
        }

        start_time = std::chrono::system_clock::now();
        for (int i = 0; i < DATACOUNT; ++i) {
            if constexpr (std::is_same<BaseQueue<int64_t>, T>::value) {
                q.Enqueue(i);
            }
            if constexpr (std::is_same<rigtorp::SPSCQueue<int64_t>, T>::value) {
                q.emplace(i);
            }
            if constexpr (std::is_same<mpmc_bounded_queue<int64_t>, T>::value) {
                while (!q.enqueue(i))
                    ;
            }
            if constexpr (std::is_same<spsc_queue<int64_t>, T>::value) {
                q.enqueue(i);
            }
            if constexpr (std::is_same<MPSCQueue<int64_t, false>, T>::value) {
                auto tmp = &unbounded_mpsc_queue_nodes[i];
                tmp->data = i;
                q.Push(tmp);
            }
            if constexpr (std::is_same<MPSCQueue<int64_t, true>, T>::value) {
                q.Push(i);
            }
        }
        start.store(false, std::memory_order_release);
    };

    auto comsumer_func = [&]() {
        while (!start.load(std::memory_order_acquire)) {
            // std::this_thread::yield();
        }

        using TASKType = typename std::conditional<
            std::is_same<rigtorp::SPSCQueue<int64_t>, T>::value,
            int64_t*,
            int64_t>::type;

        using TASKType1 = typename std::conditional<
            std::is_same<MPSCQueue<int64_t, false>, T>::value,
            MPSCQueue<int64_t, false>::Node*,
            int64_t>::type;

        TASKType1 task;
        bool has_task = false;

        while (start.load(std::memory_order_relaxed) || has_task) {
            has_task = false;
            if constexpr (std::is_same<BaseQueue<int64_t>, T>::value) {
                if (q.Dequeue(task)) {
                    has_task = true;
                    continue;
                }
            }
            if constexpr (std::is_same<rigtorp::SPSCQueue<int64_t>, T>::value) {
                if (q.front()) {
                    q.pop();
                    has_task = true;
                    continue;
                }
            }
            else if constexpr (std::is_same<mpmc_bounded_queue<int64_t>, T>::value) {
                if (q.dequeue(task)) {
                    has_task = true;
                    continue;
                }
            }
            else if constexpr (std::is_same<spsc_queue<int64_t>, T>::value) {
                if(q.dequeue(task)) {
                    has_task = true;
                    continue;
                }
            }
            else if constexpr (std::is_same<MPSCQueue<int64_t, false>, T>::value) {
                if (task = q.Pop()) {
                    has_task = true;
                    continue;
                }
            }
            else if constexpr (std::is_same<MPSCQueue<int64_t, true>, T>::value) {
                if (q.Pop(task)) {
                    has_task = true;
                    continue;
                }
            }
        }
        end_time = std::chrono::system_clock::now();
    };

    std::thread consumer(comsumer_func);
    std::thread producer(producer_func);
    start.store(true, std::memory_order_release);

    if (producer.joinable()) {
        producer.join();
    }

    if (consumer.joinable()) {
        consumer.join();
    }

    auto duration = end_time - start_time;
    std::cout << "input count:\t" << DATACOUNT << std::endl;
    std::cout << "cost time: " << duration.count() / DATACOUNT << " ns/per" << std::endl;
}

int main() {
    // std::function test
    std::cout << "----std::function test----" << std::endl;
    BaseQueue<std::function<void()>> base_q;
    std::cout << "TEST_BASE_QUEUE" << std::endl;
    function_spsc_test<BaseQueue<std::function<void()>>>(base_q);
    std::cout << std::endl;
    
    rigtorp::SPSCQueue<std::function<void()>> bounded_spsc_q(BUFFSIZE);
    std::cout << "TEST_BOUNDED_SPSC_QUEUE" << std::endl;
    function_spsc_test<rigtorp::SPSCQueue<std::function<void()>>>(bounded_spsc_q);
    std::cout << std::endl;

    mpmc_bounded_queue<std::function<void()>> bounded_mpmc_q(BUFFSIZE);
    std::cout << "TEST_MPMC_BOUNDED_QUEUE" << std::endl;
    function_spsc_test<mpmc_bounded_queue<std::function<void()>>>(bounded_mpmc_q);
    std::cout << std::endl;

    spsc_queue<std::function<void()>> unbounded_spsc_q;
    std::cout << "TEST_UNBOUNDED_SPSC_QUEUE" << std::endl;
    function_spsc_test<spsc_queue<std::function<void()>>>(unbounded_spsc_q);
    std::cout << std::endl;

    MPSCQueue<std::function<void()>, false> unbounded_mpsc_q_buf;
    std::cout << "TEST_UNBOUNDED_MPSC_QUEUE BUFFER" << std::endl;
    function_spsc_test<MPSCQueue<std::function<void()>, false>>(unbounded_mpsc_q_buf);
    std::cout << std::endl;

    MPSCQueue<std::function<void()>, true> unbounded_mpsc_q;
    std::cout << "TEST_UNBOUNDED_MPSC_QUEUE" << std::endl;
    function_spsc_test<MPSCQueue<std::function<void()>, true>>(unbounded_mpsc_q);
    std::cout << std::endl;


    // int64_t test
    std::cout << "----int64_t test----" << std::endl;
    BaseQueue<int64_t> base_q_int64;
    std::cout << "TEST_BASE_QUEUE" << std::endl;
    int64_spsc_test<BaseQueue<int64_t>>(base_q_int64);
    std::cout << std::endl;

    rigtorp::SPSCQueue<int64_t> bounded_spsc_q_int64(BUFFSIZE);
    std::cout << "TEST_BOUNDED_SPSC_QUEUE" << std::endl;
    int64_spsc_test<rigtorp::SPSCQueue<int64_t>>(bounded_spsc_q_int64);
    std::cout << std::endl;


    mpmc_bounded_queue<int64_t> bounded_mpmc_q_int64(BUFFSIZE);
    std::cout << "TEST_MPMC_BOUNDED_QUEUE" << std::endl;
    int64_spsc_test<mpmc_bounded_queue<int64_t>>(bounded_mpmc_q_int64);
    std::cout << std::endl;

    spsc_queue<int64_t> unbounded_spsc_q_int64;
    std::cout << "TEST_UNBOUNDED_SPSC_QUEUE" << std::endl;
    int64_spsc_test<spsc_queue<int64_t>>(unbounded_spsc_q_int64);
    std::cout << std::endl;

    MPSCQueue<int64_t, false> unbounded_mpsc_q_int64_buf;
    std::cout << "TEST_UNBOUNDED_MPSC_QUEUE BUFFER" << std::endl;
    int64_spsc_test<MPSCQueue<int64_t, false>>(unbounded_mpsc_q_int64_buf);
    std::cout << std::endl;

    MPSCQueue<int64_t, true> unbounded_mpsc_q_int64;
    std::cout << "TEST_UNBOUNDED_MPSC_QUEUE" << std::endl;
    int64_spsc_test<MPSCQueue<int64_t, true>>(unbounded_mpsc_q_int64);
    std::cout << std::endl;
}