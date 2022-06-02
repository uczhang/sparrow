//
// Created by ucbzh on 2022/5/26.
//
#include "field.h"

Field::Field() : m_field_type(DB_TYPE_UNKNOWN){
    m_bnull = false;
}

Field::Field(Field &f){
    m_field_value = f.m_field_value;
    m_field_name = f.m_field_name;
    m_field_type = f.get_type();
}

Field::Field(const char *value, enum Field::DataTypes type) : m_field_type(type){
    m_field_value = value;
}

void Field::set_value(const char *value){
    m_field_value.assign(value);
}


