#include <iomanip>
#include <iostream>
#include <future>

#include "thread_pool.hpp"
#include "thread_pool_2.hpp"
#include "thread_pool_bounded_mpmc.hpp"
#include "thread_pool_bounded_mpmc_2.hpp"
#include "thread_pool_bounded_spsc.hpp"
#include "thread_pool_bounded_spsc_2.hpp"
#include "thread_pool_unbounded_mpsc.hpp"
#include "thread_pool_unbounded_spsc.hpp"

const int TASK_COUNT = 10000*1000;

template<class T,int N, typename... Args>
void test_1_to_N(Args &&... args) {
    std::atomic_int a{0};
    // std::vector<int> cpu_cores = {1,5,3,7};
    std::vector<int> cpu_cores;

    T thread_pool(std::forward<Args>(args)...);
    if constexpr (std::is_same<ThreadPoolBoundedSPSC2, T>::value) {
        thread_pool.Init(1, N, cpu_cores);
    } else {
        thread_pool.Init(N, cpu_cores);
    }
    std::atomic<bool> start(false);

    auto push_func = [&] (std::atomic_int& a) {
        while (!start.load(std::memory_order_acquire)) {
            // std::this_thread::yield();
        }
        for (int i = 0; i < TASK_COUNT; ++i) {
            if constexpr (std::is_same<ThreadPoolBoundedSPSC2, T>::value) {
                thread_pool.PushTask([&] () {
                    a.fetch_add(1,std::memory_order_relaxed);
                }, 0);
            } else {
                thread_pool.PushTask([&] () {
                    a.fetch_add(1,std::memory_order_relaxed);
                });
            }
        }
    };

    auto ft0 = std::async(push_func, std::ref(a));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto start_time = std::chrono::system_clock::now();
    start.store(true, std::memory_order_release);


    ft0.get();

    while (!thread_pool.Empty()) {
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::this_thread::yield();
    }
    auto duration = std::chrono::system_clock::now() - start_time;
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));

    thread_pool.Release();

    std::cout << "input count = " << TASK_COUNT<< std::endl;
    std::cout << "output count = " << a<< std::endl;
    std::cout << "speed time = " << duration.count()/(TASK_COUNT) << " ns/per" << std::endl;
    assert(TASK_COUNT== a);
}

template<class T, typename... Args>
void test_4_to_4(Args&& ...args) {
    alignas(64) std::atomic_int a0{0};
    alignas(64) std::atomic_int a1{0};
    alignas(64) std::atomic_int a2{0};
    alignas(64) std::atomic_int a3{0};
    // std::vector<int> cpu_cores = {1,5,3,7};
    std::vector<int> cpu_cores;
    
    T thread_pool(std::forward<Args>(args)...);
    if constexpr (std::is_same<ThreadPoolBoundedSPSC2, T>::value) {
        thread_pool.Init(4, 4, cpu_cores);
    } else {
        thread_pool.Init(4, cpu_cores);
    }
    std::atomic<bool> start(false);

    auto push_func = [&] (std::atomic_int& a, int producer_index) {
        while (!start.load(std::memory_order_acquire)) {
            // std::this_thread::yield();
        }
        for (int i = 0; i < TASK_COUNT; ++i) {
            if constexpr (std::is_same<ThreadPoolBoundedSPSC2, T>::value) {
                thread_pool.PushTask([&] () {
                    a.fetch_add(1, std::memory_order_relaxed);
                }, producer_index);
            } else {
                thread_pool.PushTask([&] () {
                    a.fetch_add(1, std::memory_order_relaxed);
                });
            }
        }
    };

    auto ft0 = std::async(push_func, std::ref(a0), 0);
    auto ft1 = std::async(push_func, std::ref(a1), 1);
    auto ft2 = std::async(push_func, std::ref(a2), 2);
    auto ft3 = std::async(push_func, std::ref(a3), 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto start_time = std::chrono::system_clock::now();
    start.store(true, std::memory_order_release);


    ft0.get();
    ft1.get();
    ft2.get();
    ft3.get();

    while (!thread_pool.Empty()) {
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::this_thread::yield();
    }
    auto duration = std::chrono::system_clock::now() - start_time;
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));

    thread_pool.Release();

    std::cout << "input count = " << TASK_COUNT*4<< std::endl;
    std::cout << "output count = " << a0+a1+a2+a3<< std::endl;
    std::cout << /* std::fixed << std::setprecision(2) <<*/ "speed time = "
        << duration.count()/(a0+a1+a2+a3) << " ns/per" << std::endl;

    assert((TASK_COUNT*4) == (a0+a1+a2+a3));
}

