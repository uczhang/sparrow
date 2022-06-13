//
// Created by xiaohu on 5/9/22.
//

#ifndef PANDA_NSERVER_H
#define PANDA_NSERVER_H
#include <iostream>
#include <memory>
#include <optional>
#include "server.h"
#include "client.h"
#include "defines.h"
#include "nanolog.hpp"
#include "singleton.h"
#include "protocol/proto.h"

class Application : public Singleton<Application>{
public:
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    friend class Singleton<Application>;

public:
    std::unique_ptr<Request> handle_read_msg(std::shared_ptr<Connection> c, std::shared_ptr<MsgBuffer> &data);
    void handle_write_msg(std::shared_ptr<Connection> c, std::unique_ptr<Response> &&rsp);
    void handle_server_request(std::shared_ptr<Connection> c, const Request& req);
    void start();

    void set_context(std::function<bool(const RequestDeliver&)>&& logic_proc);

    bool handle_bind_msg(const Request& req, Response& rsp);
    bool handle_act_msg(Response& rsp);
    bool handle_deliver_msg(Response& rsp);
    void handle_to_audit_request(std::unique_ptr<RequestAudit> req);
    std::unique_ptr<Response> handle_from_audit_response(std::shared_ptr<Connection> c, std::shared_ptr<MsgBuffer>& data);


private:
    void set_logic_proc_app(std::function<bool(const RequestDeliver&)>&& logic_proc, std::shared_ptr<Server>&& server,
                            std::shared_ptr<Client>&& client){
        m_deliver_msg_proc = std::move(logic_proc);
        m_audit_client = std::move(client);
        m_server = std::move(server);
        m_audit_client->set_server(m_server);
    }
    void handle_audit_msg(RequestAudit& req,  std::shared_ptr<MsgBuffer>& data);
    void handle_signals(const std::error_code& error, int signal_number);
    
private:
    Application(): m_work(m_service), m_signals(m_service){}
    std::function<bool(const RequestDeliver& req)> m_deliver_msg_proc;
    std::shared_ptr<Server> m_server{nullptr};
    std::shared_ptr<Client> m_audit_client{nullptr};
    asio::io_context m_service;
    asio::io_context::work m_work;
    asio::signal_set m_signals;
};

bool send_rsp_msg(std::unique_ptr<ResponseDeliver> &&rsp);
bool send_adt_msg(std::unique_ptr<RequestAudit> &req);

#endif //PANDA_NSERVER_H
