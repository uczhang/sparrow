//
// Created by ucbzh on 2022/5/26.
//

#ifndef CHAMELEON_FIELD_H
#define CHAMELEON_FIELD_H

#include <algorithm>
#include <string>


class Field final {
public:

    enum DataTypes {
        DB_TYPE_UNKNOWN = 0x00,
        DB_TYPE_STRING = 0x01,
        DB_TYPE_INTEGER = 0x02,
        DB_TYPE_FLOAT = 0x03,
        DB_TYPE_BOOL = 0x04
    };

    Field();

    Field(Field &f);

    Field(const char *value, enum DataTypes type);

    enum DataTypes get_type() const { return m_field_type; }

    const std::string get_string() const { return m_field_value; }

    float get_float() const { return static_cast<float>(atof(m_field_value.c_str())); }

    bool get_bool() const { return atoi(m_field_value.c_str()) > 0; }

    int32_t get_int32() const { return static_cast<int32_t>(atol(m_field_value.c_str())); }

    uint8_t get_uint8() const { return static_cast<uint8_t>(atol(m_field_value.c_str())); }

    uint16_t get_uint16() const { return static_cast<uint16_t>(atol(m_field_value.c_str())); }

    int16_t get_int16() const { return static_cast<int16_t>(atol(m_field_value.c_str())); }

    uint32_t get_uint32() const { return static_cast<uint32_t>(atol(m_field_value.c_str())); }

    uint64_t get_uint64() const {
        uint64_t value = 0;
        value = atoll(m_field_value.c_str());
        return value;
    }

    void set_type(enum DataTypes type) { m_field_type = type; }

    void set_value(const char *value);

    void set_value(const char *value, size_t uLen);

    void set_name(const std::string &strName) {
        m_field_name = strName;
        toLowerString(m_field_name);
    }

    const std::string &get_name() { return m_field_name; }

    bool is_null() { return m_bnull; }

    template<typename T>
    void convert_value(T &value);

private:
    inline void toLowerString(std::string &str) {
        for (size_t i = 0; i < str.size(); i++) {
            if (str[i] >= 'A' && str[i] <= 'Z') {
                str[i] = str[i] + ('a' - 'A');
            }
        }
    }

public:
    bool m_bnull;

private:
    std::string m_field_value;
    std::string m_field_name;
    enum DataTypes m_field_type;
};

#endif //CHAMELEON_FIELD_H
