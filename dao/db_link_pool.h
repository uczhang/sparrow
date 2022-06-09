//
// Created by ucbzh on 2022/5/25.
//

#ifndef CHAMELEON_DB_LINK_POOL_H
#define CHAMELEON_DB_LINK_POOL_H
#include <deque>
#include <memory>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <string>
#include <tuple>
#include "singleton.h"
#include "iostream"

template<typename T> class LinkPool: public Singleton<LinkPool<T>>{
public:
    friend class Singleton<LinkPool<T>>;
    template<typename... Args> void init(int maxsize, Args&& ...args){
        std::call_once(m_flag, &LinkPool<T>::init_impl<Args...>, this, maxsize, std::forward<Args>(args)...);
    }

    std::shared_ptr<T> get_link(){
        std::unique_lock<std::mutex> locker(m_mutex);
        while(m_pool.empty()) {
            if (m_condition.wait_for(locker, std::chrono::seconds(2), [this](){
                return !m_pool.empty();
            }) == false) {
                return nullptr;
            }
        }
        auto link = m_pool.front();
        m_pool.pop_front();
        locker.unlock();

        if(link == nullptr || !link->ping()){
            return create_link();
        }
        auto now = std::chrono::system_clock::now();
        auto last = link->get_operate_time();
        auto mins = std::chrono::duration_cast<std::chrono::minutes>(now - last).count();
        if((mins- 6*60) > 0){
            return create_link();
        }
        link->update_operate_time();

        return link;
    }


    void return_back(std::shared_ptr<T> link){
        if(link == nullptr || link->has_error()){
            std::cout << "return_back nullptr" << std::endl;
            link = create_link();
        }
        std::unique_lock<std::mutex> locker(m_mutex);
        m_pool.push_back(link);
        locker.unlock();
        m_condition.notify_one();
    }

private:
    template<typename ...Args> void init_impl(int maxsize, Args&& ...args){
        m_args = std::make_tuple(std::forward<Args>(args)...);
        for(int i = 0; i < maxsize; ++i){
            auto link = std::make_shared<T>();
            if(link->open(std::forward<Args>(args)...)){
                m_pool.push_back(link);
            }
            else{
                break;
            }
        }
    }

    auto create_link(){
        auto link = std::make_shared<T>();
        return std::apply([link](auto ...params)->bool {
            return link->connect(params...);
        }, m_args) ? link : nullptr;
    }

    LinkPool() = default;
    ~LinkPool() = default;
    LinkPool(const LinkPool&) = delete;
    LinkPool& operator=(const LinkPool&) = delete;

private:
    std::deque<std::shared_ptr<T>> m_pool;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::once_flag m_flag;
    std::tuple<const char*, const char*, const char*, const char*, int, int> m_args;
};

template<typename T> class LinkGuard{
public:
    LinkGuard(std::shared_ptr<T> link) : m_link(link){}
    ~LinkGuard(){
        LinkPool<T>::instance().return_back(m_link);
    }

    std::shared_ptr<T>& get(){
        return m_link;
    }
private:
    std::shared_ptr<T> m_link;
};

#endif //CHAMELEON_DB_LINK_POOL_H
