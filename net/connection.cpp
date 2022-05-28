//
// Created by xiaohu on 3/21/22.
//
#include <connection.h>
#include <charconv>
#include <iostream>
#include "nanolog.hpp"
#include "server.h"
#include "constant.h"
Connection::Connection(asio::io_context &ioc, std::shared_ptr<Instance> holder,
                       TimeHandler&& handler,size_t msg_pool_num): m_ioc(ioc),m_socket(std::make_unique<asio::ip::tcp::socket>(ioc)),
                       m_strand(std::make_unique<asio::io_context::strand>(ioc)), m_holder(holder),
                       m_msg_pool(std::make_shared<ObjectPool<std::shared_ptr<MsgBuffer>>>([]{
                                return std::make_shared<MsgBuffer>();
                        }, msg_pool_num)){
    m_repeat_timer = std::unique_ptr<RepeatTimer>(new RepeatTimer(ioc, 10000, std::move(handler)));
}

Connection::~Connection(){
    m_repeat_timer->stop();
    close();
}

asio::ip::tcp::socket& Connection::get_socket(){
    return *m_socket;
}

void Connection::set_conn_info(){
    m_remote_ip = m_socket->remote_endpoint().address().to_string();
    m_remote_port = m_socket->remote_endpoint().port();

    m_local_ip = m_socket->local_endpoint().address().to_string();
    m_local_port = m_socket->local_endpoint().port();

    m_uuid = m_socket->native_handle();
}

void Connection::close(){
    if(!m_socket->is_open()){
        return;
    }
    std::error_code ec;
    m_socket->shutdown(asio::socket_base::shutdown_both, ec);
    m_socket->close(ec);

    if(auto pserver = m_holder.lock(); pserver != nullptr){
        pserver->release(get_uuid());
    }
}

void Connection::async_read(std::size_t expect_size, ReadHandler&& handler){

    if(m_recv_buffer == nullptr) {
        m_msg_pool->get([this](auto &&obj) {
            m_recv_buffer = std::move(obj);
        });
    }

    m_recv_buffer->adjust(expect_size);
    m_socket->async_read_some(asio::buffer(m_recv_buffer->data(), expect_size),
                              asio::bind_executor(*m_strand,[this, expect_size, self = this->shared_from_this(), handler = std::move(handler)](const std::error_code& ec, size_t bytes_transferred)mutable{
                    handle_read(expect_size, std::move(handler), ec, bytes_transferred);
    }));

}

void Connection::async_write(WriteHandler&& handler, std::shared_ptr<MsgBuffer> &&data){

    asio::post(m_ioc, asio::bind_executor(*m_strand,[this, self = shared_from_this(), data = std::move(data), handler]() mutable{
        if(data->get_indicator() == MsgBuffer::invalid || !m_socket->is_open()){
            recycle(data);
            close();
            LOG_ERR << get_remote_ip() << ":" << get_remote_port() << " socket [" << get_uuid() << "] is closed";
            return;
        }

        if(m_send_buffer_list.empty()){
            m_socket->async_write_some(asio::buffer(data->memory(), data->available()), [this, self, handler = std::move(handler), data](const std::error_code& ec, size_t bytes_transferred) mutable ->void{
                if(!ec){
                    data->consume(bytes_transferred);
                    data->update_packlen(bytes_transferred);
                    if(!data->empty()){
                        m_send_buffer_list.push_front(data);
                    }
                    else{
                        handler(shared_from_this(), ec, data->get_packlen());
                        recycle(data);
                    }
                }
                handle_write(handler, ec, bytes_transferred);
            });
        }
        else{
            m_send_buffer_list.push_back(std::move(data));
        }
    }));

}

void Connection::async_timeout(TimeHandler&& handler){
    m_repeat_timer->set_handler([this, handler = std::move(handler)]()mutable {
        handle_timeout(std::move(handler));
    });

    m_repeat_timer->start();
}

