//
// Created by xiaohu on 4/29/22.
//

#include "client.h"
#include "iocpool.h"
#include "connection.h"
#include <memory>
#include "nanolog.hpp"
#include "repeattimer.h"
#include <iostream>
#include <random>
#include "server.h"
#include "threadpool.h"
#include "constant.h"
Client::Client(const std::vector<std::pair<std::string,std::uint16_t>> &peers,
               std::size_t conn_woker_num,
               std::size_t per_conn_num,
               const Handler& read_handler):m_peers(peers),m_conn_worker_num(conn_woker_num),
                                            m_per_conn_num(per_conn_num),m_read_handler(read_handler){
    m_connector_ioc = std::make_unique<asio::io_context>();
    m_connector_ioc_work = std::make_unique<asio::io_context::work>(*m_connector_ioc);
    m_reconnect_timer = std::unique_ptr<RepeatTimer>(new RepeatTimer(*m_connector_ioc, 10000, [this](){
        std::scoped_lock<std::mutex> locker(m_manager_mutex);
        for(auto &[endpoint, conn_num] : m_server_conns){
            if(conn_num < m_per_conn_num){
                async_connect(endpoint);
            }
        }
    }));
}

Client::~Client(){
    {
        std::scoped_lock<std::mutex> locker(m_manager_mutex);
        for(auto &[uuid, c] : m_conn_manager){
            c->close();
        }
    }
    m_reconnect_timer->stop();
    m_connector_thread->join();

}

void Client::start(){
    m_ioc_pool = std::make_shared<IocPool>(m_conn_worker_num);
    m_connector_thread = std::make_shared<std::thread>([this]{
        std::error_code ec;
        m_connector_ioc->run(ec);
    });

    //asio::ip::tcp::resolver resolver(*m_connector_ioc);
    asio::post(*m_connector_ioc, [this]{
        for(auto [host, port] : m_peers){
            //auto query = asio::ip::tcp::resolver::query(host, port);
            //auto endpoints = resolver.resolve(query);
            //async_connect(endpoints);
            asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(host), port);
            async_connect(endpoint);
        }

    });
    m_reconnect_timer->start();
}


std::shared_ptr<Connection>& Client::get_conn() {
   std::lock_guard<std::mutex> locker(m_manager_mutex);
    static thread_local std::default_random_engine random;
    std::size_t i = 0;
    for(auto it = m_conn_manager.begin(); it != m_conn_manager.end(); ++it, ++i){
        if(i == random()%m_conn_manager.size()){
            return it->second;
        }
    }
    return m_conn_manager.begin()->second;
}

void Client::async_connect(const asio::ip::tcp::endpoint &endpoint){

    for(std::size_t i = 0; i < m_per_conn_num; ++i){
        auto newconn = std::make_shared<Connection>(m_ioc_pool->get_iocontext(), shared_from_this(), nullptr);
        newconn->get_socket().async_connect(endpoint, [this, newconn, endpoint = std::move(endpoint)](const std::error_code &ec)mutable {
            if(ec){
                newconn->close();
                LOG_INFO << "close " << endpoint.address().to_string() << ":" << endpoint.port();
            }
            else{
                LOG_INFO << "connect " << endpoint.address().to_string() << endpoint.port();
                handle_connect(newconn, std::move(endpoint));
            }
        });
    }
}

void Client::handle_connect(std::shared_ptr<Connection> c, asio::ip::tcp::endpoint &&endpoint){
    c->set_conn_info();
    c->get_socket().set_option(asio::ip::tcp::no_delay(true));
    c->get_socket().non_blocking(true);
    {
        std::lock_guard<std::mutex> locker(m_manager_mutex);
        m_conn_manager[c->get_uuid()] = c;

        if(m_server_conns.find(endpoint) == m_server_conns.end()){
            m_server_conns[endpoint] = 1;
        }
        else{
            m_server_conns[endpoint] += 1;
        }

    }
    async_timeout(c);
}


void Client::async_write(std::shared_ptr<Connection> conn, std::shared_ptr<MsgBuffer> &&msg){

    conn->async_write([this](std::shared_ptr<Connection> c,
                             const std::error_code& ec,
                             std::size_t bytes_transferred)->bool{

        handle_write(c, ec, bytes_transferred);
        return true;
    }, std::move(msg));

}

void Client::async_read(std::shared_ptr<Connection> &conn){

    conn->async_read(g_proto_head_len, [this](std::shared_ptr<Connection> c,
                                          const std::error_code&  ec,
                                          size_t bytes_transferred){
        handle_read(c, ec , bytes_transferred);
    });
}

void Client::handle_read(std::shared_ptr<Connection> c, const std::error_code &ec , std::size_t bytes_transferred){

    if(m_read_handler != nullptr){
        if(auto s = m_server.lock()){
            c->set_recvbytes(c->get_recv_buffer()->available());
            auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
            c->set_last_recvtime( now.count() );

            s->get_workpool()->add_task([buffer = std::move(c->get_recv_buffer()), this, c]() mutable {
                m_read_handler(c, buffer);
            });
        }
    }

}

void Client::handle_write(std::shared_ptr<Connection> c, const std::error_code &ec, std::size_t bytes_transferred){
    c->set_sendbytes(bytes_transferred);
    auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
    c->set_last_sendtime( now.count() );
    async_read(c);
}


void Client::async_timeout(std::shared_ptr<Connection> c) {
    c->async_timeout([this, c](){
        handle_timeout(c);
    });
}

void Client::handle_timeout(std::shared_ptr<Connection> c){

    //per 10 seconds send keepalive pkg;
    auto  data = c->use();
    data->seek(4);
    (*data) << std::uint32_t(g_proto_head_len);
    (*data) << CmdType::ACT_CMD;
    data->vomit(4);
    data->reset(0, g_head_magic_num);
    std::uint16_t  crc_val = Request::crc16(data->memory() + 8, data->available()-8);
    (*data) << crc_val;
    data->set_indicator(MsgBuffer::valid);
    async_write(c, std::move(data));
}