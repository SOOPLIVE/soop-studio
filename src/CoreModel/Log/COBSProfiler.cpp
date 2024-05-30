#include "COBSProfiler.h"

#include <sstream>


#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Log/CLogManager.h"



static const char* run_program_init = "run_program_init";


AFOBSProfiler::~AFOBSProfiler()
{
	if (m_pMainScope != nullptr)
		delete m_pMainScope;

	_ProfilerFree(nullptr);

	if (m_pNameStore != nullptr)
		profiler_name_store_free(m_pNameStore);
}

void AFOBSProfiler::StartProfiler()
{
	m_pNameStore = profiler_name_store_create();

	profiler_start();
	profile_register_root(run_program_init, 0);

	m_pMainScope = new ScopeProfiler(run_program_init);
}

void AFOBSProfiler::StopProfiler()
{
	if (m_pMainScope != nullptr)
		m_pMainScope->Stop();
}

ProfilerSnapshot AFOBSProfiler::_GetSnapshot()
{
	return ProfilerSnapshot{ profile_snapshot_create(), SnapshotRelease };
}

void AFOBSProfiler::_ProfilerFree(void*)
{
	profiler_stop();

	auto snap = _GetSnapshot();

	profiler_print(snap.get());
	profiler_print_time_between_calls(snap.get());

	_SaveProfilerData(snap);

	profiler_free();
}

void AFOBSProfiler::_SaveProfilerData(const ProfilerSnapshot& snap)
{
	std::string& tmpCurrLogfile = AFLogManager::GetSingletonInstance().GetStrCurrentLogFile();

	if (tmpCurrLogfile.empty())
		return;

	auto pos = tmpCurrLogfile.rfind('.');
	if (pos == tmpCurrLogfile.npos)
		return;

#define LITERAL_SIZE(x) x, (sizeof(x) - 1)
	std::ostringstream dst;
	dst.write(LITERAL_SIZE("ANENTAStudio/profiler_data/"));
	dst.write(tmpCurrLogfile.c_str(), pos);
	dst.write(LITERAL_SIZE(".csv.gz"));
#undef LITERAL_SIZE

	BPtr<char> path = AFConfigManager::GetSingletonInstance().
					  GetConfigPathPtr(dst.str().c_str());
	if (!profiler_snapshot_dump_csv_gz(snap.get(), path))
		blog(LOG_WARNING, "Could not save profiler data to '%s'",
			static_cast<const char*>(path));
}

