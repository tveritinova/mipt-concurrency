#ifndef thread_pool_h
#define thread_pool_h


#include <mutex>
#include <deque>
#include <queue>
#include <condition_variable>
#include <exception>
#include <utility>

//Блокирующая очередь ограниченного размера
template <typename T, class Container = std::deque<T>>
class thread_safe_queue
{
    Container queue;
    std::mutex queue_mtx; // мьютекс, который захватывается потоком, при pop и enqueue
    std::condition_variable not_empty; //условные переменные
    std::condition_variable not_full;
    std::size_t queue_capacity;
    bool isShutdown;
    
public:
    
    explicit thread_safe_queue(std::size_t capacity = 10) : queue_capacity(capacity), isShutdown(false) {}
    thread_safe_queue(thread_safe_queue<T> &) = delete; // запрещённый конструктор копирования
    void enqueue( T && item); // добавляет item в очередь,но если она уже заполнена(она ограничена), поток ждет
    bool pop(T & item); //удаляет элемент из очереди , сохраняя его в item. Если очередь пуста, то поток ждет
    void shutdown();
};

template <typename T, class Container>
inline void thread_safe_queue<T, Container>::enqueue(T && item)
{
    std::unique_lock<std::mutex> lock(queue_mtx);
    if (queue.size() == queue_capacity && !isShutdown)
    {
        not_full.wait(lock, [this]() { return queue.size() != queue_capacity; });
        
    }
    if (isShutdown)
    {
        throw std::exception();
    }
    queue.push_back(std::move(item));
    not_empty.notify_one();
}

template <typename T, class Container>
inline bool thread_safe_queue<T, Container>::pop(T & item)
{
    std::unique_lock<std::mutex> lock(queue_mtx);
    while (queue.empty() && !isShutdown)
    {
        not_empty.wait(lock, [this]() { return !queue.empty(); });
    }
    if (isShutdown)
    {
        return false;
    }
    item = std::move(queue.front());
    queue.pop_front();
    not_full.notify_one();
    return true;
}

template <typename T, class Container>
inline void thread_safe_queue<T, Container>::shutdown()
{
    std::unique_lock<std::mutex> lock(queue_mtx);
    isShutdown = true;
    not_empty.notify_all();		
}



#include <functional>
#include <future>
#include <thread>
#include <utility>
#include <vector>

//#include "thread_safe_queue.h"

template <class Value>
class thread_pool
{
public:
    thread_pool();
    explicit thread_pool(std::size_t num_workers);
    
    std::future<Value> submit(std::function<Value()> f);
    void shutdown();
    
private:
    void start_workers();
    void run_workers();
    
    std::size_t num_workers;
    thread_safe_queue<std::pair<std::function<Value()>, std::promise<Value>>> tasks;
    std::vector<std::thread> workers;
};

int def_num_workers = 10;

template<typename Value>
thread_pool<Value>::thread_pool()
{
    num_workers = std::thread::hardware_concurrency();
    if (num_workers < 1)
    {
        num_workers = def_num_workers;
    }
    start_workers();
}

template<typename Value>
thread_pool<Value>::thread_pool(std::size_t num_workers)
{
    if (num_workers < 1)
    {
        num_workers = std::thread::hardware_concurrency();
        if (num_workers < 1)
        {
            num_workers = def_num_workers;
        }
    }
    start_workers();
}

template<typename Value>
void thread_pool<Value>::start_workers()
{
    for (size_t i = 0; i < num_workers; ++i)
    {
        workers.emplace_back(std::thread([&](){
            run_workers();
        }));
    }
}

template<typename Value>
void thread_pool<Value>::run_workers()
{
    while (true)
    {
        std::pair<std::function<Value()>, std::promise<Value>> task;
        if (!tasks.pop(task))
        {
            break;
        }
        try
        {
            task.second.set_value(task.first());
        }
        catch (std::exception&)
        {
            task.second.set_exception(std::current_exception());
        }
    }
}

template<typename Value>
std::future<Value> thread_pool<Value>::submit(std::function<Value()> f)
{
    std::promise<Value> prom;
    std::future<Value> fut = prom.get_future();
    tasks.enqueue(std::make_pair(f, std::move(prom)));
    return fut;
}

template<typename Value>
void thread_pool<Value>::shutdown()
{
    tasks.shutdown();
    for (std::thread& worker : workers)
    {
        worker.join();
    }
}

#endif /* thread_pool_h */
