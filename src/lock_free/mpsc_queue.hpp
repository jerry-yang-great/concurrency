// https://www.1024cores.net/home/lock-free-algorithms/queues/intrusive-mpsc-node-based-queue
// Intrusive MPSC node-based queue

// Advantages:
// + Intrusive. No need for additional internal nodes.
// + Wait-free and fast producers. One XCHG is maximum what one can get with multi-producer non-distributed queue.
// + Extremely fast consumer. On fast-path it's atomic-free, XCHG executed per node batch, in order to grab 'last item'.
// + No need for node order reversion. So pop operation is always O(1).
// + ABA-free.
// + No need for PDR. That is, one can use this algorithm out-of-the-box. No need for thread registration/deregistration, periodic activity, deferred garbage etc.

// Disadvantages:
// - Push function is blocking wrt consumer. I.e. if producer blocked in (*), then consumer is blocked too. Fortunately 'window of inconsistency' is extremely small - producer must be blocked exactly in (*). Actually it's disadvantage only as compared with totally lockfree algorithm. It's still much better lockbased algorithm.
#include <atomic>

template <typename T>
class MPSCQueue { // mpscq_t
public:
    // template <typename T>
    struct Node {
        std::atomic<Node*> next{nullptr}; // volatile
        T data;
        Node(T&& d): next(nullptr), data(d) { }
        Node() { }
    };

    MPSCQueue(): head(&stub), tail(&stub) { }

    ~MPSCQueue() {
        T data;
        while(Pop(data)) { }
    }

    void Push(T&& data) {
        Node* new_node = new Node(std::move(data));
        PushInternel(new_node);
    }

    bool Pop(T& data) {
        Node* old_tail = tail;
        Node* next = old_tail->next.load(std::memory_order_relaxed);
        if (old_tail == &stub) {
            if (next == nullptr) {
                return false;
            }

            tail = next;
            old_tail = tail;
            next = next->next.load(std::memory_order_relaxed);
        }

        if (next) {
            tail = next;
            data = std::move(old_tail->data);
            delete old_tail;
            return true;
        }

        Node* old_head = head.load(std::memory_order_relaxed);
        if (old_tail != old_head) {
            return false; // Push 执行了exchange，还没来得及执行prev->next.store，可以认为是空队列
        }

        // 队列只有一个元素，且stub不在队列中，为什么非得把stub放进去，后面逻辑可以不要吗？便于内存回收stub？
        PushInternel(&stub);
        next = old_head->next.load(std::memory_order_relaxed);
        if (next) {
            tail = next;
            data = std::move(old_tail->data);
            delete old_tail;
            return true;
        }
        return false;
    }

    bool Empty() {
        return tail->next.load(std::memory_order_relaxed) == nullptr;

        // Node* old_tail = tail;
        // Node* next = old_tail->next.load(std::memory_order_relaxed);
        // if (old_tail == &stub) {
        //     return next == nullptr;
        // }
        // if(head.load(std::memory_order_relaxed) == old_tail) {
        //     return true;
        // }
    }

private:
    inline void PushInternel(Node* node) {
        node->next.store(nullptr, std::memory_order_relaxed);
        Node* prev = head.exchange(node, std::memory_order_relaxed); // todo std::memory_order
        prev->next.store(node, std::memory_order_relaxed);
    }

    std::atomic<Node*> head; // volatile
    Node* tail;
    Node stub;

};

// Node* mpscq_pop(mpscq_t* self) {
//     Node* tail = self->tail;
//     Node* next = tail->next;
//     if (tail == &self->stub) {
//         if (0 == next)
//             return 0;
//         self->tail = next;
//         tail = next;
//         next = next->next;
//     }

//     if (next) {
//         self->tail = next;
//         return tail;
//     }

//     Node* head = self->head;
//     if (tail != head)
//         return 0;

//     mpscq_push(self, &self->stub);
//     next = tail->next;
//     if (next)
//     {
//         self->tail = next;
//         return tail;
//     }
//     return 0;

// }