#include <future>
#include <iostream>
#include "ThreadPool.h"

int myTask(int n) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Processing task " << n << std::endl;
    return 1;
}


int main() {
    ThreadPool pool(3);
    // 使用函数
    int res = 0;
    auto task1 = pool.enqueue([&](){res = myTask(1);});
    auto task2 = pool.enqueue([&](){res = myTask(3);});
    task1.get();
    task2.get();
    // // 使用 lambda 表达式
    // std::future<int> result2 = t.enqueue([](int y) { return y + y; }, 10);
    // t.enqueue(compute);

    return 0;
}
