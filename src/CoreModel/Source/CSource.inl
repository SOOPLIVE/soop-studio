#pragma once

#include <vector>
#include <string>

#include "util/util.hpp"



inline OBSScene AFSourceUtil::CnvtToOBSScene(OBSSource source)
{
	return OBSScene(obs_scene_from_source(source));
};

inline bool AFSourceUtil::RemoveSimpleCallback(void*, obs_source_t* pSrc)
{
	obs_source_remove(pSrc);
	return true;
};

inline bool AFSourceUtil::GetNameSimpleCallback(void* paramVecStr, obs_source_t* pSrc)
{
	auto vecNames = static_cast<std::vector<std::string> *>(paramVecStr);
	vecNames->push_back(obs_source_get_name(pSrc));
	return true;
};
