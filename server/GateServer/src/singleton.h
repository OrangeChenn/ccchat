#ifndef __SINGLETON_H__
#define __SINGLETON_H__

#include <memory>

template <class T>
class Singleton {
protected:
    Singleton() = default;
    static std::shared_ptr<T> s_instance;

private:
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator=(const Singleton<T>&) = delete;

public:
    static std::shared_ptr<T> GetInstance() {
        static std::once_flag s_flag;
        std::call_once(s_flag, [&]() {
            s_instance = std::shared_ptr<T>(new T);
        });
        return s_instance;
    }

    void PrintAddress() {
        std::cout << s_instance.get() << std::endl;
    }

    ~Singleton() {
        std::cout << "this is sigleton destruct" << std::endl;
    }
};

template <typename T>
std::shared_ptr<T> Singleton<T>::s_instance = nullptr;

#endif // __SINGLETON_H__