// https://www.1024cores.net/home/lock-free-algorithms/queues/unbounded-spsc-queue
// Unbounded SPSC Queue

// Unbounded single-producer/single-consumer node-based queue. Internal non-reducible cache of nodes is used. Dequeue operation is always wait-free. Enqueue operation is wait-free in common case (when there is available node in the cache), otherwise enqueue operation calls ::operator new(), so probably not wait-free. No atomic RMW operations nor heavy memory fences are used, i.e. enqueue and dequeue operations issue just several plain loads, several plain stores and one conditional branching. Cache-conscious data layout is used, so producer and consumer can work simultaneously causing no cache-coherence traffic.
// Single-producer/single-consumer queue can be used for communication with thread which services hardware device (wait-free property is required), or when there are naturally only one producer and one consumer. Also N single-producer/single-consumer queues can be used to construct multi-producer/single-consumer queue, or N^2 queues can be used to construct fully-connected system of N threads (other partially-connected topologies are also possible).
// Hardware platform: x86-32/64 
// Compiler: Intel C++ Compiler
#pragma once
#include <cstddef>

#define __memory_barrier() __asm__ __volatile__ ("mfence" : : : "memory")

// load with 'consume' (data-dependent) memory ordering 
template<typename T>
T load_consume(T const* addr) {
    // hardware fence is implicit on x86
    T v = *const_cast<T const volatile*>(addr);
    __memory_barrier(); // compiler fence
    return v;
}

// store with 'release' memory ordering
template<typename T>
void store_release(T* addr, T v) {
    // hardware fence is implicit on x86
    __memory_barrier(); // compiler fence
    *const_cast<T volatile*>(addr) = v;
} 

// cache line size on modern x86 processors (in bytes) 
size_t const cache_line_size = 64; 

// single-producer/single-consumer queue 
template<typename T> 
class spsc_queue { 
public:
  spsc_queue() {
      node* n = new node;
      n->next_ = 0;
      tail_ = head_ = first_= tail_copy_ = n;
  }

  ~spsc_queue() {
      node* n = first_;
      do {
          node* next = n->next_;
          delete n;
          n = next;
      } while (n);
  } 

  void enqueue(T v) {
      node* n = alloc_node();
      n->next_ = 0;
      n->value_ = v;
      store_release(&head_->next_, n);
      head_ = n;
  }

  // returns 'false' if queue is empty 
  bool dequeue(T& v) {
      if (load_consume(&tail_->next_)) {
          v = tail_->next_->value_;
          store_release(&tail_, tail_->next_);
          return true;
      } else {
          return false;
      }
  }

  bool empty() {
      return !load_consume(&tail_->next_);
  }

private:
  // internal node structure 
  struct node { 
      node* next_; 
      T value_; 
  };
  // consumer part 
  // accessed mainly by consumer, infrequently be producer 
  node* tail_; // tail of the queue 
  // delimiter between consumer part and producer part, 
  // so that they situated on different cache lines 
  char cache_line_pad_ [cache_line_size];

  // producer part 
  // accessed only by producer 
  node* head_; // head of the queue 
  node* first_; // last unused node (tail of node cache) 
  node* tail_copy_; // helper (points somewhere between first_ and tail_) 

  node* alloc_node() { 
      // first tries to allocate node from internal node cache, 
      // if attempt fails, allocates node via ::operator new() 
      if (first_ != tail_copy_) {
          node* n = first_;
          first_ = first_->next_;
          return n;
      }

      tail_copy_ = load_consume(&tail_);
      if (first_ != tail_copy_) {
          node* n = first_;
          first_ = first_->next_;
          return n;
      }

      node* n = new node;
      return n;
  }

  spsc_queue(spsc_queue const&);
  spsc_queue& operator = (spsc_queue const&);
}; 
