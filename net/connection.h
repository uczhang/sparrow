//
// Created by xiaohu on 3/21/22.
//
#ifndef PANDA_CONNECTION_H
#define PANDA_CONNECTION_H
#include "asio.hpp"
#include <memory>
#include <list>
#include <any>
#include "defines.h"
#include "repeattimer.h"
#include "msgbuffer.h"
#include "objectpool.h"
#include "instance.h"
class Connection final : public std::enable_shared_from_this<Connection>{
public:
    enum class PackageStat{
        NEED_HEAD,
        NEED_BODY,
        WHOLE,
        INCORRECT
    };

    struct PackageCheck{
        PackageStat operator()(MsgBuffer& buffer);
    private:
        inline constexpr static std::uint32_t m_package_len_max{2*1024*1024};
    };

public:

    using WriteHandler = std::function<bool(std::shared_ptr<Connection>,
                                            const std::error_code&  ec,
                                            size_t bytes_transferred)>;

    using ReadHandler = std::function<void(std::shared_ptr<Connection>,
                                           const std::error_code&  ec,
                                           size_t bytes_transferred)>;

    Connection() = default;
    explicit Connection(asio::io_context &ioc, std::shared_ptr<Instance> holder, TimeHandler&& handler, std::size_t msg_pool_num = 1000);
    ~Connection();

    void async_read(std::size_t expect_size, ReadHandler&& handler);
    void async_write(WriteHandler&& handler, std::shared_ptr<MsgBuffer> &&data);
    void async_timeout(TimeHandler&& handler);


    asio::ip::tcp::socket& get_socket();
    void set_conn_info();
    void close();
    std::string& get_remote_ip(){
        return m_remote_ip;
    }

    int get_remote_port(){
        return m_remote_port;
    }

    std::size_t& get_uuid(){
        return m_uuid;
    }

    auto &get_recv_buffer(){
        return m_recv_buffer;
    }

    std::size_t get_recv_bytes(){
        return m_recv_bytes;
    }

    std::size_t get_send_bytes(){
        return m_send_bytes;
    }

    std::uint64_t get_last_recvtime(){
        return m_last_recv_time;
    }

    std::uint64_t get_last_sendtime(){
        return m_last_send_time;
    }

    asio::io_context& get_iocontext(){
        return m_ioc;
    }

    void free(std::shared_ptr<MsgBuffer> &buffer){
        m_msg_pool->free(buffer);
    }

    void recycle(std::shared_ptr<MsgBuffer> &buffer){
        buffer->clear();
        free(buffer);
    }

    std::shared_ptr<MsgBuffer> use(){
        std::shared_ptr<MsgBuffer> buf;
        m_msg_pool->get([&buf](auto &&obj){
            buf =  std::move(obj);
        });
        return buf;
    }

    void set_last_sendtime(std::uint64_t time){
        m_last_send_time = time;
    }
    void set_sendbytes(std::uint64_t bytes){
        m_send_bytes += bytes;
    }

    void set_last_recvtime(std::uint64_t time){
        m_last_send_time = time;
    }
    void set_recvbytes(std::uint64_t bytes){
        m_send_bytes += bytes;
    }

private:
    void handle_read(std::size_t expect_size, ReadHandler&& handler, const std::error_code& ec, std::size_t bytes_transferred);
    void handle_write(const WriteHandler& handler, const std::error_code& ec, std::size_t bytes_transferred);
    void handle_timeout(TimeHandler&& handler);

private:
    asio::io_context &m_ioc;

    std::shared_ptr<MsgBuffer> m_recv_buffer{nullptr};
    std::list<std::shared_ptr<MsgBuffer>> m_send_buffer_list;
    std::unique_ptr<asio::ip::tcp::socket> m_socket;
    std::unique_ptr<RepeatTimer> m_repeat_timer;
    std::unique_ptr<asio::io_context::strand> m_strand;

    //msgbuffer
    std::shared_ptr<ObjectPool<std::shared_ptr<MsgBuffer>>> m_msg_pool;
    std::weak_ptr<Instance> m_holder;
    std::string m_remote_ip;
    int m_remote_port;
    std::string m_local_ip;
    int m_local_port;

    std::size_t m_uuid;

    std::uint64_t m_send_bytes{0};
    std::uint64_t m_recv_bytes{0};

    std::uint64_t m_last_recv_time;
    std::uint64_t m_last_send_time;

};
#endif //PANDA_CONNECTION_H
