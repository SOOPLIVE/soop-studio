#pragma once


#include <memory>

#include <util/profiler.hpp>




static auto SnapshotRelease = [](profiler_snapshot_t* snap) {
	profile_snapshot_free(snap);
};

using ProfilerSnapshot =
	std::unique_ptr<profiler_snapshot_t, decltype(SnapshotRelease)>;



class AFOBSProfiler final
{
#pragma region QT Field, CTOR/DTOR
public:
	AFOBSProfiler() {};
	~AFOBSProfiler();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
	void							StartProfiler();
	void							StopProfiler();
    
    profiler_name_store_t*          GetGetProfilerNameStore() const { return m_pNameStore; };
#pragma endregion public func

#pragma region private func
private:
	ProfilerSnapshot				_GetSnapshot();
	void							_ProfilerFree(void*);
	void							_SaveProfilerData(const ProfilerSnapshot& snap);


#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
private:
	ScopeProfiler*					m_pMainScope = nullptr;
	profiler_name_store_t*			m_pNameStore = nullptr;
#pragma endregion private member var
};
