//
// Created by xiaohu on 4/29/22.
//

#ifndef PANDA_CLIENT_H
#define PANDA_CLIENT_H
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <thread>
#include <asio.hpp>
#include <map>
#include "defines.h"
#include "instance.h"

class Connection;
class Server;
class IocPool;
class MsgBuffer;
class RepeatTimer;
class Client : public Instance, public std::enable_shared_from_this<Client>{
public:
    Client(const std::vector<std::pair<std::string,std::uint16_t>> &peers,
           std::size_t conn_woker_num = 2, std::size_t per_conn_num = 2, const Handler& read_handler = nullptr);
    Client(const Client&) = delete;
    Client& operator = (const Client&) = delete;

    virtual ~Client();
    void async_connect(const asio::ip::tcp::endpoint &endpoint);
    void async_read(std::shared_ptr<Connection> &conn);
    void async_write(std::shared_ptr<Connection> conn, std::shared_ptr<MsgBuffer> &&msg);
    void async_timeout(std::shared_ptr<Connection> c);

    virtual void release(std::size_t id) override{
        m_conn_manager.erase(id);
    }

    std::shared_ptr<Connection>& get_conn();
    void set_server(std::shared_ptr<Server> s){
        m_server = s;
    }
    void start();

private:
    void handle_connect(std::shared_ptr<Connection> c, asio::ip::tcp::endpoint &&endpoint);
    void handle_read(std::shared_ptr<Connection> c, const std::error_code &ec , std::size_t bytes_transferred);
    void handle_write(std::shared_ptr<Connection> c, const std::error_code &ec, std::size_t bytes_transferred);
    void handle_timeout(std::shared_ptr<Connection> c);

private:
    std::mutex m_manager_mutex;
    std::unordered_map<std::size_t, std::shared_ptr<Connection>> m_conn_manager;
    std::map<asio::ip::tcp::endpoint, std::size_t> m_server_conns;

    std::shared_ptr<std::thread> m_connector_thread{nullptr};
    std::unique_ptr<asio::io_context> m_connector_ioc{nullptr};
    std::unique_ptr<asio::io_context::work> m_connector_ioc_work{nullptr};

    std::size_t m_conn_worker_num;
    std::shared_ptr<IocPool> m_ioc_pool;
    std::size_t m_per_conn_num;

    std::vector<std::pair<std::string,std::uint16_t>> m_peers;

    Predict m_pred{nullptr};
    std::unique_ptr<RepeatTimer> m_reconnect_timer;
    Handler m_read_handler{nullptr};
    std::weak_ptr<Server> m_server;

};
#endif //PANDA_CLIENT_H
