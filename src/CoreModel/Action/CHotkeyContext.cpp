#include "CHotkeyContext.h"


#include <util/profiler.hpp>


#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/OBSOutput/COBSOutputContext.h"

#include "Application/CApplication.h"
#include "MainFrame/CMainFrame.h"


obs_key AFHotkeyContext::MakeKey(int virtualKeyCode)
{
	return obs_key_from_virtual_key(virtualKeyCode);
}

void AFHotkeyContext::InjectEvent(obs_key_combination_t key, bool pressed)
{
	obs_hotkey_inject_event(key, pressed);
}

void Test()
{

}

void AFHotkeyContext::InitContext()
{
	auto& locale = AFLocaleTextManager::GetSingletonInstance();

	_InitHotkeyTranslations(&locale);

#define MAKE_CALLBACK(pred, method, log_action)								\
	[](void *data, obs_hotkey_pair_id, obs_hotkey_t *, bool pressed) {		\
		AFMainFrame& mainFrame = *static_cast<AFMainFrame *>(data);			\
		if ((pred) && pressed) {											\
			blog(LOG_INFO, log_action " due to hotkey");					\
			method();														\
			return true;													\
		}																	\
		return false;														\
	}

	AFMainFrame* mainFrameData = App()->GetMainView();

	ProfileScope("AFHotkeyContext::InitContext");
	m_StreamingHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.StartStreaming", locale.Str("Basic.Main.StartStreaming"),
		"OBSBasic.StopStreaming", locale.Str("Basic.Main.StopStreaming"),
		MAKE_CALLBACK(mainFrame.EnableStartStreaming(), mainFrame.StartStreaming, "Starting stream"),
		MAKE_CALLBACK(mainFrame.EnableStopStreaming(), mainFrame.StopStreaming, "Stoping stream"),
		mainFrameData, mainFrameData);

	_LoadHotkeyPair(m_StreamingHotkeys, 
					"OBSBasic.StartStreaming","OBSBasic.StopStreaming");


	auto cb = [](void* data, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
		AFMainFrame& mainFrame = *static_cast<AFMainFrame*>(data);
		if (mainFrame.IsStreamActive() && pressed) {
			mainFrame.ForceStopStreaming();
		}
	};

	//m_ForceStreamingStopHotkey = obs_hotkey_register_frontend(
	//	"OBSBasic.ForceStopStreaming", locale.Str("Basic.Main.ForceStopStreaming"),
	//	cb, mainFrameData);
	//_LoadHotkey(m_ForceStreamingStopHotkey, "OBSBasic.ForceStopStreaming");

	m_RecordingHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.StartRecording", locale.Str("Basic.Main.StartRecording"),
		"OBSBasic.StopRecording", locale.Str("Basic.Main.StopRecording"),
		MAKE_CALLBACK(mainFrame.EnableStartRecording(), mainFrame.StartRecording, "Starting recording"),
		MAKE_CALLBACK(mainFrame.EnableStopRecording(), mainFrame.StopRecording, "Starting recording"),
		mainFrameData, mainFrameData);
	_LoadHotkeyPair(m_RecordingHotkeys, 
					"OBSBasic.StartRecording", "OBSBasic.StopRecording");

	//m_PauseHotkeys = obs_hotkey_pair_register_frontend(
	//	"OBSBasic.PauseRecording", locale.Str("Basic.Main.PauseRecording"),
	//	"OBSBasic.UnpauseRecording", locale.Str("Basic.Main.UnpauseRecording"),
	//	MAKE_CALLBACK(mainFrame.EnablePauseRecording(), mainFrame.PauseRecording, "Pausing recording"),
	//	MAKE_CALLBACK(mainFrame.EnableUnPauseRecording(), mainFrame.UnPauseRecording, "Unpausing recording"),
	//	mainFrameData, mainFrameData);
	//_LoadHotkeyPair(m_PauseHotkeys, 
	//				"OBSBasic.PauseRecording","OBSBasic.UnpauseRecording");

	m_SplitFileHotkey = obs_hotkey_register_frontend(
		"OBSBasic.SplitFile", locale.Str("Basic.Main.SplitFile"),
		[](void*, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
			if (pressed)
				obs_frontend_recording_split_file();
		},
		this);
	_LoadHotkey(m_SplitFileHotkey, "OBSBasic.SplitFile");

	m_ReplayBufHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.StartReplayBuffer",locale.Str("Basic.Main.StartReplayBuffer"),
		"OBSBasic.StopReplayBuffer", locale.Str("Basic.Main.StopReplayBuffer"),
		MAKE_CALLBACK(!mainFrame.IsReplayBufferActive(), mainFrame.StartReplayBuffer, "Starting replay buffer"),
		MAKE_CALLBACK(mainFrame.IsReplayBufferActive(), mainFrame.StopReplayBuffer, "Stopping replay buffer"),
		mainFrameData, mainFrameData);
	_LoadHotkeyPair(m_ReplayBufHotkeys, 
					"OBSBasic.StartReplayBuffer","OBSBasic.StopReplayBuffer");

	//if (vcamEnabled) {
	//	vcamHotkeys = obs_hotkey_pair_register_frontend(
	//		"OBSBasic.StartVirtualCam",
	//		localeTextManager.Str("Basic.Main.StartVirtualCam"),
	//		"OBSBasic.StopVirtualCam",
	//		localeTextManager.Str("Basic.Main.StopVirtualCam"),
	//		MAKE_CALLBACK(!basic.outputHandler->VirtualCamActive(),
	//			basic.StartVirtualCam,
	//			"Starting virtual camera"),
	//		MAKE_CALLBACK(basic.outputHandler->VirtualCamActive(),
	//			basic.StopVirtualCam,
	//			"Stopping virtual camera"),
	//		this, this);
	//	LoadHotkeyPair(vcamHotkeys, "OBSBasic.StartVirtualCam",
	//		"OBSBasic.StopVirtualCam");
	//}

	m_TogglePreviewHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.EnablePreview",  locale.Str("Basic.Main.PreviewConextMenu.Enable"),
		"OBSBasic.DisablePreview", locale.Str("Basic.Main.Preview.Disable"),
		MAKE_CALLBACK(!mainFrame.GetPreviewEnable(), mainFrame.EnablePreview, "Enabling preview"),
		MAKE_CALLBACK(mainFrame.GetPreviewEnable(), mainFrame.DisablePreview, "Disabling preview"),
		mainFrameData, mainFrameData);
	_LoadHotkeyPair(m_TogglePreviewHotkeys, 
					"OBSBasic.EnablePreview", "OBSBasic.DisablePreview");

	m_togglePreviewProgramHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.EnablePreviewProgram", locale.Str("Basic.EnablePreviewProgramMode"),
		"OBSBasic.DisablePreviewProgram",locale.Str("Basic.DisablePreviewProgramMode"),
		MAKE_CALLBACK(!mainFrame.IsPreviewProgramMode(), mainFrame.EnablePreviewProgam, "Enabling preview"),
		MAKE_CALLBACK(mainFrame.IsPreviewProgramMode(), mainFrame.DiablePreviewProgam, "Disabling preview"),
		mainFrameData, mainFrameData);
	_LoadHotkeyPair(m_togglePreviewProgramHotkeys,
					"OBSBasic.EnablePreviewProgram",
					"OBSBasic.DisablePreviewProgram",
					"OBSBasic.TogglePreviewProgram");