const int BUFFSIZE = 131072;

int main() {
    std::cout <<"---- test_1_to_1 ----" <<std::endl;
    std::cout <<"ThreadPool base" <<std::endl;
    test_1_to_N<ThreadPool, 1>();

    std::cout <<std::endl <<"ThreadPool2" <<std::endl;
    test_1_to_N<ThreadPool2, 1>();

    std::cout <<std::endl <<"ThreadPoolBoundedMPMC" <<std::endl;
    test_1_to_N<ThreadPoolBoundedMPMC, 1>(BUFFSIZE);

    std::cout <<std::endl <<"ThreadPoolBoundedMPMC2" <<std::endl;
    test_1_to_N<ThreadPoolBoundedMPMC2, 1>(BUFFSIZE);

    std::cout <<std::endl <<"ThreadPoolBoundedSPSC" <<std::endl;
    test_1_to_N<ThreadPoolBoundedSPSC, 1>(BUFFSIZE);

    std::cout <<std::endl <<"ThreadPoolUnboundedMPSC" <<std::endl;
    test_1_to_N<ThreadPoolUnboundedMPSC, 1>();

    std::cout <<std::endl <<"ThreadPoolUnboundedSPSC" <<std::endl;
    test_1_to_N<ThreadPoolUnboundedSPSC, 1>();


    std::cout << std::endl;
    std::cout <<"---- test_1_to_4 ----" <<std::endl;
    std::cout <<"ThreadPool base" <<std::endl;
    test_1_to_N<ThreadPool, 4>();

    std::cout <<std::endl <<"ThreadPool2" <<std::endl;
    test_1_to_N<ThreadPool2, 4>();

    std::cout <<std::endl <<"ThreadPoolBoundedMPMC" <<std::endl;
    test_1_to_N<ThreadPoolBoundedMPMC, 4>(BUFFSIZE);

    std::cout <<std::endl <<"ThreadPoolBoundedMPMC2" <<std::endl;
    test_1_to_N<ThreadPoolBoundedMPMC2, 4>(BUFFSIZE);

    std::cout <<std::endl <<"ThreadPoolBoundedSPSC" <<std::endl;
    test_1_to_N<ThreadPoolBoundedSPSC, 4>(BUFFSIZE);

    std::cout <<std::endl <<"ThreadPoolUnboundedMPSC" <<std::endl;
    test_1_to_N<ThreadPoolUnboundedMPSC, 4>();

    std::cout <<std::endl <<"ThreadPoolUnboundedSPSC" <<std::endl;
    test_1_to_N<ThreadPoolUnboundedSPSC, 4>();


    std::cout << std::endl;
    std::cout <<"---- test_4_to_4 ----" <<std::endl;
    std::cout <<"ThreadPool base" <<std::endl;
    test_4_to_4<ThreadPool>();

    std::cout <<std::endl <<"ThreadPool2" <<std::endl;
    test_4_to_4<ThreadPool2>();

    std::cout <<std::endl <<"ThreadPoolBoundedMPMC" <<std::endl;
    test_4_to_4<ThreadPoolBoundedMPMC>(BUFFSIZE);

    std::cout <<std::endl <<"ThreadPoolBoundedMPMC2" <<std::endl;
    test_4_to_4<ThreadPoolBoundedMPMC2>(BUFFSIZE);

    std::cout <<std::endl <<"ThreadPoolUnboundedMPSC" <<std::endl;
    test_4_to_4<ThreadPoolUnboundedMPSC>();

    std::cout <<std::endl <<"ThreadPoolBoundedSPSC2" <<std::endl;
    test_4_to_4<ThreadPoolBoundedSPSC2>(BUFFSIZE);

    return 0;

}










