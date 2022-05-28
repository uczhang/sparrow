//
// Created by xiaohu on 2022/4/12.
//

#ifndef PANDA_IOCPOOL_H
#define PANDA_IOCPOOL_H
#include "asio.hpp"
#include <thread>
class IocPool{
public:
    IocPool(std::size_t size = std::thread::hardware_concurrency());
    IocPool(const IocPool&) = delete;
    IocPool& operator=(const IocPool&) = delete;
    asio::io_context& get_iocontext();
    void stop();
    ~IocPool();
private:
    std::vector<std::unique_ptr<asio::io_context>> m_iocs;
    std::vector<std::unique_ptr<asio::io_context::work>> m_ioc_works;
    std::vector<std::unique_ptr<std::thread>> m_threads;
    std::atomic_size_t m_ioc_pos;
};
#endif //PANDA_IOCPOOL_H
