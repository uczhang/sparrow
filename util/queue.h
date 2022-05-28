//
// Created by xiaohu on 3/21/22.
//

#ifndef PANDA_QUEUE_H
#define PANDA_QUEUE_H
#include <deque>
#include <mutex>
#include <condition_variable>
#include <type_traits>
#include <list>
#include "iostream"

template<typename T> class Queue{
public:
    Queue(const Queue&) = delete;
    Queue& operator = (const Queue&) = delete;

    Queue(std::size_t maxsize):m_maxsize(maxsize),m_stop(false){

    }

    void enqueue(T &value){
        add(value);
    }

    void enqueue(T &&value){
        add(std::move(value));
    }

    template<typename U> void add( U &&value){
        //static_assert(std::is_same_v<U, T>, "type U and T not same");
        {
            std::unique_lock<std::mutex> locker(m_mutex);
            m_not_full_cv.wait(locker, [this]()->bool{
                return notfull() || m_stop;
            });

            if(m_stop){
                return;
            }
            m_queue.push_back(std::forward<U>(value));
        }
        m_not_empty_cv.notify_one();
    }


    void dequeue(T &value){
        {
            std::unique_lock<std::mutex> locker(m_mutex);
            m_not_empty_cv.wait(locker, [this]()->bool{
                return notempty() || m_stop;
            });
            if(m_stop){
                return;
            }

            value = std::move(m_queue.front());
            m_queue.pop_front();
        }
        m_not_full_cv.notify_one();
    }

    void dequeue(std::list<T> &l){
        {
            std::unique_lock<std::mutex> locker(m_mutex);
            m_not_empty_cv.wait(locker, [this]()->bool{
                return notempty() || m_stop;
            });
            if(m_stop){
                return;
            }
            l = std::move(m_queue);
        }
        m_not_full_cv.notify_one();
    }

    std::size_t size(){
        std::scoped_lock<std::mutex> locker(m_mutex);
        return m_queue.size();
    }


    bool full(){
        std::scoped_lock<std::mutex> locker(m_mutex);
        return m_queue.size() >= m_maxsize;
    }

    bool empty(){
        std::scoped_lock<std::mutex> locker(m_mutex);
        return m_queue.empty();
    }

    void stop(){
        {
            std::lock_guard<std::mutex> locker(m_mutex);
            m_stop = true;
        }
        m_not_empty_cv.notify_all();
        m_not_full_cv.notify_all();
    }

private:
    bool notfull(){
        return m_queue.size() < m_maxsize;
    }

    bool notempty(){
        return !m_queue.empty();
    }

private:
    std::deque<T> m_queue;
    std::size_t m_maxsize;
    std::mutex m_mutex;
    std::condition_variable m_not_empty_cv;
    std::condition_variable m_not_full_cv;
    bool m_stop;
};
#endif //PANDA_QUEUE_H
