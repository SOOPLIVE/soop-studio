#pragma once

#include <obs.hpp>
#include <util/util.hpp>

class AFServiceManager final
{
#pragma region QT Field, CTOR/DTOR
public:
    static AFServiceManager& GetSingletonInstance()
    {
        static AFServiceManager* instance = nullptr;
        if(instance == nullptr)
            instance = new AFServiceManager;
        return *instance;
    };
	~AFServiceManager();

private:
    // singleton constructor
    AFServiceManager() = default;
    AFServiceManager(const AFServiceManager&) = delete;
    AFServiceManager(/* rValue */AFServiceManager&& other) noexcept = delete;
    AFServiceManager& operator=(const AFServiceManager&) = delete;
    //
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    bool InitService();
    void SaveService();
    bool LoadService();
    obs_service_t* GetService();
    void SetService(obs_service_t* newService);
#pragma endregion public func

#pragma region private func
private:
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
    OBSService service = nullptr;
#pragma endregion private member var
};