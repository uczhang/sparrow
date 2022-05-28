//
// Created by xiaohu on 3/23/22.
//

#ifndef PANDA_REPEATTIMER_H
#define PANDA_REPEATTIMER_H
#include <asio.hpp>
#include <functional>
#include <atomic>
#include <chrono>

class RepeatTimer final {
public:
    RepeatTimer(asio::io_context &ioc, std::uint64_t timeout, TimeHandler &&handler = nullptr):m_ioc(ioc),m_timer(m_ioc){
        m_handler = std::move(handler);
        m_timeout = timeout;
    }

    RepeatTimer(const RepeatTimer& ) = delete;
    RepeatTimer& operator=(const RepeatTimer&) = delete;
    void start(){
        m_running.store(true);
        expired(m_timeout);
    }

    void stop(){
        m_running.store(false);
    }

    void set_timeout(std::uint64_t timeout){
        m_timeout = timeout;
    }

    void set_handler(TimeHandler && handler){
        if(m_handler == nullptr){
            m_handler = std::move(handler);
        }
    }

private:
    void expired(std::uint64_t time_ms){
        if(!m_running.load()){
            return;
        }
        m_timer.expires_from_now(std::chrono::milliseconds(time_ms));
        m_timer.async_wait([this, time_ms](const std::error_code &ec){
           if(ec.value() == asio::error::operation_aborted || !m_running.load()){
               return;
           }
            m_handler();
           expired(time_ms);
        });
    }

private:
    std::atomic<bool> m_running{false};
    asio::io_context &m_ioc;
    asio::steady_timer m_timer;
    TimeHandler m_handler{nullptr};
    std::uint64_t m_timeout;
};
#endif //PANDA_REPEATTIMER_H
