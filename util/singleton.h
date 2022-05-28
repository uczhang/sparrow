//
// Created by xiaohu on 5/9/22.
//

#ifndef PANDA_SINGLETON_H
#define PANDA_SINGLETON_H

template<typename T> class Singleton{
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    virtual ~Singleton(){
    }
    static T& instance(){
        static T _instance;
        return _instance;
    }
protected:
    Singleton(){
    }
};

#endif //PANDA_SINGLETON_H
