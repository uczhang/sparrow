//
// Created by xiaohu on 4/25/22.
//

#ifndef PANDA_PROTO_H
#define PANDA_PROTO_H
#include <cstdint>
#include <string>
#include <optional>
#include "msgbuffer.h"

class CmdType{
public:
    static constexpr std::uint32_t BIND_CMD = 0x00000001;
    static constexpr std::uint32_t BIND_CMD_RSP = 0x80000001;

    static constexpr std::uint32_t ACT_CMD = 0x00000002;
    static constexpr std::uint32_t ACT_CMD_RSP = 0x80000002;

    static constexpr std::uint32_t DELIVER_MSG_CMD = 0x0000003;
    static constexpr std::uint32_t DELIVER_MSG_CMD_RSP = 0x8000003;

    static constexpr std::uint32_t AUDIT_TOKEN_CMD =   0x00000004;
    static constexpr std::uint32_t AUDIT_RSP_TOKEN_CMD = 0x80000004;
    static constexpr std::uint32_t AUDIT_RESULT_TOKEN_CMD = 0x00000005;

};
class Head{
public:
    std::uint32_t m_len{0};
    std::uint32_t m_cmd{0};
public:
    void encode(MsgBuffer& buffer) const;
    void decode(MsgBuffer& buffer);
    void reset(MsgBuffer& buffer);
};

class DeliverBody{
public:
    std::string agent_id{};
    std::string msg_id{}; 
    std::string token_id{}; 
    std::uint32_t svc_type{}; 
    std::uint8_t content_type{}; 
    std::string timestamp{};
    std::string from{};
    std::uint32_t counts{}; 
    std::string to{};
    std::string hash{};
    std::uint32_t content_length{};
    std::string content_text{};
    std::string content_url{};
    std::string remark{};
    std::string file_name{}; 
    std::uint8_t is_review{};
    std::string session_id{}; 
    std::string kernel_id{};
    std::uint32_t  text_is_sync{};

public:
    void encode(MsgBuffer& buffer) const;
    void decode(MsgBuffer& buffer);
};

class AuditBody{
public:
    AuditBody() = default;
    AuditBody(const AuditBody&) = default;
    AuditBody& operator=(const AuditBody&) = default;
    void encode(MsgBuffer& buffer);
    std::string msg_id;
    std::string from;
    std::string mo_time;
    std::uint32_t svc_type;
    std::string to;
    std::string chat_id;
    std::string hash;
    std::uint8_t msg_type;
    std::uint32_t content_len;
    std::string content;
    std::uint8_t audit_type;
    std::string policy_id;
    std::string policy_type;
    std::string file_path;
    std::string session_id;
    std::string sys_id;
    std::uint32_t to_counts;
    std::string token_id;
    std::string content_url;
    std::string remark;
    std::string file_name;
    std::string hp_id;
    std::uint8_t report_type;
    std::uint32_t cmd;
    std::uint8_t is_review;
};

class DeliverRespBody{
public:
    std::uint32_t status{}; 
    std::string msg_id{};
    std::uint8_t msg_type{};
    std::uint32_t svc_type{};
    std::string	token_id{};
    std::string session_id{};
    std::string kernel_id{};
    std::string detail{};
    std::string inspect_id{};
    std::string esStatisticsParam{};
    std::uint32_t  text_is_sync{};
public:
    void encode(MsgBuffer& buffer) const;
    void decode(MsgBuffer& buffer);
};

class BindBody{
public:
    std::string uid;
public:
    void encode(MsgBuffer& buffer) const;
    void decode(MsgBuffer& buffer);
};

using BindRespBody = BindBody;

class Connection;
class Request{
public:
    void decode_head(MsgBuffer& buffer){
        m_head.decode(buffer);
    }
    void set_head(Head& head){
        m_head = head;
    }
    virtual void decode_body(MsgBuffer& buffer) = 0;
    virtual bool parse(MsgBuffer& buffer) = 0;
    Request(std::weak_ptr<Connection> c):m_owner(c){

    }

    virtual ~Request(){}
    auto& get_head() const{
        return m_head;
    }

    auto& get_head() {
        return m_head;
    }

    auto& get_owner() const{
        return m_owner;
    }
public:
    static std::uint16_t crc16(const char *data, int len);

private:
    Head m_head{};
    std::weak_ptr<Connection> m_owner{};
};


class RequestAct : public Request{
public:
    using Request::Request;
    virtual ~RequestAct(){};
    virtual void decode_body(MsgBuffer& buffer) override{}
    virtual bool parse(MsgBuffer& buffer) override{
        decode_head(buffer);
        return true;
    }
};

class RequestBind : public Request{
public:
    using Request::Request;
    virtual ~RequestBind(){};
    virtual void decode_body(MsgBuffer& buffer) override;
    virtual bool parse(MsgBuffer& buffer) override;

    auto& get_body() const {
        return m_body;
    }
    auto& get_body(){
        return m_body;
    }
private:
    BindBody m_body{};
};

class RequestDeliver : public Request{
public:
    using Request::Request;
    virtual ~RequestDeliver(){};
    virtual void decode_body(MsgBuffer& buffer) override;
    virtual bool parse(MsgBuffer& buffer) override;
    auto& get_body() const{
        return m_body;
    }
    auto& get_body() {
        return m_body;
    }
private:
    DeliverBody m_body{};
};

class Response{
public:
    void encode_head(MsgBuffer& buffer){
        m_head.encode(buffer);
    }
    void reset_head(MsgBuffer& buffer){
        m_head.reset(buffer);
    }
    virtual void encode_body(MsgBuffer& buffer) = 0;
    virtual bool makeup(MsgBuffer& buffer) = 0;
    Response(std::weak_ptr<Connection> c):m_owner(c){

    }
    Response(){}
    virtual ~Response(){}

    auto& get_head() const{
        return m_head;
    }
    auto& get_head() {
        return m_head;
    }

    auto& get_owner() const{
        return m_owner;
    }
private:
    Head m_head{};
    std::weak_ptr<Connection> m_owner{};
};

class ResponseAct : public Response{
public:
    using Response::Response;
    virtual void encode_body(MsgBuffer& buffer) override{}
    virtual bool makeup(MsgBuffer& buffer) override{
        encode_head(buffer);
        return true;
    }
    virtual ~ResponseAct(){}

};

class ResponseBind : public Response{
public:
    using Response::Response;
    virtual void encode_body(MsgBuffer& buffer) override;
    virtual bool makeup(MsgBuffer& buffer) override;
    virtual ~ResponseBind(){}
    auto& get_body() const{
        return m_body;
    }
    auto& get_body(){
        return m_body;
    }
private:
    BindBody  m_body;
};

class ResponseDeliver : public Response{
public:
    using Response::Response;
    virtual void encode_body(MsgBuffer& buffer) override;
    virtual bool makeup(MsgBuffer& buffer) override;
    virtual ~ResponseDeliver(){}

    auto& get_body() const{
        return m_body;
    }
    auto& get_body(){
        return m_body;
    }

private:
    DeliverRespBody m_body;
};



//服务器主动发送请求到审核服务器代理
class RequestAudit : public Response{
public:
    using Response::Response;
    virtual void encode_body(MsgBuffer& buffer) override;
    virtual bool makeup(MsgBuffer& buffer) override;
    virtual ~RequestAudit(){}

    auto& get_body() const{
        return m_body;
    }
    auto& get_body(){
        return m_body;
    }
private:
    AuditBody m_body{};
};
#endif //PANDA_PROTO_H
