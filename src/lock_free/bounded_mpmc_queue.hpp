// https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
// Bounded MPMC queue

// According to the classification it's MPMC, array-based, fails on overflow, does not require GC, 
// w/o priorities, causal FIFO, blocking producers and consumers queue. The algorithm is pretty simple and fast. 
// It's not lockfree in the official meaning, just implemented by means of atomic RMW operations w/o mutexes. 
// The cost of enqueue/dequeue is 1 CAS per operation. No amortization, just 1 CAS. 
// No dynamic memory allocation/management during operation. Producers and consumers are separated from each other 
// (as in the two-lock queue), i.e. do not touch the same data while queue is not empty.
// On my dual-core laptop enqueue/dequeue takes 75 cycles on average in a synthetic multi-threaded benchmark.
// Source code test suite are attached below (the file contains limited implementation of  std::atomic, ready to run on Windows, MSVC, x86-32) .
#include <cassert>

template<typename T>
class mpmc_bounded_queue {
public:
  mpmc_bounded_queue(size_t buffer_size)
    : buffer_(new cell_t [buffer_size])
    , buffer_mask_(buffer_size - 1) {
    assert((buffer_size >= 2) &&
      ((buffer_size & (buffer_size - 1)) == 0));

    for (size_t i = 0; i != buffer_size; i += 1)
      buffer_[i].sequence_.store(i, std::memory_order_relaxed);
    enqueue_pos_.store(0, std::memory_order_relaxed);
    dequeue_pos_.store(0, std::memory_order_relaxed);
  }

  ~mpmc_bounded_queue() {
    delete [] buffer_;
  }

  bool enqueue(T const& data) {
    cell_t* cell;
    size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
    for (;;) {// 队列满了，这里会一直循环？
      cell = &buffer_[pos & buffer_mask_];
      size_t seq = 
        cell->sequence_.load(std::memory_order_acquire);

      intptr_t dif = (intptr_t)seq - (intptr_t)pos;
      if (dif == 0) { // cell为空
        if (enqueue_pos_.compare_exchange_weak
            (pos, pos + 1, std::memory_order_relaxed))
          break;
      } else if (dif < 0) // 未知异常情况?
        return false;
      else // 已经被用了
        pos = enqueue_pos_.load(std::memory_order_relaxed);
    }

    cell->data_ = data;
    cell->sequence_.store(pos + 1, std::memory_order_release);

    return true;

  }

  bool dequeue(T& data) {
    cell_t* cell;
    size_t pos = dequeue_pos_.load(std::memory_order_relaxed);

    for (;;) {
      cell = &buffer_[pos & buffer_mask_];
      size_t seq = 
        cell->sequence_.load(std::memory_order_acquire);

      intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
      if (dif == 0) { // 有数据
        if (dequeue_pos_.compare_exchange_weak
            (pos, pos + 1, std::memory_order_relaxed))
          break;
      } else if (dif < 0) // 未知异常情况?
        return false;
      else // 数据已经被别人读走了
        pos = dequeue_pos_.load(std::memory_order_relaxed);
    }

    data = cell->data_;
    cell->sequence_.store
      (pos + buffer_mask_ + 1, std::memory_order_release);

    return true;
  }

private:

  struct cell_t {
    std::atomic<size_t>   sequence_;
    T                     data_;
  };

  static size_t const     cacheline_size = 64;
  typedef char            cacheline_pad_t [cacheline_size];

  cacheline_pad_t         pad0_;
  cell_t* const           buffer_;
  size_t const            buffer_mask_;

  cacheline_pad_t         pad1_;
  std::atomic<size_t>     enqueue_pos_;

  cacheline_pad_t         pad2_;
  std::atomic<size_t>     dequeue_pos_;

  cacheline_pad_t         pad3_;

  mpmc_bounded_queue(mpmc_bounded_queue const&);
  void operator = (mpmc_bounded_queue const&);

}; 