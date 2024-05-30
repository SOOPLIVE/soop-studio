#pragma once

#include <mutex>

template <typename T>
class AFSafeSingleton
{
#pragma region QT Field, CTOR/DTOR
public:
    static T& GetInstance()
    {
        std::call_once(s_onceFlag, [] {
            s_instance = std::make_unique<T>();
        });

        return *s_instance;
    }
    
    static T* UnSafeGetInstace() { return s_instance.get(); };
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
#pragma endregion public func

#pragma region private func
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
private:
    static std::unique_ptr<T> s_instance;
    static std::once_flag s_onceFlag;
#pragma endregion private member var

};

template <typename T>
std::unique_ptr<T> AFSafeSingleton<T>::s_instance = nullptr;

template <typename T>
std::once_flag AFSafeSingleton<T>::s_onceFlag;