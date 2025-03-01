# concurrency

lock-free data structure and thread pools

performance test result:
## 1 11th Gen Intel(R) Core(TM) i7-11800H @ 2.30GHz

### 1.1 lock_free_test
```
----std::function test----
TEST_BASE_QUEUE
input count:    10000000
output count:   10000000
cost time: 152 ns/per

TEST_BOUNDED_SPSC_QUEUE
input count:    10000000
output count:   10000000
cost time: 15 ns/per

TEST_MPMC_BOUNDED_QUEUE
input count:    10000000
output count:   10000000
cost time: 17 ns/per

TEST_UNBOUNDED_SPSC_QUEUE
input count:    10000000
output count:   10000000
cost time: 43 ns/per

TEST_UNBOUNDED_MPSC_QUEUE
input count:    10000000
output count:   10000000
cost time: 77 ns/per

----int64_t test----
TEST_BASE_QUEUE
input count:    10000000
cost time: 162 ns/per

TEST_BOUNDED_SPSC_QUEUE
input count:    10000000
cost time: 3 ns/per

TEST_MPMC_BOUNDED_QUEUE
input count:    10000000
cost time: 10 ns/per

TEST_UNBOUNDED_SPSC_QUEUE
input count:    10000000
cost time: 25 ns/per

TEST_UNBOUNDED_MPSC_QUEUE
input count:    10000000
cost time: 59 ns/per
```

### 1.2 thread_pool_test 
```
---- test_1_to_4 ----
ThreadPool base
input count = 10000000
output count = 10000000
speed time = 938 ns/per

ThreadPool2
input count = 10000000
output count = 10000000
speed time = 618 ns/per

ThreadPoolBoundedMPMC
input count = 10000000
output count = 10000000
speed time = 133 ns/per

ThreadPoolBoundedSPSC
input count = 10000000
output count = 10000000
speed time = 101 ns/per

ThreadPoolUnboundedMPSC
input count = 10000000
output count = 10000000
speed time = 155 ns/per

ThreadPoolUnboundedSPSC
input count = 10000000
output count = 10000000
speed time = 114 ns/per

---- test_4_to_4 ----
ThreadPool base
input count = 40000000
output count = 40000000
speed time = 572 ns/per

ThreadPool2
input count = 40000000
output count = 40000000
speed time = 203 ns/per

ThreadPoolBoundedMPMC
input count = 40000000
output count = 40000000
speed time = 226 ns/per

ThreadPoolUnboundedMPSC
input count = 40000000
output count = 40000000
speed time = 109 ns/per
```
-------------------------------
## 2 AMD Ryzen 7 5825U with Radeon Graphics

### 2.1 lock_free_test
```
----std::function test----
TEST_BASE_QUEUE
input count:	10000000
output count:	10000000
cost time: 241 ns/per

TEST_BOUNDED_SPSC_QUEUE
input count:	10000000
output count:	10000000
cost time: 63 ns/per

TEST_MPMC_BOUNDED_QUEUE
input count:	10000000
output count:	10000000
cost time: 16 ns/per

TEST_UNBOUNDED_SPSC_QUEUE
input count:	10000000
output count:	10000000
cost time: 49 ns/per

TEST_UNBOUNDED_MPSC_QUEUE BUFFER
input count:	10000000
output count:	10000000
cost time: 39 ns/per

TEST_UNBOUNDED_MPSC_QUEUE
input count:	10000000
output count:	10000000
cost time: 44 ns/per

----int64_t test----
TEST_BASE_QUEUE
input count:	10000000
cost time: 116 ns/per

TEST_BOUNDED_SPSC_QUEUE
input count:	10000000
cost time: 27 ns/per

TEST_MPMC_BOUNDED_QUEUE
input count:	10000000
cost time: 9 ns/per

TEST_UNBOUNDED_SPSC_QUEUE
input count:	10000000
cost time: 38 ns/per

TEST_UNBOUNDED_MPSC_QUEUE BUFFER
input count:	10000000
cost time: 15 ns/per

TEST_UNBOUNDED_MPSC_QUEUE
input count:	10000000
cost time: 43 ns/per
```

### 2.2 thread_pool_test 
```
---- test_1_to_1 ----
ThreadPool base
input count = 10000000
output count = 10000000
speed time = 225 ns/per

ThreadPool2
input count = 10000000
output count = 10000000
speed time = 168 ns/per

ThreadPoolBoundedMPMC
input count = 10000000
output count = 10000000
speed time = 17 ns/per

ThreadPoolBoundedMPMC2
input count = 10000000
output count = 10000000
speed time = 27 ns/per

ThreadPoolBoundedSPSC
input count = 10000000
output count = 10000000
speed time = 19 ns/per

ThreadPoolUnboundedMPSC
input count = 10000000
output count = 10000000
speed time = 53 ns/per

ThreadPoolUnboundedSPSC
input count = 10000000
output count = 10000000
speed time = 43 ns/per

---- test_1_to_4 ----
ThreadPool base
input count = 10000000
output count = 10000000
speed time = 660 ns/per

ThreadPool2
input count = 10000000
output count = 10000000
speed time = 318 ns/per

ThreadPoolBoundedMPMC
input count = 10000000
output count = 10000000
speed time = 68 ns/per

ThreadPoolBoundedMPMC2
input count = 10000000
output count = 10000000
speed time = 47 ns/per

ThreadPoolBoundedSPSC
input count = 10000000
output count = 10000000
speed time = 28 ns/per

ThreadPoolUnboundedMPSC
input count = 10000000
output count = 10000000
speed time = 92 ns/per

ThreadPoolUnboundedSPSC
input count = 10000000
output count = 10000000
speed time = 82 ns/per

---- test_4_to_4 ----
ThreadPool base
input count = 40000000
output count = 40000000
speed time = 182 ns/per

ThreadPool2
input count = 40000000
output count = 40000000
speed time = 143 ns/per

ThreadPoolBoundedMPMC
input count = 40000000
output count = 40000000
speed time = 76 ns/per

ThreadPoolBoundedMPMC2
input count = 40000000
output count = 40000000
speed time = 39 ns/per

ThreadPoolUnboundedMPSC
input count = 40000000
output count = 40000000
speed time = 50 ns/per

ThreadPoolBoundedSPSC2
input count = 40000000
output count = 40000000
speed time = 23 ns/per
```