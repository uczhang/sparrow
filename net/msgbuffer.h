//
// Created by ucbzh on 2022/4/17.
//

#ifndef PANDA_MSGBUFFER_H
#define PANDA_MSGBUFFER_H
#include <vector>
#include <functional>
#include <memory>
#include <cstdint>
class MsgBuffer final{
public:
    enum BufferStat{
        none,
        valid,
        invalid
    };

    explicit MsgBuffer(std::size_t size = 1024): m_packlen(0),m_head(0),m_tail(0),m_indicator(none){
        m_buffer.resize(size);
    }
    MsgBuffer(MsgBuffer&& buffer){
        m_buffer = std::move(buffer.m_buffer);
        m_tail = buffer.m_tail;
        m_head = buffer.m_head;
        m_packlen = buffer.m_packlen;
    }

    MsgBuffer(const MsgBuffer&) = default;
    MsgBuffer& operator=(const MsgBuffer&) = default;

    std::size_t size(){
        return m_buffer.size();
    }

    char* data(){
        return &m_buffer[m_tail];
    }

    std::size_t surplus(){
        return m_buffer.size() - m_tail;
    }

    void commit(std::size_t size){
        m_tail += size;
    }

    void consume(std::size_t size){
        m_head += size;
    }

    void vomit(std::size_t size){
        if(size > m_head){
            return;
        }
        m_head -= size;
    }

    void seek(std::size_t size){
        m_head += size;
        m_tail += size;
    }

    void clear(){
        m_head = m_tail = m_packlen = 0;
        m_indicator = none;
        m_buffer.resize(m_default_buffer_size);
        std::fill(m_buffer.begin(), m_buffer.end(), '\0');
    }

    MsgBuffer& operator <<(std::uint8_t v);
    MsgBuffer& operator <<(std::uint16_t v);
    MsgBuffer& operator <<(std::uint32_t v);
    MsgBuffer& operator << (const std::string &data);
    void get(char* buf, size_t n);
    template<int N> void get(char (&buf)[N]){
        get(buf, N);
    }

    MsgBuffer& operator >>(std::uint8_t &v);
    MsgBuffer& operator >>(std::uint16_t &v);
    MsgBuffer& operator >>(std::uint32_t &v);
    MsgBuffer& operator >> (std::string &data);

    void peek(std::uint8_t &v);
    void peek(std::uint16_t &v);
    void peek(std::uint32_t &v);

    std::size_t available() const{
        return m_tail - m_head;
    }

    bool empty() const{
        return m_head == m_tail;
    }
    char* memory() {
        return &m_buffer[m_head];
    }

    std::size_t get_packlen(){
        return m_packlen;
    }

    void set_packlen(std::size_t packlen){
        m_packlen = packlen;
    }
    void update_packlen(std::size_t packlen){
        m_packlen += packlen;
    }

    void append(const MsgBuffer& msg);
    template<int N> void append(const char (&buf)[N]){
        append(buf, N);
    }
    void append(const char* buf, size_t n);
    void adjust(std::size_t size);

    void fetch(std::size_t offset, std::size_t count, std::function<void(const char*, std::size_t)> handler){
        std::size_t length = offset + count;
        if(offset > m_tail || length > size()){
            return;
        }
        handler(memory() + offset, count);
    }

    void reset(std::size_t offset, std::size_t count, std::function<void(char*, std::size_t)> handler){
        std::size_t length = offset + count;
        if(offset > m_tail || length > size()){
            return;
        }
        handler(memory() + offset, count);
    }

    void reset_code(MsgBuffer& msg){
        std::copy(msg.memory(), msg.data(), memory());
    }

    MsgBuffer& reset(std::size_t offset, std::uint8_t &v);
    MsgBuffer& reset(std::size_t offset, std::uint16_t &v);
    MsgBuffer& reset(std::size_t offset, std::uint32_t &v);
    MsgBuffer& reset(std::size_t offset, std::string &data);


    void reset(std::size_t offset, const char* buf, size_t size){
        if(size == 0){
            return;
        }
        std::copy(buf, buf + size, memory()+offset);
    }

    template<int N> void reset(std::size_t offset, const char (&buf)[N]){
        reset(offset, buf, N);
    }

    BufferStat get_indicator(){
        return m_indicator;
    }
    void set_indicator(BufferStat i){
        m_indicator = i;
    }

    std::size_t get_mhead(){
        return m_head;
    }
    std::size_t get_mtail(){
        return m_tail;
    }
private:
    std::vector<char> m_buffer;
    std::size_t m_packlen;
    std::size_t m_head;
    std::size_t m_tail;
    BufferStat m_indicator;
    inline static constexpr size_t m_default_buffer_size = 1024;
};
#endif //PANDA_MSGBUFFER_H
