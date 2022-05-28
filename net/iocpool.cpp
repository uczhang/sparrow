//
// Created by xiaohu on 2022/4/12.
//
#include "iocpool.h"
#include "iostream"

IocPool::IocPool(std::size_t size):m_ioc_pos(0){

    for(auto i = 0; i < size; ++i){
        m_iocs.emplace_back(std::make_unique<asio::io_context>());
    }

    for(auto &ioc : m_iocs){
        m_ioc_works.emplace_back(std::make_unique<asio::io_context::work>(*ioc));
        m_threads.emplace_back(std::make_unique<std::thread>([&ioc](){
            std::error_code ec;
            ioc->run(ec);
        }));
    }
}

asio::io_context& IocPool::get_iocontext(){
    asio::io_context &ioc = *m_iocs[m_ioc_pos];
    ++m_ioc_pos;
    if(m_ioc_pos >= m_iocs.size()){
        m_ioc_pos = 0;
    }
    return ioc;
}

void IocPool::stop(){
    for(auto &ioc : m_iocs){
        ioc->stop();
        ioc.reset();
    }
    m_iocs.clear();

    for(auto &ioc_work : m_ioc_works){
        ioc_work.reset();
    }
    m_ioc_works.clear();

    for(auto &t : m_threads){
        t->join();
    }
    m_threads.clear();
}

IocPool::~IocPool() {
    stop();
}