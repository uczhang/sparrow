//
// Created by xiaohu on 5/10/22.
//
#include <nlohmann_json.hpp>
#include "application.h"
#include "nanolog.hpp"
#include "server.h"
#include "constant.h"
extern  nlohmann::json cfg;

static bool parse_check(std::shared_ptr<MsgBuffer> &data){
    std::uint16_t crc_pack;
    data->fetch(data->available()-2, 2, [&crc_pack](const char* buf, std::size_t len){
        memcpy(&crc_pack, buf, len);
        crc_pack = ntohs(crc_pack);
    });

    std::uint16_t crc_calc;
    data->fetch(8, data->available()-8-2, [&crc_calc, &data](const char* buf, std::size_t len){
        crc_calc = Request::crc16(buf, len);
    });
    if(crc_pack != crc_calc){
        LOG_ERR << "crc_calc: [" << crc_calc << "] crc_pack: [" << crc_pack << "]";
        return false;
    }

    std::uint32_t inpack_len = static_cast<std::uint32_t>(data->available()-6);
    data->reset(4, inpack_len);
    data->consume(4);
    return true;
}

std::unique_ptr<Request> Application::handle_read_msg(std::shared_ptr<Connection> c, std::shared_ptr<MsgBuffer> &data){

    if(!parse_check(data)){
        return nullptr;
    }
    Head head;
    head.decode(*data);
    std::unique_ptr<Request> msg{nullptr};
    switch (head.m_cmd) {
        case CmdType::BIND_CMD:
        {
            msg = std::make_unique<RequestBind>(c);
            msg->decode_body(*data);
            msg->set_head(head);
            break;
        }
        case CmdType::ACT_CMD:
        {
            msg = std::make_unique<RequestAct>(c);
            msg->set_head(head);
            break;
        }

        case CmdType::DELIVER_MSG_CMD:
        {
            msg = std::make_unique<RequestDeliver>(c);
            msg->decode_body(*data);
            msg->set_head(head);
            if(auto p = dynamic_cast<RequestDeliver*>(msg.get()); p != nullptr){

                std::cout << p->get_body().agent_id <<" "<< p->get_body().msg_id <<" "<< p->get_body().token_id <<" "<< p->get_body().svc_type << std::endl;
                std::cout << static_cast<std::uint32_t>(p->get_body().content_type) <<" "<< p->get_body().timestamp <<" "<< p->get_body().from<<" "<< p->get_body().counts << std::endl;
                std::cout << p->get_body().to <<" "<< p->get_body().hash <<" "<< p->get_body().content_length <<" "<< p->get_body().content_text << std::endl;
                std::cout << p->get_body().content_url <<"url--- "<< p->get_body().remark <<"mark--- "<< p->get_body().file_name <<"file_name--- "<< static_cast<std::uint32_t>(p->get_body().is_review) << std::endl;
                std::cout << p->get_body().session_id <<"session_id--- "<< p->get_body().kernel_id <<"kernel_id--- "<< p->get_body().text_is_sync << std::endl;

            }
            break;
        }
        default:
        {
            LOG_ERR << "hp/tp request cmd is not correct!";
            break;
        }

    }

    c->recycle(data);
    return msg;
}

void Application::handle_write_msg(std::shared_ptr<Connection> c, std::unique_ptr<Response> &&rsp){

    std::shared_ptr<MsgBuffer> data = c->use();
    if(rsp == nullptr){
        data->set_indicator(MsgBuffer::invalid);
        m_server->async_write(c, std::move(data));
        LOG_ERR << " MsgBuff use() function return nullptr in " << __FUNCTION__ ;
        return;
    }
    data->seek(4);
    if(!rsp->makeup(*data)){
        LOG_ERR << " MsgBuff makeup() function return false in " << __FUNCTION__ ;
        return;
    }
    data->vomit(4);

    data->reset(0, g_head_magic_num);
    std::uint32_t packlen = data->available() - 4;
    data->reset(4, packlen);

    std::uint16_t  crc_val = Request::crc16(data->memory() + 8, packlen - 8);
    (*data) << crc_val;
    data->set_indicator(MsgBuffer::valid);
    m_server->async_write(c, std::move(data));
    return ;
}

void Application::handle_server_request(std::shared_ptr<Connection> c, const Request& req){

    switch(req.get_head().m_cmd){
        case CmdType::BIND_CMD:
        {
            std::unique_ptr<Response> rsp = std::make_unique<ResponseBind>(c);
            handle_bind_msg(req, *rsp);
            std::cout << "rsp.len: [" << rsp->get_head().m_len << "] rsp.cmd: [" << std::hex << rsp->get_head().m_cmd << std::dec<< "]" << std::endl;
            handle_write_msg(c, std::move(rsp));
            break;
        }

        case CmdType::ACT_CMD:
        {
            std::unique_ptr<Response> rsp = std::make_unique<ResponseAct>(c);
            handle_act_msg(*rsp);
            std::cout << "rsp.len: [" << rsp->get_head().m_len << "] rsp.cmd: [" << std::hex << rsp->get_head().m_cmd << std::dec<< "]" << std::endl;
            handle_write_msg(c, std::move(rsp));
            break;
        }

        case CmdType::DELIVER_MSG_CMD:
        {
            if(m_deliver_msg_proc != nullptr){
                const RequestDeliver &req_deliver = dynamic_cast<const RequestDeliver&>(req);
                m_deliver_msg_proc(req_deliver);
            }
            break;
        }

        default:
            handle_write_msg(c, nullptr);
    }

}

