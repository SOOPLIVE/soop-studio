#pragma once

#include <obs.hpp>
#include <util/util.hpp>


class AFVideoUtil final
{
#pragma region QT Field, CTOR/DTOR
public:
    AFVideoUtil(){};
    ~AFVideoUtil(){};
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
	int								ResetVideo();
#pragma endregion public func

#pragma region private func
private:
	inline enum obs_scale_type		_GetScaleType(config_t* basicConfig);
	inline enum video_format		_GetVideoFormatFromName(const char* name);
	inline enum video_colorspace	_GetVideoColorSpaceFromName(const char* name);

	void							_GetFPSCommon(uint32_t& num, uint32_t& den) const;
	void							_GetFPSInteger(uint32_t& num, uint32_t& den) const;
	void							_GetFPSFraction(uint32_t& num, uint32_t& den) const;
	void							_GetFPSNanoseconds(uint32_t& num, uint32_t& den) const;
	void							_GetConfigFPS(uint32_t& num, uint32_t& den) const;
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
#pragma endregion private member var
};


#include "CVideo.inl"
