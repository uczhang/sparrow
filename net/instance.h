//
// Created by xiaohu on 5/9/22.
//

#ifndef PANDA_INSTANCE_H
#define PANDA_INSTANCE_H
class Instance{
public:
    Instance() = default;
    virtual void release(std::size_t id) = 0;
    virtual ~Instance(){
    }
};
#endif //PANDA_INSTANCE_H
