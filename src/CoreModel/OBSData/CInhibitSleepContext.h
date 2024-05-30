#pragma once

#include <util/platform.h>

class AFInhibitSleepContext final
{
#pragma region QT Field, CTOR/DTOR
public:
    static AFInhibitSleepContext& GetSingletonInstance()
    {
        static AFInhibitSleepContext* instance = nullptr;
        if (instance == nullptr)
            instance = new AFInhibitSleepContext;
        return *instance;
    };
    ~AFInhibitSleepContext();
private:
    // singleton constructor
    AFInhibitSleepContext() = default;
    AFInhibitSleepContext(const AFInhibitSleepContext&) = delete;
    AFInhibitSleepContext(/* rValue */AFInhibitSleepContext&& other) noexcept = delete;
    AFInhibitSleepContext& operator=(const AFInhibitSleepContext&) = delete;
    //
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void                                InitContext();
    void                                FinContext();


    inline void                         IncrementSleepInhibition();
    inline void                         DecrementSleepInhibition();


#pragma endregion public func

#pragma region private func
#pragma endregion private func
#pragma region public member var

#pragma endregion public member var
#pragma region private member var
private:
    os_inhibit_t*                       m_SleepInhibitor = nullptr;
    int                                 m_iSleepInhibitRefs = 0;
#pragma endregion private member var
};


inline void AFInhibitSleepContext::IncrementSleepInhibition()
{
    if (!m_SleepInhibitor)
        return;
    if (m_iSleepInhibitRefs++ == 0)
        os_inhibit_sleep_set_active(m_SleepInhibitor, true);
};

inline void AFInhibitSleepContext::DecrementSleepInhibition()
{
    if (!m_SleepInhibitor)
        return;
    if (m_iSleepInhibitRefs == 0)
        return;
    if (--m_iSleepInhibitRefs == 0)
        os_inhibit_sleep_set_active(m_SleepInhibitor, false);
};

