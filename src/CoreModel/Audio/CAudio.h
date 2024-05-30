#pragma once

#include <obs.hpp>
#include <util/util.hpp>

class AFAudioUtil final
{
#pragma region QT Field, CTOR/DTOR
public:
	AFAudioUtil();
	~AFAudioUtil();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
	void							CreateFirstRunSources();
	int								ResetAudio();
	void							ResetAudioDevice(const char* sourceId, const char* deviceId, const char* deviceDesc, int channel);
	void							LoadAudioMonitoring();
#pragma endregion public func

#pragma region private func
private:
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
#pragma endregion private member var
};

#include "CAudio.inl"
extern char* get_new_source_name(const char* name, const char* format);