#undef MAKE_CALLBACK
	//m_ContextBarHotkeys = obs_hotkey_pair_register_frontend(
	//	"OBSBasic.ShowContextBar", locale.Str("Basic.Main.ShowContextBar"),
	//	"OBSBasic.HideContextBar", locale.Str("Basic.Main.HideContextBar"),
	//	MAKE_CALLBACK(!mainFrame.IsPreviewProgramMode(), mainFrame.EnablePreviewProgam, "Showing Context Bar"),
	//	MAKE_CALLBACK(mainFrame.IsPreviewProgramMode(), mainFrame.DiablePreviewProgam, "Hiding Context Bar"),
	//	mainFrameData, mainFrameData);
	//_LoadHotkeyPair(m_ContextBarHotkeys, "OBSBasic.ShowContextBar",
	//	"OBSBasic.HideContextBar");

	auto cbTransition = [](void* data, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
		if (pressed) {
			QMetaObject::invokeMethod(static_cast<AFMainFrame*>(data),
				"qslotTransitionScene", Qt::QueuedConnection);
		}
	};

	m_TransitionHotkey = obs_hotkey_register_frontend(
		"OBSBasic.Transition", locale.Str("Transition"),
		cbTransition, mainFrameData);
	_LoadHotkey(m_TransitionHotkey, "OBSBasic.Transition");

	//auto cbResetStats = [](void* data, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
	//	AFMainFrame& mainFrame = *static_cast<AFMainFrame*>(data);
	//	if (pressed) {
	//		mainFrame.RecvHotkeyResetStats();
	//	}
	//};

	//m_StatsHotkey = obs_hotkey_register_frontend(
	//	"OBSBasic.ResetStats", locale.Str("Basic.Stats.ResetStats"),
	//	cbResetStats, mainFrameData);
	//_LoadHotkey(m_StatsHotkey, "OBSBasic.ResetStats");

	auto cbScreenShot = [](void* data, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
		if (pressed) {
			QMetaObject::invokeMethod(static_cast<AFMainFrame*>(data),
				"Screenshot", Qt::QueuedConnection);
		}
	};

	m_ScreenshotHotkey = obs_hotkey_register_frontend(
		"OBSBasic.Screenshot", locale.Str("Screenshot"),
		cbScreenShot, mainFrameData);
	_LoadHotkey(m_ScreenshotHotkey, "OBSBasic.Screenshot");

	//auto cbSourceScreenShot = [](void* data, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
	//	AFMainFrame& mainFrame = *static_cast<AFMainFrame*>(data);
	//	if (pressed) {
	//		mainFrame.RecvHotkeyScreenShotSelectedSource();
	//	}
	//};

	//m_SourceScreenshotHotkey = obs_hotkey_register_frontend(
	//	"OBSBasic.SelectedSourceScreenshot",
	//	locale.Str("Screenshot.SourceHotkey"),
	//	cbSourceScreenShot, mainFrameData);
	//_LoadHotkey(m_SourceScreenshotHotkey, "OBSBasic.SelectedSourceScreenshot");
}

