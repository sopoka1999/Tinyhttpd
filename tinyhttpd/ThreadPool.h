#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <stdexcept>
#include <future>
#include <iostream>

class ThreadPool {
public:
    ThreadPool(size_t threads);
    std::future<void> enqueue(std::function<void()> task);
    ~ThreadPool();
private:
    //工作线程
    std::vector< std::thread > workers;
    // 任务队列
    std::queue< std::function<void()> > tasks;
    //同步
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

