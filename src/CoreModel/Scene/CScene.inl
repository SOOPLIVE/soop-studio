#pragma once

inline OBSSource AFSceneUtil::CnvtToOBSSource(OBSScene scene)
{
	return OBSSource(obs_scene_get_source(scene));
};


inline OBSScene	AFSceneUtil::CnvtToOBSScene(OBSSource source)
{
	return OBSScene(obs_scene_from_source(source));
}