bool AFHotkeyContext::FinContext()
{
	return false;
}

void AFHotkeyContext::ClearContext()
{
	obs_hotkey_pair_unregister(m_StreamingHotkeys);
	obs_hotkey_pair_unregister(m_RecordingHotkeys);
	obs_hotkey_pair_unregister(m_PauseHotkeys);
	obs_hotkey_unregister(m_SplitFileHotkey);
	obs_hotkey_pair_unregister(m_ReplayBufHotkeys);
	obs_hotkey_pair_unregister(m_VcamHotkeys);
	obs_hotkey_pair_unregister(m_TogglePreviewHotkeys);
	obs_hotkey_pair_unregister(m_ContextBarHotkeys);
	obs_hotkey_pair_unregister(m_togglePreviewProgramHotkeys);
	obs_hotkey_unregister(m_ForceStreamingStopHotkey);
	obs_hotkey_unregister(m_TransitionHotkey);
	obs_hotkey_unregister(m_StatsHotkey);
	obs_hotkey_unregister(m_ScreenshotHotkey);
	obs_hotkey_unregister(m_SourceScreenshotHotkey);

	if(m_ReplayBufferSave)
		obs_hotkey_unregister(m_ReplayBufferSave);

}

void AFHotkeyContext::RegisterHotkeyReplayBufferSave(obs_output_t* output)
{
	if (!output)
		return;

	AFMainFrame* mainFrameData = App()->GetMainView();

	m_ReplayBufferSave =
		obs_hotkey_register_output(output, "ReplayBuffer.Save", Str("ReplayBuffer.Save"),
			[](void* data, obs_hotkey_pair_id id, obs_hotkey_t* hotkey, bool pressed) {
				AFMainFrame& mainFrame = *static_cast<AFMainFrame*>(data);
				if (id && pressed) {
					mainFrame.qslotReplayBufferSave();
				}
			}, mainFrameData);

}

void AFHotkeyContext::UnRegisterHotkeyReplayBufferSave()
{
	if (m_ReplayBufferSave)
		obs_hotkey_unregister(m_ReplayBufferSave);
}


