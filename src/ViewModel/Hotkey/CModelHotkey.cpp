#include "CModelHotkey.h"

AFModelHotkey::AFModelHotkey(obs_hotkey_id id)
	:m_KeyID(id) {}

bool AFModelHotkey::LoadBindingsKey(IN OUT tVEC_KEY_COMBI& bindings, obs_hotkey_id id_)
{
	bool res = false;

	if (m_IsIgnoreChangedBindings || m_KeyID != id_)
		return res;


	auto LoadBindings = [&](obs_hotkey_binding_t* binding) {
		if (obs_hotkey_binding_get_hotkey_id(binding) != m_KeyID)
			return;

		auto get_combo = obs_hotkey_binding_get_key_combination;
		bindings.push_back(get_combo(binding));
	};
	using LoadBindings_t = decltype(&LoadBindings);

	obs_enum_hotkey_bindings(
		[](void* data, size_t, obs_hotkey_binding_t* binding) {
			auto LoadBindings = *static_cast<LoadBindings_t>(data);
			LoadBindings(binding);
			return true;
		},
		static_cast<void*>(&LoadBindings));


	res = true;

	return res;
}

void AFModelHotkey::UpdateBindingsKey(IN OUT tVEC_KEY_COMBI& bindings)
{
	auto AtomicUpdate = [&]() {
		m_IsIgnoreChangedBindings = true;

		obs_hotkey_load_bindings(m_KeyID, bindings.data(),
								 bindings.size());

		m_IsIgnoreChangedBindings = false;
		};
	using AtomicUpdate_t = decltype(&AtomicUpdate);

	obs_hotkey_update_atomic(
		[](void* d) { (*static_cast<AtomicUpdate_t>(d))(); },
		static_cast<void*>(&AtomicUpdate));
}

void AFModelHotkey::ClearAndGetKeys(IN OUT tVEC_KEY_COMBI& bindings, IN tVEC_SMALL_MODEL_KEY vecKeys) const
{
	bindings.clear();
	for (auto& edit : vecKeys)
		if (!obs_key_combination_is_empty(edit.m_TempKey))
			bindings.emplace_back(edit.m_TempKey);
}
