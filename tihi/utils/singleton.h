#ifndef TIHI_UTILS_SINGLETON_H
#define TIHI_UTILS_SINGLETON_H

#include <memory>

namespace tihi {

template<typename T>
class Singleton {
public:
    static T* GetInstance() {
        static T v;
        return &v;
    }
};

template<typename T>
class SingletonPtr {
public:
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};

}

#endif // TIHI_UTILS_SINGLETON_H