#ifndef thread_pool_h
#define thread_pool_h


#include <functional>
#include <future>
#include <thread>
#include <utility>
#include <vector>

#include "thread_safe_queue.h"

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
        task.second.set_value(task.first());
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
