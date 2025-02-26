#include <iomanip>
#include <iostream>
#include <future>

#include "thread_pool_2.hpp"

const int TASK_COUNT = 10000*1000;

int main() {
    std::atomic_int a0{0};
    ThreadPool thread_pool;
    thread_pool.Init(4);
    std::atomic_int a1{0};

    auto start_time = std::chrono::system_clock::now();
    auto ft0 = std::async([&] () {
        for (int i = 0; i < TASK_COUNT; ++i) {
            thread_pool.PushTask([&] () {
                a0.fetch_add(1);
            });
        }
    });
    auto ft1 = std::async([&] () {
        for (int i = 0; i < TASK_COUNT; ++i) {
            thread_pool.PushTask([&] () {
                a1.fetch_add(1);
            });
        }
    });

    ft0.get();
    ft1.get();

    while (!thread_pool.Empty()) {
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::this_thread::yield();
    }
    auto duration = std::chrono::system_clock::now() - start_time;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    thread_pool.Release();

    std::cout << "input count = " << TASK_COUNT*2<< std::endl;
    std::cout << "output count = " << a0+a1<< std::endl;
    std::cout << std::fixed << std::setprecision(2) << "speed time = " 
        << duration.count()/(1.0*1e6) << "ms" << std::endl;

    return 0;
}










