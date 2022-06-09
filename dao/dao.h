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
    Dao():m_conn(LinkPool<Mysql>::instance().get_link()){
        if(!m_conn.get()){
            std::cout << "link is nullptr" << std::endl;
            LOG_WARN << "db get connection failed";
        }
    }

    bool is_open(){
        return m_conn.get() != nullptr;
    }

    template<typename T> auto query(const std::string& sql){
        return m_conn.get()->template query<T>(sql);
    }

    //non query sql, such as update, delete, insert
    bool execute(const std::string& sql) {
        if (!m_conn.get())
            return false;

        bool r = m_conn.get()->execute(sql);
        if (!r) {
            LOG_WARN << "insert role failed";
            return false;
        }
        return r;
    }

    bool begin() {
        if(!m_conn.get())
            return false;
        return m_conn.get()->begin();
    }

    bool rollback() {
        if(!m_conn.get())
            return false;
        return m_conn.get()->rollback();
    }

    bool commit() {
        if(!m_conn.get())
            return false;
        return m_conn.get()->commit();
    }

    static void init(int max_conns, const std::string &ip, const std::string &usr, const std::string &pwd, int port = 3306, const char* db_name = nullptr, int timeout = 1) {
        LinkPool<Mysql>::instance().init(max_conns, ip.c_str(), usr.c_str(), pwd.c_str(), db_name, port, timeout);
    }

private:
    Dao(const Dao&) = delete;
    Dao& operator= (const Dao&) = delete;

    LinkGuard<Mysql> m_conn;
};

#endif //CHAMELEON_DAO_H
