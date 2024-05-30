#pragma once

#define IN
#define OUT
//

#include <vector>


#include <obs.hpp>


class AFSmallModelHotkey final
{
public:
	AFSmallModelHotkey() = default;
	~AFSmallModelHotkey() = default;

	obs_key_combination_t			m_OriginalKey;
	obs_key_combination_t			m_TempKey;

	bool							m_bIsChanged = false;
};



// Def Type
typedef std::vector<obs_key_combination_t> tVEC_KEY_COMBI;
typedef std::vector<AFSmallModelHotkey> tVEC_SMALL_MODEL_KEY;



class AFModelHotkey final
{
#pragma region QT Field, CTOR/DTOR
public:
	AFModelHotkey(obs_hotkey_id id);
	~AFModelHotkey() = default;
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
	bool							LoadBindingsKey(IN OUT tVEC_KEY_COMBI& bindings, obs_hotkey_id id_);
	void							UpdateBindingsKey(IN OUT tVEC_KEY_COMBI& bindings);
	void							ClearAndGetKeys(IN OUT tVEC_KEY_COMBI& bindings, IN tVEC_SMALL_MODEL_KEY vecKeys) const;
#pragma endregion public func

#pragma region private func
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
private:
	bool							m_IsIgnoreChangedBindings = false;
	obs_hotkey_id					m_KeyID;
#pragma endregion private member var
};
