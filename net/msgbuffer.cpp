//
// Created by ucbzh on 2022/4/18.
//

#include "msgbuffer.h"
#include <arpa/inet.h>
#include <cstring>
#include <iterator>
#include <iostream>

void MsgBuffer::append(const MsgBuffer& msg){
    adjust(available());
    std::copy(msg.m_buffer.data() + msg.m_head, msg.m_buffer.data() + msg.m_tail, data());
    m_tail += msg.available();
}

void MsgBuffer::append(const char* buf, size_t size){
    if(size == 0){
        return;
    }
    adjust(size);
    std::copy(buf, buf + size, data());
    commit(size);
}

void MsgBuffer::adjust(std::size_t size){
    if(m_buffer.size() - m_tail >= size){
        return;
    }

    //if(m_head + (m_buffer.size() - m_tail) >= size){
        ;
    //}
    //else{
    //    std::size_t length = (m_buffer.size() * 2) >= (available() + size) ? (m_buffer.size() * 2) : (available() + size);
    //    m_buffer.resize(length);
    //}

    std::size_t length = (m_buffer.size() * 2) >= (m_buffer.size() + size) ? (m_buffer.size() * 2) : m_buffer.size() + size;
    m_buffer.resize(length);

    //std::copy(m_buffer.data() + m_head, m_buffer.data() + m_tail, m_buffer.data());
    //m_tail = m_tail - m_head;
    //m_head = 0;
    return;
}

MsgBuffer& MsgBuffer::operator <<(std::uint8_t v){
    append(reinterpret_cast<const char*>(&v), 1);
    return *this;
}

MsgBuffer& MsgBuffer::operator <<(std::uint16_t v){
    uint16_t sv = htons(v);
    append(reinterpret_cast<const char *>(&sv), 2);
    return *this;
}

MsgBuffer& MsgBuffer::operator <<(std::uint32_t v){
    uint32_t iv = htonl(v);
    append(reinterpret_cast<const char *>(&iv), 4);
    return *this;
}

MsgBuffer& MsgBuffer::operator << (const std::string &data){
    append(data.data(), data.size() + 1);
    return *this;
}

void MsgBuffer::get(char* buf, size_t n){
    if(n > available()){
        return;
    }
    std::copy(memory(), memory()+n, buf);
}

MsgBuffer& MsgBuffer::operator >>(std::uint8_t &v){
    fetch(0, sizeof(std::uint8_t), [this, &v](const char* buf, std::size_t size){
        v = buf[0];
    });
    consume(sizeof(std::uint8_t));
    return *this;
}

MsgBuffer& MsgBuffer::operator >>(std::uint16_t &v){
    fetch(0, sizeof(std::uint16_t), [this, &v](const char* buf, std::size_t size){
        v = ntohs(*(reinterpret_cast<const std::uint16_t*>(buf)));
    });
    consume(sizeof(std::uint16_t));
    return *this;
}

MsgBuffer& MsgBuffer::operator >>(std::uint32_t &v){
    fetch(0, sizeof(std::uint32_t), [this, &v](const char* buf, std::size_t size){
        v = ntohl(*(reinterpret_cast<const std::uint32_t*>(buf)));
    });
    consume(sizeof(std::uint32_t));
    return *this;
}

MsgBuffer& MsgBuffer::operator >> (std::string &data){
    std::size_t length = strlen(memory());
    std::copy(memory(), memory()+length, std::back_insert_iterator(data));
    consume(length+1);
    return *this;
}

void MsgBuffer::peek(std::uint8_t &v){
    v = memory()[0];
}

void MsgBuffer::peek(std::uint16_t &v){
    v = ntohs(*(reinterpret_cast<std::uint16_t*>(memory())));
}

void MsgBuffer::peek(std::uint32_t &v){
    v = ntohl(*(reinterpret_cast<std::uint32_t*>(memory())));
}

MsgBuffer& MsgBuffer::reset(std::size_t offset, std::uint8_t &v){
    std::copy(reinterpret_cast<char*>(&v), (reinterpret_cast<char*>(&v)) + sizeof(v), memory()+offset);
    return *this;
}

MsgBuffer& MsgBuffer::reset(std::size_t offset, std::uint16_t &v){
    uint16_t sv = htons(v);
    std::copy(reinterpret_cast<char*>(&sv), (reinterpret_cast<char*>(&sv)) + sizeof(sv), memory()+offset);
    return *this;
}

MsgBuffer& MsgBuffer::reset(std::size_t offset, std::uint32_t &v){
    uint32_t lv = htonl(v);
    std::copy(reinterpret_cast<char*>(&lv), (reinterpret_cast<char*>(&lv)) + sizeof(lv), memory()+offset);
    return *this;
}

MsgBuffer& MsgBuffer::reset(std::size_t offset, std::string &data){
    std::copy(data.data(), data.data()+data.size()+1, memory()+offset);
    return *this;
}