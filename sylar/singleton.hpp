#ifndef __SYLAR_SINGLETON_H__
#define __SYLAR_SINGLETON_H__

namespace  sylar{

template<typename T>
class Singleton{
public:
    static T* GetInstance(){
        static T v;
        return &v;
    }
};

}

#endif