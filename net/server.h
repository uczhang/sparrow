//
// Created by xiaohu on 3/23/22.
//

#ifndef PANDA_SERVER_H
#define PANDA_SERVER_H
#include <memory>
#include <vector>
#include <atomic>
#include <unordered_map>
#include "asio.hpp"
#include "defines.h"
#include "instance.h"
#include <cstdint>
#include <connection.h>
//class Connection;
class IocPool;
class ThreadPool;
class Server : public Instance, public std::enable_shared_from_this<Server>{
public:
    Server(std::vector<int> &&ports,
            std::size_t worker_num = 3,
            std::size_t conn_worker_num = 2,
            const Handler&  read_handler = nullptr,
            const Handler&  write_handler = nullptr,
            const Predict& pred = nullptr);
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    virtual ~Server();
    void async_read(std::shared_ptr<Connection> &conn, std::size_t expect_size);
    void async_write(std::shared_ptr<Connection> conn, std::shared_ptr<MsgBuffer> &&msg);
    void async_timeout(std::shared_ptr<Connection> conn);
    void start();
    virtual void release(std::size_t id) override{
        m_conn_manager.erase(id);
    }
    std::shared_ptr<ThreadPool>& get_workpool(){
        return m_worker_pool;
    }

protected:
    Handler m_read_handler{nullptr};
    Handler m_write_handler{nullptr};
    Predict m_pred{nullptr};
    std::atomic<bool> m_run{false};

private:
    void async_accept();
    void handle_read(std::shared_ptr<Connection> conn, const std::error_code& ec, std::size_t bytes_transferred);
    void handle_write(std::shared_ptr<Connection> conn, const std::error_code& ec, std::size_t bytes_transferred);
    void handle_accept(std::shared_ptr<Connection> &conn);
    void handle_timeout(std::shared_ptr<Connection> conn);

private:
    inline static thread_local std::unordered_map<std::size_t, std::shared_ptr<Connection>> m_conn_manager{};
    std::vector<std::unique_ptr<asio::ip::tcp::acceptor>> m_acceptor;
    std::vector<int> m_ports;
    std::size_t m_worker_num;
    std::size_t m_conn_worker_num;

    //process bussiness
    std::shared_ptr<ThreadPool> m_worker_pool;
    //process connection
    std::shared_ptr<IocPool> m_ioc_pool;

    std::shared_ptr<std::thread> m_acceptor_thread;
    std::unique_ptr<asio::io_context>  m_acceptor_ioc{nullptr};
    std::unique_ptr<asio::io_context::work> m_acceptor_ioc_work{nullptr};

};
#endif //PANDA_SERVER_H
