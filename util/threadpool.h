//
// Created by xiaohu on 3/21/22.
//

#ifndef PANDA_THREADPOOL_H
#define PANDA_THREADPOOL_H
#include <thread>
#include <vector>
#include <queue.h>
#include <atomic>
#include "defines.h"
class ThreadPool{
public:
    ThreadPool(std::size_t thread_nums = std::thread::hardware_concurrency(), std::size_t queue_size = 10000);
    ~ThreadPool();

    void add_task(Task &task, std::size_t index = -1);
    void add_task(Task &&task, std::size_t index = -1);
    void stop();

private:
    void start(std::size_t thread_nums);
    void stop_threadpool();

private:
    std::once_flag m_flag;
    std::vector<std::unique_ptr<Queue<Task>>> m_queues;
    std::vector<std::shared_ptr<std::thread>> m_workers;
    std::size_t m_thread_nums;
    std::atomic<bool> m_running;

};
#endif //PANDA_THREADPOOL_H
