//
// Created by ucbzh on 2022/5/26.
//
#include <limits.h>
#ifndef LLONG_MAX
#define LLONG_MAX    LONG_LONG_MAX
#endif
#ifndef ULLONG_MAX
#define ULLONG_MAX   ULONG_LONG_MAX
#endif
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mysql_db.h"
#include "mysqld_error.h"


Mysql::~Mysql() {
    if(m_need_free){
        free_result();
    }
    close();
}

int Mysql::client_version(){
    return mysql_get_client_version();
}

int64_t Mysql::get_variable(const char *variable) {
    int64_t  ret = -1;
    char buf[128];
    snprintf(buf, sizeof(buf), "show variables like '%s'", variable);
    if(query_exec(buf) == 0 && use_result() == 0){
        if(fetch_row() == 0 && !strcasecmp(m_row[0], variable)){
            ret = atoll(m_row[1]);
        }
        free_result();
    }
    return ret;
}

bool Mysql::mysql_options_set(){
    int timeout = -1;
    if(mysql_options(m_mysql.get(), MYSQL_OPT_CONNECT_TIMEOUT, reinterpret_cast<const char*>(&timeout)) != 0){
        m_db_err = mysql_errno(m_mysql.get());
        m_db_err_msg = mysql_error(m_mysql.get());
        return false;
    }
    if(mysql_options(m_mysql.get(), MYSQL_SET_CHARSET_NAME, "utf8") != 0){
        m_db_err = mysql_errno(m_mysql.get());
        m_db_err_msg = mysql_error(m_mysql.get());
        return false;
    }

    char value = 1;
    if(mysql_options(m_mysql.get(), MYSQL_OPT_RECONNECT, reinterpret_cast<const char*>(&value)) != 0){
        m_db_err = mysql_errno(m_mysql.get());
        m_db_err_msg = mysql_error(m_mysql.get());
        return false;
    }
    return true;
}

int Mysql::close(){
    if(m_connected){
        mysql_close(m_mysql.get());
        m_connected = false;
    }
    return 0;
}

bool Mysql::ping(){
    if(mysql_ping(m_mysql.get())!= 0){
        m_db_err = mysql_errno(m_mysql.get());
        m_db_err_msg = mysql_error(m_mysql.get());
        close();
        return false;
    }

    return true;
}

bool Mysql::query_exec(const std::string& sql) {
    if(mysql_real_query(m_mysql.get(), sql.c_str(), strlen(sql.c_str())) != 0){
        m_db_err = mysql_errno(m_mysql.get());
        m_db_err_msg = mysql_error(m_mysql.get());
        close();
        return false;
    }

    return true;
}

bool Mysql::begin() {
    return query_exec("begin");
}

bool Mysql::commit() {
    return query_exec("commit");
}

bool Mysql::rollback() {
    return query_exec("rollback");
}

int64_t Mysql::affected_rows() {
    my_ulonglong rowNum;
    rowNum = mysql_affected_rows(m_mysql.get());
    if(rowNum < 0){
        m_db_err = mysql_errno(m_mysql.get());
        m_db_err_msg= mysql_error(m_mysql.get());
        close();
        return -1;
    }
    return static_cast<int64_t>(rowNum);
}

const char* Mysql::result_info() {
    return mysql_info(m_mysql.get());
}

uint64_t Mysql::insert_id() {
    my_ulonglong id;
    id = mysql_insert_id(m_mysql.get());
    return static_cast<uint64_t>(id);
}

bool Mysql::use_result() {
    m_res.reset(mysql_store_result(m_mysql.get()));
    if(m_res == nullptr){
        if(mysql_errno(m_mysql.get()) != 0){
            m_db_err = mysql_errno(m_mysql.get());
            m_db_err_msg= mysql_error(m_mysql.get());
            close();
            return false;
        }
        else{
            m_res_num = 0;
            return false;
        }
    }

    m_res_num = mysql_num_rows(m_res.get());
    if(m_res_num < 0){
        m_db_err = mysql_errno(m_mysql.get());
        m_db_err_msg= mysql_error(m_mysql.get());
        mysql_free_result(m_res.get());
        close();
        return false;
    }

    m_need_free = true;
    return true;
}

int Mysql::fetch_row(){
    m_row = mysql_fetch_row(m_res.get());
    if(m_row == nullptr){
        m_db_err = mysql_errno(m_mysql.get());
        m_db_err_msg= mysql_error(m_mysql.get());
        mysql_free_result(m_res.get());
        close();
        return -1;
    }
    return 0;
}

int Mysql::free_result() {
    if(m_need_free){
        mysql_free_result(m_res.get());
        m_need_free = false;
    }
    return 0;
}

uint32_t Mysql::escape_string(char *to, const char *from) {
    return mysql_real_escape_string(m_mysql.get(), to, from, strlen(from));
}

uint32_t Mysql::escape_string(char *to, const char *from, int len) {
    return mysql_real_escape_string(m_mysql.get(), to, from, len);
}


enum Field::DataTypes Mysql::convert_native_type(enum_field_types mysqlType) const {
    switch (mysqlType) {
        case FIELD_TYPE_TIMESTAMP:
        case FIELD_TYPE_DATE:
        case FIELD_TYPE_TIME:
        case FIELD_TYPE_DATETIME:
        case FIELD_TYPE_YEAR:
        case FIELD_TYPE_STRING:
        case FIELD_TYPE_VAR_STRING:
        case FIELD_TYPE_BLOB:
        case FIELD_TYPE_SET:
        case FIELD_TYPE_NULL:
            return Field::DB_TYPE_STRING;

        case FIELD_TYPE_TINY:
        case FIELD_TYPE_SHORT:
        case FIELD_TYPE_LONG:
        case FIELD_TYPE_INT24:
        case FIELD_TYPE_LONGLONG:
        case FIELD_TYPE_ENUM:
            return Field::DB_TYPE_INTEGER;

        case FIELD_TYPE_DECIMAL:
        case FIELD_TYPE_FLOAT:
        case FIELD_TYPE_DOUBLE:
            return Field::DB_TYPE_FLOAT;
        default:
            return Field::DB_TYPE_UNKNOWN;
    }
}
