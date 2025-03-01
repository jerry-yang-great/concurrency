#pragma once
#include <thread>

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

class ThreadPoolUtility {
public:
    static inline void SetThreadAffinity(std::thread& thread, int cpu_core) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);          // 清空 CPU 集合
        CPU_SET(cpu_core, &cpuset);  // 将指定的 CPU 核心添加到集合中

        // 获取 std::thread 的本地句柄
        pthread_t nativeHandle = thread.native_handle();

        // 设置线程的 CPU 亲和性
        int result = pthread_setaffinity_np(nativeHandle, sizeof(cpu_set_t), &cpuset);
    }
};

