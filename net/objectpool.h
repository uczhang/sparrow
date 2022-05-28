//
// Created by xiaohu on 4/24/22.
//

#ifndef PANDA_OBJECTPOOL_H
#define PANDA_OBJECTPOOL_H
#include <list>
#include <mutex>
#include <type_traits>
#include <functional>
#include <memory>
#include <iostream>
template<typename T> class ObjectPool final{
public:
    explicit ObjectPool(const std::function<T()>& creator, std::size_t sz = 10):m_construct(creator){
        if constexpr (std::is_constructible_v<T>){
            for(auto i = 0; i < sz; ++i){
                m_free_list.template emplace_back(m_construct());
            }
            //for(auto &k : m_free_list){
            //    std::cout <<  reinterpret_cast<unsigned long >((reinterpret_cast<char*>(k.get()))) << std::endl;
            //}
        }
    }

    ~ObjectPool(){
        std::lock_guard<std::mutex> locker(m_mutex);
        for(auto &v : m_free_list){
            v.reset();
        }
        m_free_list.clear();
    }

    void get(const std::function<void(T&&)> &proc){
        std::lock_guard<std::mutex> locker(m_mutex);
        if(!m_free_list.empty()){
            auto node = std::move(m_free_list.front());
            m_free_list.pop_front();
            proc(std::move(node));
        }
        else{
            proc(m_construct());
        }
    }

    void free(T& obj){
        std::lock_guard<std::mutex> locker(m_mutex);
        m_free_list.push_back(obj);
    }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator= (const ObjectPool&) = delete;
private:
    std::mutex m_mutex;
    std::list<T> m_free_list;
    std::function<T()> m_construct;
};
#endif //PANDA_OBJECTPOOL_H
