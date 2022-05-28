//
// Created by xiaohu on 3/21/22.
//
#include "threadpool.h"
#include <random>

ThreadPool::ThreadPool(std::size_t thread_nums, std::size_t queue_size):m_thread_nums(thread_nums){
    for(auto i = 0; i < thread_nums; ++i){
        m_queues.emplace_back(new Queue<Task>(queue_size));
    }
    m_running.store(true);
    start(thread_nums);
}

ThreadPool::~ThreadPool(){
    stop();
}

void ThreadPool::add_task(Task &task, std::size_t index){
    if(index < 0 || index >= m_thread_nums){
        static thread_local std::default_random_engine e;
        static thread_local std::uniform_int_distribution<unsigned> u(0, m_thread_nums-1);
        m_queues[u(e)]->enqueue(std::move(task));
    }
    else{
        m_queues[index]->enqueue(task);
    }

}

void ThreadPool::add_task(Task &&task, std::size_t index){
    if(index < 0 || index >= m_thread_nums){
        static thread_local std::default_random_engine e;
        static thread_local std::uniform_int_distribution<unsigned> u(0, m_thread_nums-1);
        std::size_t pos = u(e);
        m_queues[pos]->enqueue(std::move(task));
    }
    else{
        m_queues[index]->enqueue(std::move(task));
    }

}

void ThreadPool::stop(){
    std::call_once(m_flag, [this](){
       stop_threadpool();
    });
}

void ThreadPool::stop_threadpool(){
    for(auto i = 0; i < m_thread_nums; ++i){
        m_queues[i]->stop();
    }
    m_running.store(false);

    for(auto &w : m_workers){
        w->join();
    }
}

void ThreadPool::start(std::size_t thread_nums){

    for(size_t i = 0; i < thread_nums; ++i){
        m_workers.push_back(std::make_shared<std::thread>([this](int index){
            while(m_running) {
                Task task{};
                m_queues[index]->dequeue(task);
                if(task != nullptr){
                    task();
                }
            }
        }, i));
    }

}
