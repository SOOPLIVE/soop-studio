#pragma once

#include <obs.hpp>
#include <util/util.hpp>

class AFServiceManager final
{
public:
    AFServiceManager() {}
    ~AFServiceManager() { service = nullptr; }

public:
    bool InitService();
    void SaveService();
    bool LoadService();
    obs_service_t* GetService();
    void SetService(obs_service_t* newService);

private:
    OBSService service;
};