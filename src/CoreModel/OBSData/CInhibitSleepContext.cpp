#include "CInhibitSleepContext.h"




AFInhibitSleepContext::~AFInhibitSleepContext()
{
	FinContext();
}

void AFInhibitSleepContext::InitContext()
{
	if (m_SleepInhibitor == nullptr)
		m_SleepInhibitor = os_inhibit_sleep_create("SOOP,OBS Video/audio");
}

void AFInhibitSleepContext::FinContext()
{
	if (m_SleepInhibitor != nullptr)
	{
		os_inhibit_sleep_set_active(m_SleepInhibitor, false);
		os_inhibit_sleep_destroy(m_SleepInhibitor);

		m_SleepInhibitor = nullptr;
	}
}
