#pragma once

#include <obs.hpp>

class AFSceneUtil final
{
#pragma region QT Field, CTOR/DTOR
public:
	AFSceneUtil() = delete;
	~AFSceneUtil() = delete;
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
	static inline OBSSource		CnvtToOBSSource(OBSScene scene);
	static inline OBSScene		CnvtToOBSScene(OBSSource source);

	static bool					SceneItemHasVideo(obs_sceneitem_t* item);
	static inline bool			IsCropEnabled(const obs_sceneitem_crop* crop)
                                {
                                    return	crop->left > 0 || crop->top > 0 ||
                                            crop->right > 0 || crop->bottom > 0;
                                }

	static obs_source_t*		CreateOBSScene(const char* name);
	static void					TransitionToScene(OBSSource scene, bool force = false,
												  bool quickTransition = false,
												  int quickDuration = 0, bool black = false,
												  bool manual = false);

	static int					GetOverrideTransitionDuration(OBSSource source);
	static void					OverrideTransition(OBSSource transition);

	static void					SetTransition(OBSSource transition);

#pragma endregion public func

#pragma region private func
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
#pragma endregion private member var
};

class AFSceneObj
{
#pragma region QT Field, CTOR/DTOR
public:
	AFSceneObj();
	virtual ~AFSceneObj();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
#pragma endregion public func

#pragma region private func
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
#pragma endregion private member var
};


#include "CScene.inl"
