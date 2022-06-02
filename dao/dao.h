//
// Created by ucbzh on 2022/5/26.
//

#ifndef CHAMELEON_DAO_H
#define CHAMELEON_DAO_H
#include "mysql_db.h"
#include "db_link_pool.h"
#include "nanolog.hpp"

class Dao final {
public:
    Dao():m_conn(LinkPool<Mysql>::instance().get_link()), m_guard(m_conn)
    {
        if(!m_conn){
            LOG_WARN << "db get connection failed";
        }
    }

    bool is_open() const {
        return m_conn != nullptr;
    }

    template<typename T> auto query(const std::string& sql){
        return m_conn->template query<T>(sql);
    }

    //non query sql, such as update, delete, insert
    bool execute(const std::string& sql) {
        if (!m_conn)
            return false;

        bool r = m_conn->execute(sql);
        if (!r) {
            LOG_WARN << "insert role failed";
            return false;
        }
        return r;
    }

    bool begin() {
        if(!m_conn)
            return false;
        return m_conn->begin();
    }

    bool rollback() {
        if(!m_conn)
            return false;
        return m_conn->rollback();
    }

    bool commit() {
        if(!m_conn)
            return false;
        return m_conn->commit();
    }

    static void init(int max_conns, const std::string &ip, const std::string &usr, const std::string &pwd, int port = 3306, const char* db_name = nullptr, int timeout = 1) {
        LinkPool<Mysql>::instance().init(max_conns, ip.c_str(), usr.c_str(), pwd.c_str(), db_name, port, timeout);
    }

private:
    Dao(const Dao&) = delete;
    Dao& operator= (const Dao&) = delete;

    std::shared_ptr<Mysql> m_conn;
    LinkGuard<Mysql> m_guard;
};

#endif //CHAMELEON_DAO_H
