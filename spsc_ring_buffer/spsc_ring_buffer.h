#ifndef spsc_ring_buffer_h
#define spsc_ring_buffer_h

#include <vector>
#include <atomic>

template<typename T>
class spsc_ring_buffer
{
public:
    
    explicit spsc_ring_buffer(size_t size) : capacity(size + 1), buf(capacity) {}
    bool enqueue(T e);
    bool dequeue(T& e);
    
private:
    
    struct node_t
    {
        node_t (T e) : obj(e) {}
        node_t () {}

        T obj;
        char pad[64 - sizeof(T) % 64]; //CACHE_LINE_SIZE
    };
    
    std::atomic<std::size_t> head{0};
    std::atomic<std::size_t> tail{0};
    std::size_t capacity;
    std::vector<node_t> buf;
    
    std::size_t next(std::size_t index);
};

template<typename T>
std::size_t spsc_ring_buffer<T>::next(std::size_t index)
{
    return (index + 1) % capacity;
}

template<typename T>
bool spsc_ring_buffer<T>::enqueue(T e)
{
    std::size_t cur_head = head.load(std::memory_order_acquire);
    std::size_t cur_tail = tail.load(std::memory_order_relaxed);
    
    if (next(cur_tail) == cur_head)
    {
        return false;
    }
    
    buf[cur_tail] = e;
    tail.store(next(cur_tail), std::memory_order_release);
    return true;
}

template<typename T>
bool spsc_ring_buffer<T>::dequeue(T& e)
{
    std::size_t cur_head = head.load(std::memory_order_relaxed);
    std::size_t cur_tail = tail.load(std::memory_order_acquire);
    
    if (cur_tail == cur_head)
    {
        return false;
    }
    
    e = buf[cur_head].obj;
    head.store(next(cur_head), std::memory_order_release);
    return true;
}

#endif /* spsc_ring_buffer_h */