void AFHotkeyContext::_InitHotkeyTranslations(AFLocaleTextManager* pManager)
{
	ProfileScope("AFHotkeyContext::_InitHotkeyTranslations");

	struct obs_hotkeys_translations t = {};
	t.insert = pManager->Str("Hotkeys.Insert");
	t.del = pManager->Str("Hotkeys.Delete");
	t.home = pManager->Str("Hotkeys.Home");
	t.end = pManager->Str("Hotkeys.End");
	t.page_up = pManager->Str("Hotkeys.PageUp");
	t.page_down = pManager->Str("Hotkeys.PageDown");
	t.num_lock = pManager->Str("Hotkeys.NumLock");
	t.scroll_lock = pManager->Str("Hotkeys.ScrollLock");
	t.caps_lock = pManager->Str("Hotkeys.CapsLock");
	t.backspace = pManager->Str("Hotkeys.Backspace");
	t.tab = pManager->Str("Hotkeys.Tab");
	t.print = pManager->Str("Hotkeys.Print");
	t.pause = pManager->Str("Hotkeys.Pause");
	t.left = pManager->Str("Hotkeys.Left");
	t.right = pManager->Str("Hotkeys.Right");
	t.up = pManager->Str("Hotkeys.Up");
	t.down = pManager->Str("Hotkeys.Down");
#ifdef _WIN32
	t.meta = pManager->Str("Hotkeys.Windows");
#else
	t.meta = pManager->Str("Hotkeys.Super");
#endif
	t.menu = pManager->Str("Hotkeys.Menu");
	t.space = pManager->Str("Hotkeys.Space");
	t.numpad_num = pManager->Str("Hotkeys.NumpadNum");
	t.numpad_multiply = pManager->Str("Hotkeys.NumpadMultiply");
	t.numpad_divide = pManager->Str("Hotkeys.NumpadDivide");
	t.numpad_plus = pManager->Str("Hotkeys.NumpadAdd");
	t.numpad_minus = pManager->Str("Hotkeys.NumpadSubtract");
	t.numpad_decimal = pManager->Str("Hotkeys.NumpadDecimal");
	t.apple_keypad_num = pManager->Str("Hotkeys.AppleKeypadNum");
	t.apple_keypad_multiply = pManager->Str("Hotkeys.AppleKeypadMultiply");
	t.apple_keypad_divide = pManager->Str("Hotkeys.AppleKeypadDivide");
	t.apple_keypad_plus = pManager->Str("Hotkeys.AppleKeypadAdd");
	t.apple_keypad_minus = pManager->Str("Hotkeys.AppleKeypadSubtract");
	t.apple_keypad_decimal = pManager->Str("Hotkeys.AppleKeypadDecimal");
	t.apple_keypad_equal = pManager->Str("Hotkeys.AppleKeypadEqual");
	t.mouse_num = pManager->Str("Hotkeys.MouseButton");
	t.escape = pManager->Str("Hotkeys.Escape");
	obs_hotkeys_set_translations(&t);

	obs_hotkeys_set_audio_hotkeys_translations(pManager->Str("Mute"), pManager->Str("Unmute"),
											   pManager->Str("Push-to-mute"),
											   pManager->Str("Push-to-talk"));

	obs_hotkeys_set_sceneitem_hotkeys_translations(pManager->Str("SceneItemShow"),
												   pManager->Str("SceneItemHide"));

	obs_hotkey_enable_callback_rerouting(true);

	AFMainFrame* mainFrame = App()->GetMainView();
	obs_hotkey_set_callback_routing_func(AFMainFrame::HotkeyTriggered, mainFrame);
}

OBSData AFHotkeyContext::_LoadHotkeyData(const char* name)
{
	auto& confManager = AFConfigManager::GetSingletonInstance();

	const char* info =
		config_get_string(confManager.GetBasic(), "Hotkeys", name);
	if (!info)
		return {};

	OBSDataAutoRelease data = obs_data_create_from_json(info);
	if (!data)
		return {};

	return data.Get();
}

void AFHotkeyContext::_LoadHotkey(obs_hotkey_id id, const char* name)
{
	OBSDataArrayAutoRelease array =
		obs_data_get_array(_LoadHotkeyData(name), "bindings");

	obs_hotkey_load(id, array);
}

void AFHotkeyContext::_LoadHotkeyPair(obs_hotkey_pair_id id, const char* name0, 
									  const char* name1, 
									  const char* oldName /*= NULL*/)
{
	auto& confManager = AFConfigManager::GetSingletonInstance();

	if (oldName)
	{
		const auto info = config_get_string(confManager.GetBasic(),
											"Hotkeys", oldName);
		if (info)
		{
			config_set_string(confManager.GetBasic(), "Hotkeys", name0, info);
			config_set_string(confManager.GetBasic(), "Hotkeys", name1, info);
			config_remove_value(confManager.GetBasic(), "Hotkeys", oldName);
			config_save(confManager.GetBasic());
		}
	}
	OBSDataArrayAutoRelease array0 =
		obs_data_get_array(_LoadHotkeyData(name0), "bindings");
	OBSDataArrayAutoRelease array1 =
		obs_data_get_array(_LoadHotkeyData(name1), "bindings");

	obs_hotkey_pair_load(id, array0, array1);
}