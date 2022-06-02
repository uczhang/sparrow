//
// Created by ucbzh on 2022/5/26.
//

#ifndef CHAMELEON_MYSQL_DB_H
#define CHAMELEON_MYSQL_DB_H
#include <stdint.h>
#include <memory>
#include <string>
#include <chrono>
#include "mysql.h"
#include "dao/field.h"
#include "errmsg.h"

class Mysql final {

public:
    Mysql() = default;

    ~Mysql();

    void use_matched_rows(){ m_use_matched = 1;}
    std::string get_err_msg(){ return m_db_err_msg;}
    int get_errno(){ return m_db_err;}
    template<typename... Args> bool open(Args... args) {
        int ret = connect(std::forward<Args>(args)...);
        if(ret != 0 && (get_errno() == CR_SERVER_GONE_ERROR || get_errno() == CR_SERVER_LOST)){
            ret = connect(std::forward<Args>(args)...);
        }
        return ret;
    }
    int close();
    bool ping();
    int client_version();
    bool query_exec(const std::string& sql);
    bool begin();
    bool commit();
    bool rollback();
    int64_t affected_rows();
    const char* result_info();
    uint64_t insert_id();
    uint32_t escape_string(char* to, const char* from);
    uint32_t escape_string(char* to, const char* from, int len);
    int64_t  get_variable(const char* variable);
    bool use_result();
    int fetch_row();
    int free_result();
    unsigned  long* get_length(void){
        return mysql_fetch_lengths(m_res.get());
    }

    template<typename T> auto query(const std::string &sql){
        std::vector<T> result;

        if(!query_exec(sql)) {
            return result;
        }

        if(!use_result()){
            return result;
        }

        int numFields = mysql_num_fields(m_res.get());
        MYSQL_FIELD *fields = mysql_fetch_fields(m_res.get());
        std::vector<std::unique_ptr<Field>> cur_row;

        for(int i = 0; i < numFields; ++i){
            std::unique_ptr<Field> row =  std::unique_ptr<Field>(new Field());
            row->set_name(fields[i].name);
            row->set_type(convert_native_type(fields[i].type));
            cur_row.push_back(std::move(row));
        }

        while(fetch_row() == 0){
            for(int i = 0; i < numFields; ++i){
                if(m_row[i] == NULL)
                {
                    cur_row[i]->m_bnull = true;
                    cur_row[i]->set_value("");
                }
                else {
                    cur_row[i]->m_bnull = false;
                    cur_row[i]->set_value(m_row[i]);
                }
            }
            result.emplace_back(cur_row);
        }

        free_result();

        return result;
    }

    int execute(const std::string &sql){
        int ret = query_exec(sql);
        if(ret != 0){
            return ret;
        }

        return static_cast<int>(mysql_affected_rows(m_mysql.get()));
    }

    template<typename... Args> bool connect(Args&&... args) {
        if (m_connected) {
           return true;
        }
        m_mysql.reset(mysql_init(nullptr));
        if (m_mysql == nullptr) {
            m_db_err = mysql_errno(m_mysql.get());
            m_db_err_msg = mysql_error(m_mysql.get());
            return false;
        }

        mysql_options_set();

        static thread_local int retry = 0;
        int timeout = -1;
        auto tp = std::tuple_cat(get_tp(timeout, std::forward<Args>(args)...), std::make_tuple(nullptr, 0));
        if (std::apply(&mysql_real_connect, tp) == nullptr) {
            m_db_err = mysql_errno(m_mysql.get());
            m_db_err_msg = mysql_error(m_mysql.get());
            ++retry;
            if(m_db_err != CR_SERVER_GONE_ERROR && m_db_err != CR_SERVER_LOST) {
                return false;
            }

            if(retry < 3 && connect(args...)){
               ;
            }
            else{
                return false;
            }
        }

        m_connected = true;
        return true;
    }

    void update_operate_time(){
        m_latest_time = std::chrono::system_clock::now();
    }

    auto get_operate_time(){
        return m_latest_time;
    }

private:
    bool mysql_options_set();

    enum Field::DataTypes convert_native_type(enum_field_types mysqlType) const;
    template<typename... Args> auto get_tp(int& timeout, Args&&... args) {
        auto tp = std::make_tuple(m_mysql.get(), std::forward<Args>(args)...);
        if constexpr (sizeof...(Args) == 6) {
            auto [c, s1, s2, s3, s4, s5, i] = tp;
            timeout = i;
            return std::make_tuple(c, s1, s2, s3, s4, s5);
        }
        else
            return tp;
    }

private:
    std::unique_ptr<MYSQL_RES> m_res;
    MYSQL_ROW m_row;
    int m_res_num;
    bool m_need_free;
    bool m_connected;
    std::unique_ptr<MYSQL> m_mysql;
    std::string m_db_err_msg;
    int m_db_err;
    int m_use_matched;
    std::chrono::system_clock::time_point m_latest_time{std::chrono::system_clock::now()};
};

#endif //CHAMELEON_MYSQL_DB_H