void Connection::handle_read(std::size_t expect_size, ReadHandler&& handler, const std::error_code& ec, std::size_t bytes_transferred){
    std::pair<bool,int> ret{};

    if(ec) {
        if (ec.value() == asio::error::interrupted || ec.value() == asio::error::try_again) {
            ret = {true, expect_size};
        }
        else{
            LOG_ERR << "errValue:[" << ec.value() << "] errMessage:[" << ec.message() << "]";
            close();
            ret = {false, -1};
        }
    }
    else{
        m_recv_buffer->commit(bytes_transferred);
        auto check_result = PackageCheck{}(*m_recv_buffer);

        switch (check_result) {
            case PackageStat::INCORRECT:
            {
                close();
                ret = {false, -1};
                break;
            }
            case PackageStat::WHOLE:
            {
                m_recv_buffer->set_indicator(MsgBuffer::valid);
                handler(shared_from_this(), ec, bytes_transferred);
                ret = {true, g_proto_head_len};
                break;
            }
            case PackageStat::NEED_HEAD:
            {
                ret = {true, g_proto_head_len - m_recv_buffer->available()};
                break;
            }
            case PackageStat::NEED_BODY:
            {
                ret = {true, m_recv_buffer->get_packlen()-m_recv_buffer->available()};
                break;
            }
        }
    }

    LOG_INFO << "first: " << ret.first << " expect_size: " << ret.second;
    if(ret.first){
        if(m_recv_buffer == nullptr){
            m_msg_pool->get([this](auto &&obj) {
                m_recv_buffer = std::move(obj);
            });
        }

        m_recv_buffer->adjust(ret.second);
        m_socket->async_read_some(asio::buffer(m_recv_buffer->data(), ret.second),
                                  asio::bind_executor(*m_strand,[this, expect_size = ret.second, self = this->shared_from_this(), handler=std::move(handler)](const std::error_code& ec, size_t bytes_transferred)mutable {
                                      handle_read(expect_size, std::move(handler), ec, bytes_transferred);
                                  }));
    }
}

void Connection::handle_write(const WriteHandler& handler, const std::error_code& ec, std::size_t bytes_transferred){
    if((ec && ec.value() != asio::error::interrupted && ec.value() != asio::error::try_again) ||
       !bytes_transferred){
            LOG_ERR << this->get_uuid() << " is closed by " << ec.message();
            close();
    }

    while(!m_send_buffer_list.empty()){
        auto data = std::move(m_send_buffer_list.front());
        m_send_buffer_list.pop_front();
        m_socket->async_write_some(asio::buffer(data->memory(), data->available()),
                                   asio::bind_executor(*m_strand,[this, &handler, data](const std::error_code &ec, size_t bytes_transferred)mutable{
                                        if(!ec){
                                            data->consume(bytes_transferred);
                                            data->update_packlen(bytes_transferred);
                                            if(!data->empty()){
                                              m_send_buffer_list.push_front(data);
                                            }
                                            else{
                                                handler(shared_from_this(), ec, data->get_packlen());
                                                recycle(data);
                                            }
                                        }
                                       handle_write(handler, ec, bytes_transferred);
                                   }));
    }
}

void Connection::handle_timeout(TimeHandler&& handler){
    handler();
}

Connection::PackageStat Connection::PackageCheck::operator()(MsgBuffer& buffer){

    if(buffer.get_packlen() == 0){
        if(buffer.available() < g_proto_head_len) {
            return PackageStat::NEED_HEAD;
        }

        if(*reinterpret_cast<std::uint32_t*>(buffer.memory()) != *reinterpret_cast<const std::uint32_t*>(g_head_magic_num)){
            return PackageStat::INCORRECT;
        }
        std::uint32_t packlen = 0;
        buffer.consume(sizeof(std::uint32_t));
        buffer.peek(packlen);
        buffer.set_packlen(packlen+2);
        buffer.vomit(sizeof(std::uint32_t));
    }

    if(buffer.get_packlen() > m_package_len_max || buffer.get_packlen() < g_proto_head_len + 2){
        return PackageStat::INCORRECT;
    }
    else{

        if(buffer.available() < buffer.get_packlen()){
            return PackageStat::NEED_BODY;
        }
        else{
            return PackageStat::WHOLE;
        }
    }
}