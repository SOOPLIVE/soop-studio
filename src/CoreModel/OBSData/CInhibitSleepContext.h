#pragma once

#include <util/platform.h>

class AFInhibitSleepContext final
{
public:
    AFInhibitSleepContext();
    ~AFInhibitSleepContext();

public:
    void IncrementSleepInhibition();
    void DecrementSleepInhibition();

private:
    os_inhibit_t* m_pSleepInhibitor = nullptr;
    int m_sleepInhibitRefs = 0;
};