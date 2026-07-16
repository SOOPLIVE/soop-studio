#include "CInhibitSleepContext.h"

AFInhibitSleepContext::AFInhibitSleepContext()
    :m_pSleepInhibitor(os_inhibit_sleep_create("SOOP,OBS Video/audio"))
{}
AFInhibitSleepContext::~AFInhibitSleepContext()
{
	if(m_pSleepInhibitor != nullptr)
	{
		os_inhibit_sleep_set_active(m_pSleepInhibitor, false);
		os_inhibit_sleep_destroy(m_pSleepInhibitor);

		m_pSleepInhibitor = nullptr;
	}
}
//
void AFInhibitSleepContext::IncrementSleepInhibition()
{
    if(m_sleepInhibitRefs++ == 0)
        os_inhibit_sleep_set_active(m_pSleepInhibitor, true);
};
void AFInhibitSleepContext::DecrementSleepInhibition()
{
    if(m_sleepInhibitRefs == 0)
        return;
    if(--m_sleepInhibitRefs == 0)
        os_inhibit_sleep_set_active(m_pSleepInhibitor, false);
};