#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threads): stop(false){
    std::cout<<"ThreadPool"<<std::endl;

    for(size_t i = 0; i<threads; ++i){
        workers.emplace_back([this] { // [this] 代表捕获this对象
            while(true) {
                std::function<void()> task;

                {
                    //锁定queue_mutex
                    std::unique_lock<std::mutex>lock(this->queue_mutex);
                    //如果stop 或者 tasks为空，暂时解锁，然后挂起 // 否则继续执行
                    this->condition.wait(lock, [this]{return this->stop || !this->tasks.empty();});

                    if(this->stop && this->tasks.empty())
                        return ;

                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }

                task();
            }
        });
    }
}


std::future<void> ThreadPool::enqueue(std::function<void()> task) {
    auto packaged_task = std::make_shared<std::packaged_task<void()>>(std::move(task));

    std::future<void> res = packaged_task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // 线程池停止后不允许添加新任务
        if (stop) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }

        // 将任务添加到队列
        tasks.emplace([packaged_task]() {
            (*packaged_task)();
        });
    }

    // 通知一个等待中的线程
    condition.notify_one();
    return res;
}


ThreadPool::~ThreadPool(){
    std::cout<<"~ThreadPool"<<std::endl;

    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }

    condition.notify_all();

    for(std::thread & worker: workers)
        worker.join();
}