bool Application::handle_bind_msg(const Request& req, Response& rsp){

    const RequestBind &bind_req = static_cast<const RequestBind&>(req);
    ResponseBind &bind_rsp = static_cast<ResponseBind&>(rsp);

    bind_rsp.get_body().uid = bind_rsp.get_body().uid;

    bind_rsp.get_head().m_cmd = CmdType::BIND_CMD_RSP;
    bind_rsp.get_head().m_len = sizeof(Head) + bind_rsp.get_body().uid.size();

    return true;
}

bool Application::handle_act_msg(Response& rsp){
    rsp.get_head().m_cmd = CmdType::ACT_CMD_RSP;
    rsp.get_head().m_len = sizeof(Head);

    return true;
}

bool Application::handle_deliver_msg(Response& rsp){
    rsp.get_head().m_cmd = CmdType::DELIVER_MSG_CMD_RSP;
    return true;
}

void  Application::set_context(std::function<bool(const RequestDeliver&)>&& logic_proc){

    set_logic_proc_app(std::move(logic_proc),
                       std::make_shared<Server>(
                               cfg.at("ports").get<std::vector<int>>(),
                               cfg.at("worker_num"),
                               cfg.at("server_conn_worker_num"),
                               [this](std::shared_ptr<Connection> c, std::shared_ptr<MsgBuffer> &data)mutable {
                                   std::unique_ptr<Request> req = handle_read_msg(c, data);
                                   if (req == nullptr) {
                                       handle_write_msg(c, nullptr);
                                       return;
                                   }

                                   handle_server_request(c, *req);
                               },
                               nullptr,
                               nullptr),
                       std::make_shared<Client>(
                               std::vector<std::pair<std::string, std::uint16_t>>{{"172.21.22.155", 23071}},
                               cfg.at("client_conn_worker_num"),
                               cfg.at("per_conn_num"),
                               [this](std::shared_ptr<Connection> c, std::shared_ptr<MsgBuffer> &data) {
                                   handle_from_audit_response(c, data);
                               }
                       ));
}

void Application::handle_to_audit_request(std::unique_ptr<RequestAudit> req){
    req->get_head().m_cmd = CmdType::AUDIT_TOKEN_CMD;
    static thread_local std::weak_ptr<Connection> c;
    auto sc = c.lock();
    if(!sc){
        c  = m_audit_client->get_conn();
        sc = c.lock();
    }
    std::shared_ptr<MsgBuffer> data = sc->use();

    handle_audit_msg(*req, data);

    m_audit_client->async_write(sc, std::move(data));
}

void Application::handle_audit_msg(RequestAudit& req, std::shared_ptr<MsgBuffer>& data){
    req.get_head().m_cmd = CmdType::AUDIT_TOKEN_CMD;
    req.makeup(*data);
}

std::unique_ptr<Response> Application::handle_from_audit_response(std::shared_ptr<Connection> c, std::shared_ptr<MsgBuffer>& data){
    if(!parse_check(data)){
        return nullptr;
    }
    Head head;
    head.decode(*data);
    std::unique_ptr<Response> msg{nullptr};
    switch (head.m_cmd) {
        case CmdType::ACT_CMD_RSP:
        case CmdType::AUDIT_RSP_TOKEN_CMD:
        {
            std::cout << "++++cmd: " << head.m_cmd << std::endl;
            break;
        }
        case CmdType::AUDIT_RESULT_TOKEN_CMD:
        {
            break;
        }
        default:
        {
            LOG_ERR << "audit server response cmd is not correct!";
        }
    }

    c->recycle(data);
    return msg;
}

void Application::handle_signals(const std::error_code& error, int signal_number){
    if (!error){
        switch (signal_number) {
            case SIGINT:
            case SIGTERM:
            case SIGPIPE:
            {
                std::cout << "ignore signal " << signal_number << std::endl;
                break;
            }
            case SIGHUP:
            {
                std::cout << "SIGHUP signal need processing" << signal_number << std::endl;
                m_service.stop();
                break;
            }
        }
    }
    m_signals.async_wait([this](const std::error_code& error , int signal_number ){
        handle_signals(error, signal_number);
    });
}

void Application::start(){

    //处理信号
    m_signals.add(SIGTERM);
    m_signals.add(SIGINT);
    m_signals.add(SIGPIPE);
    m_signals.add(SIGHUP);

    m_signals.async_wait([this](const std::error_code& error , int signal_number ){
        handle_signals(error, signal_number);
    });

    //网络客户端启动
    m_audit_client->start();

    //网络服务端启动
    m_server->start();

    //模块流程初始化
    //Filter::instance().init();

    m_service.run();
}

bool send_adt_msg(std::unique_ptr<RequestAudit> &req){
    Application::instance().handle_to_audit_request(std::move(req));
    return true;
}

bool send_rsp_msg(std::unique_ptr<ResponseDeliver> &&rsp){

    if(auto c = rsp->get_owner().lock()){
        Application::instance().handle_deliver_msg(*rsp);
        Application::instance().handle_write_msg(c, std::move(rsp));
        return true;
    }

    return false;
}