# concurrency

cpu: 11th Gen Intel(R) Core(TM) i7-11800H @ 2.30GHz


concurrency/src$ ./lock_free_test 
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

concurrency/src$ ./thread_pool_test 
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

-------------------------------
cpu: AMD Ryzen 7 5825U with Radeon Graphics


$ ./lock_free_test 
----std::function test----
TEST_BASE_QUEUE
input count:	10000000
output count:	10000000
cost time: 181 ns/per

TEST_BOUNDED_SPSC_QUEUE
input count:	10000000
output count:	10000000
cost time: 99 ns/per

TEST_MPMC_BOUNDED_QUEUE
input count:	10000000
output count:	10000000
cost time: 16 ns/per

TEST_UNBOUNDED_SPSC_QUEUE
input count:	10000000
output count:	10000000
cost time: 51 ns/per

TEST_UNBOUNDED_MPSC_QUEUE
input count:	10000000
output count:	10000000
cost time: 49 ns/per

----int64_t test----
TEST_BASE_QUEUE
input count:	10000000
cost time: 86 ns/per

TEST_BOUNDED_SPSC_QUEUE
input count:	10000000
cost time: 29 ns/per

TEST_MPMC_BOUNDED_QUEUE
input count:	10000000
cost time: 3 ns/per

TEST_UNBOUNDED_SPSC_QUEUE
input count:	10000000
cost time: 42 ns/per

TEST_UNBOUNDED_MPSC_QUEUE
input count:	10000000
cost time: 19 ns/per

great@great:~/code/concurrency/src$ ./thread_pool_test 
---- test_1_to_4 ----
ThreadPool base
input count = 10000000
output count = 10000000
speed time = 625 ns/per

ThreadPool2
input count = 10000000
output count = 10000000
speed time = 333 ns/per

ThreadPoolBoundedMPMC
input count = 10000000
output count = 10000000
speed time = 78 ns/per

ThreadPoolBoundedSPSC
input count = 10000000
output count = 10000000
speed time = 37 ns/per

ThreadPoolUnboundedMPSC
input count = 10000000
output count = 10000000
speed time = 109 ns/per

ThreadPoolUnboundedSPSC
input count = 10000000
output count = 10000000
speed time = 74 ns/per

---- test_4_to_4 ----
ThreadPool base
input count = 40000000
output count = 40000000
speed time = 223 ns/per

ThreadPool2
input count = 40000000
output count = 40000000
speed time = 163 ns/per

ThreadPoolBoundedMPMC
input count = 40000000
output count = 40000000
speed time = 85 ns/per

ThreadPoolUnboundedMPSC
input count = 40000000
output count = 40000000
speed time = 52 ns/per
