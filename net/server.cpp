//
// Created by xiaohu on 3/23/22.
//
#include "server.h"
#include "nserver.h"
#include "connection.h"
#include "threadpool.h"
#include "iocpool.h"
#include "nanolog.hpp"
#include "constant.h"

Server::Server(std::vector<int> &&ports,
        std::size_t worker_num,
        std::size_t conn_worker_num,
        const Handler& read_handler,
        const Handler& write_handler,
        const Predict &pred):m_worker_num(worker_num), m_conn_worker_num(conn_worker_num),
                             m_read_handler(read_handler),m_write_handler(write_handler),
                             m_pred(pred){
    m_ports = std::move(ports);
    m_acceptor_ioc = std::make_unique<asio::io_context>();
    m_acceptor_ioc_work = std::make_unique<asio::io_context::work>(*m_acceptor_ioc);
}
Server::~Server() {

    m_worker_pool->stop();
    m_ioc_pool->stop();
    m_acceptor_thread->join();
}

void Server::start() {
    m_worker_pool = std::make_shared<ThreadPool>(m_worker_num);
    m_ioc_pool = std::make_shared<IocPool>(m_conn_worker_num);
    m_run.store(true);

    auto i = 0;
    for(auto port : m_ports){
        m_acceptor.emplace_back(std::make_unique<asio::ip::tcp::acceptor>(*m_acceptor_ioc));
        std::error_code ec;
        auto endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port);
        m_acceptor[i]->open(endpoint.protocol());
        m_acceptor[i]->bind(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port), ec);
        if(ec){
            return;
        }
        m_acceptor[i]->set_option(asio::ip::tcp::acceptor::reuse_address(true), ec);
        if(ec){
            return;
        }
        m_acceptor[i]->listen();

        ++i;
    }

    m_acceptor_thread = std::make_shared<std::thread>([this]{
        std::error_code ec;
        m_acceptor_ioc->run(ec);
    });
    async_accept();
}

void Server::async_accept() {
    for(auto &acceptor : m_acceptor){

        auto newconn = std::make_shared<Connection>(m_ioc_pool->get_iocontext(), shared_from_this(), nullptr);
        acceptor->async_accept(newconn->get_socket(),[this, newconn](const std::error_code &ec)mutable {
            if(ec){
                //打印错误
                LOG_ERR << ec.message();
            }
            else{
                async_accept();
                handle_accept(newconn);
            }
        });
    }
}

void Server::async_read(std::shared_ptr<Connection> &conn, std::size_t expect_size){

        conn->async_read(expect_size, [this](std::shared_ptr<Connection> c,
                                        const std::error_code& ec,
                                        std::size_t bytes_transferred) ->void{

                handle_read(c, ec , bytes_transferred);

        });

}


void Server::async_write(std::shared_ptr<Connection> conn, std::shared_ptr<MsgBuffer> &&msg){

        conn->async_write([this](std::shared_ptr<Connection> c,
                                 const std::error_code& ec,
                                 std::size_t bytes_transferred)->bool{
            handle_write(c, ec, bytes_transferred);

            return true;
        }, std::move(msg));
}

void Server::handle_accept(std::shared_ptr<Connection> &conn){
    conn->set_conn_info();
    conn->get_socket().set_option(asio::ip::tcp::no_delay(true));
    conn->get_socket().non_blocking(true);
    asio::post(conn->get_iocontext(), [this, conn]{
        m_conn_manager[conn->get_uuid()] = conn;
    });

    async_read(conn, g_proto_head_len);
}

void Server::handle_read(std::shared_ptr<Connection> conn, const std::error_code& ec, std::size_t bytes_transferred){
    //处理接收消息
    conn->set_recvbytes(conn->get_recv_buffer()->available());
    auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
    conn->set_last_recvtime( now.count() );

    //业务处理[1、解码，校验, 2、根据消息类型业务处理]
    m_worker_pool->add_task([buffer = std::move(conn->get_recv_buffer()), this, conn]() mutable {
        m_read_handler(conn, buffer);
    });

}

void Server::handle_write(std::shared_ptr<Connection> conn, const std::error_code& ec, std::size_t bytes_transferred){
    //处理发送消息
    conn->set_sendbytes(bytes_transferred);
    auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
    conn->set_last_sendtime( now.count() );

    //发送成功后，业务处理
    //m_worker_pool->add_task([this, conn](){
    //    m_write_handler(conn);
    //});
}

void Server::async_timeout(std::shared_ptr<Connection> conn){
    conn->async_timeout([this, conn](){
        handle_timeout(conn);
    });
}

void Server::handle_timeout(std::shared_ptr<Connection> conn){

    //timer业务处理
    static thread_local std::uint8_t  timeout_counts = 0;
    auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
    if(now.count() - conn->get_last_recvtime() > 10){
        if(++timeout_counts > 10){
            conn->close();
        }
    }
    else{
        timeout_counts = 0;
    }
}

