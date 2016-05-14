#include <atomic>
#include <mutex>
#include <condition_variable>

class robot_cv
{
public:
    robot_cv() : count(0) {}
    robot_cv(std::size_t _count) : count(_count) {}
    void signal();
    void wait();
    
private:
    std::atomic<std::size_t> count;
    std::mutex mtx;
    std::condition_variable cv;
};