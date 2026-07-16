#include "CHotkeyContext.h"

#include "Application/CApplication.h"

#include <util/util.hpp>
#include <util/profiler.hpp>

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/OBSOutput/COBSOutputContext.h"
#include "CoreModel/OBSOutput/COutput.h"

#include "MainFrame/CMainFrame.h"


void AFHotkeyContext::InitHotkeys()
{
	ProfileScope("AFHotkeyContext::InitHotkeys");

	struct obs_hotkeys_translations t = {};
	t.insert = Str("Hotkeys.Insert");
	t.del = Str("Hotkeys.Delete");
	t.home = Str("Hotkeys.Home");
	t.end = Str("Hotkeys.End");
	t.page_up = Str("Hotkeys.PageUp");
	t.page_down = Str("Hotkeys.PageDown");
	t.num_lock = Str("Hotkeys.NumLock");
	t.scroll_lock = Str("Hotkeys.ScrollLock");
	t.caps_lock = Str("Hotkeys.CapsLock");
	t.backspace = Str("Hotkeys.Backspace");
	t.tab = Str("Hotkeys.Tab");
	t.print = Str("Hotkeys.Print");
	t.pause = Str("Hotkeys.Pause");
	t.left = Str("Hotkeys.Left");
	t.right = Str("Hotkeys.Right");
	t.up = Str("Hotkeys.Up");
	t.down = Str("Hotkeys.Down");
#ifdef _WIN32
	t.meta = Str("Hotkeys.Windows");
#else
	t.meta = Str("Hotkeys.Super");
#endif
	t.menu = Str("Hotkeys.Menu");
	t.space = Str("Hotkeys.Space");
	t.numpad_num = Str("Hotkeys.NumpadNum");
	t.numpad_multiply = Str("Hotkeys.NumpadMultiply");
	t.numpad_divide = Str("Hotkeys.NumpadDivide");
	t.numpad_plus = Str("Hotkeys.NumpadAdd");
	t.numpad_minus = Str("Hotkeys.NumpadSubtract");
	t.numpad_decimal = Str("Hotkeys.NumpadDecimal");
	t.apple_keypad_num = Str("Hotkeys.AppleKeypadNum");
	t.apple_keypad_multiply = Str("Hotkeys.AppleKeypadMultiply");
	t.apple_keypad_divide = Str("Hotkeys.AppleKeypadDivide");
	t.apple_keypad_plus = Str("Hotkeys.AppleKeypadAdd");
	t.apple_keypad_minus = Str("Hotkeys.AppleKeypadSubtract");
	t.apple_keypad_decimal = Str("Hotkeys.AppleKeypadDecimal");
	t.apple_keypad_equal = Str("Hotkeys.AppleKeypadEqual");
	t.mouse_num = Str("Hotkeys.MouseButton");
	t.escape = Str("Hotkeys.Escape");
	obs_hotkeys_set_translations(&t);

	obs_hotkeys_set_audio_hotkeys_translations(Str("Mute"), Str("Unmute"),
											   Str("Push-to-mute"),
											   Str("Push-to-talk"));

	obs_hotkeys_set_sceneitem_hotkeys_translations(Str("SceneItemShow"),
												   Str("SceneItemHide"));

	obs_hotkey_enable_callback_rerouting(true);

	obs_hotkey_set_callback_routing_func(AFMainFrame::HotkeyTriggered, MAINFRAME);
}

void AFHotkeyContext::CreateHotkeys()
{
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

	auto main = MAINFRAME;

	ProfileScope("AFHotkeyContext::InitContext");
	m_streamingHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.StartStreaming", Str("Basic.Main.StartStreaming"),
		"OBSBasic.StopStreaming", Str("Basic.Main.StopStreaming"),
		MAKE_CALLBACK(mainFrame.EnableStartStreaming(), mainFrame.StartStreaming, "Starting stream"),
		MAKE_CALLBACK(mainFrame.EnableStopStreaming(), mainFrame.StopStreaming, "Stoping stream"),
		main, main);

	loadHotkeyPair(m_streamingHotkeys, "OBSBasic.StartStreaming","OBSBasic.StopStreaming");

	/*auto cb = [](void* data, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
		AFMainFrame& mainFrame = *static_cast<AFMainFrame*>(data);
		if (AFOutputUtil::IsStreamActive() && pressed) {
			mainFrame.ForceStopStreaming();
		}
	};

	m_forceStreamingStopHotkey = obs_hotkey_register_frontend(
		"OBSBasic.ForceStopStreaming", Str("Basic.Main.ForceStopStreaming"),
		cb, main);
	loadHotkey(m_forceStreamingStopHotkey, "OBSBasic.ForceStopStreaming");*/

	m_recordingHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.StartRecording", Str("Basic.Main.StartRecording"),
		"OBSBasic.StopRecording", Str("Basic.Main.StopRecording"),
		MAKE_CALLBACK(mainFrame.EnableStartRecording(), mainFrame.StartRecording, "Starting recording"),
		MAKE_CALLBACK(mainFrame.EnableStopRecording(), mainFrame.StopRecording, "Starting recording"),
		main, main);
	loadHotkeyPair(m_recordingHotkeys, "OBSBasic.StartRecording", "OBSBasic.StopRecording");

	/*m_pauseHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.PauseRecording", Str("Basic.Main.PauseRecording"),
		"OBSBasic.UnpauseRecording", Str("Basic.Main.UnpauseRecording"),
		MAKE_CALLBACK(mainFrame.EnablePauseRecording(), mainFrame.PauseRecording, "Pausing recording"),
		MAKE_CALLBACK(mainFrame.EnableUnPauseRecording(), mainFrame.UnPauseRecording, "Unpausing recording"),
		main, main);
	loadHotkeyPair(m_pauseHotkeys, "OBSBasic.PauseRecording","OBSBasic.UnpauseRecording");*/

	m_splitFileHotkey = obs_hotkey_register_frontend(
		"OBSBasic.SplitFile", Str("Basic.Main.SplitFile"),
		[](void*, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
			if (pressed)
				obs_frontend_recording_split_file();
		}, this);
	loadHotkey(m_splitFileHotkey, "OBSBasic.SplitFile");

	m_addChapterHotkey = obs_hotkey_register_frontend(
		"OBSBasic.AddChapterMarker", Str("Basic.Main.AddChapterMarker"),
		[](void*, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
			if(pressed)
				obs_frontend_recording_add_chapter(nullptr);
		}, this);
	loadHotkey(m_addChapterHotkey, "OBSBasic.AddChapterMarker");

	m_replayBufHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.StartReplayBuffer",Str("Basic.Main.StartReplayBuffer"),
		"OBSBasic.StopReplayBuffer", Str("Basic.Main.StopReplayBuffer"),
		MAKE_CALLBACK(!AFOutputUtil::IsReplayBufferActive(), mainFrame.StartReplayBuffer, "Starting replay buffer"),
		MAKE_CALLBACK(AFOutputUtil::IsReplayBufferActive(), mainFrame.StopReplayBuffer, "Stopping replay buffer"),
		main, main);
	loadHotkeyPair(m_replayBufHotkeys, "OBSBasic.StartReplayBuffer","OBSBasic.StopReplayBuffer");

	if(main->VirtualCamEnabled()) {
		m_virCamHotkeys = obs_hotkey_pair_register_frontend(
			"OBSBasic.StartVirtualCam", Str("Basic.Main.StartVirtualCam"),
			"OBSBasic.StopVirtualCam", Str("Basic.Main.StopVirtualCam"),
			MAKE_CALLBACK(!AFOutputUtil::IsVirtualCamActive(), mainFrame.qslotStartVirtualCam, "Starting virtual camera"),
			MAKE_CALLBACK(AFOutputUtil::IsVirtualCamActive(), mainFrame.qslotStopVirtualCam, "Stopping virtual camera"),
			main, main);
		loadHotkeyPair(m_virCamHotkeys, "OBSBasic.StartVirtualCam", "OBSBasic.StopVirtualCam");
	}

	m_togglePreviewHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.EnablePreview",  Str("Basic.Main.PreviewConextMenu.Enable"),
		"OBSBasic.DisablePreview", Str("Basic.Main.Preview.Disable"),
		MAKE_CALLBACK(!mainFrame.GetPreviewEnable(), mainFrame.EnablePreview, "Enabling preview"),
		MAKE_CALLBACK(mainFrame.GetPreviewEnable(), mainFrame.DisablePreview, "Disabling preview"),
		main, main);
	loadHotkeyPair(m_togglePreviewHotkeys, "OBSBasic.EnablePreview", "OBSBasic.DisablePreview");

	m_togglePreviewProgramHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.EnablePreviewProgram", Str("Basic.EnablePreviewProgramMode"),
		"OBSBasic.DisablePreviewProgram",Str("Basic.DisablePreviewProgramMode"),
		MAKE_CALLBACK(!mainFrame.IsPreviewProgramMode(), mainFrame.EnablePreviewProgam, "Enabling preview"),
		MAKE_CALLBACK(mainFrame.IsPreviewProgramMode(), mainFrame.DiablePreviewProgam, "Disabling preview"),
		main, main);
	loadHotkeyPair(m_togglePreviewProgramHotkeys,
				   "OBSBasic.EnablePreviewProgram",
				   "OBSBasic.DisablePreviewProgram",
				   "OBSBasic.TogglePreviewProgram");

#undef MAKE_CALLBACK

	auto cbTransition = [](void* data, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
		if (pressed) {
			QMetaObject::invokeMethod(static_cast<AFMainFrame*>(data), "qslotTransitionScene", Qt::QueuedConnection);
		}
	};

	m_transitionHotkey = obs_hotkey_register_frontend("OBSBasic.Transition", Str("Transition"), cbTransition, main);
	loadHotkey(m_transitionHotkey, "OBSBasic.Transition");

	/*auto resetStats = [](void* data, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
		if (pressed) {
			QMetaObject::invokeMethod(static_cast<AFMainFrame*>(data), "ResetStatsHotkey",
									  Qt::QueuedConnection);
		}
	};

	m_statsHotkey = obs_hotkey_register_frontend("OBSBasic.ResetStats", Str("Basic.Stats.ResetStats"),
												 resetStats, main);
	loadHotkey(m_statsHotkey, "OBSBasic.ResetStats");*/

	auto screenShot = [](void* data, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
		if (pressed) {
			QMetaObject::invokeMethod(static_cast<AFMainFrame*>(data), "qslotScreenShot",
									  Qt::QueuedConnection);
		}
	};

	m_screenshotHotkey = obs_hotkey_register_frontend(
		"OBSBasic.Screenshot", Str("Screenshot"),
		screenShot, main);
	loadHotkey(m_screenshotHotkey, "OBSBasic.Screenshot");

	/*auto screenshotSource = [](void* data, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
		if (pressed) {
			QMetaObject::invokeMethod(static_cast<AFMainFrame*>(data), "ScreenshotSelectedSource",
									  Qt::QueuedConnection);
		}
	};

	m_sourceScreenshotHotkey = obs_hotkey_register_frontend("OBSBasic.SelectedSourceScreenshot",
															Str("Screenshot.SourceHotkey"), screenshotSource, main);
	loadHotkey(m_sourceScreenshotHotkey, "OBSBasic.SelectedSourceScreenshot");*/
}

void AFHotkeyContext::ClearHotkeys()
{
	obs_hotkey_pair_unregister(m_streamingHotkeys);
	obs_hotkey_pair_unregister(m_recordingHotkeys);
	obs_hotkey_pair_unregister(m_pauseHotkeys);
	obs_hotkey_unregister(m_splitFileHotkey);
	obs_hotkey_unregister(m_addChapterHotkey);
	obs_hotkey_pair_unregister(m_replayBufHotkeys);
	obs_hotkey_pair_unregister(m_virCamHotkeys);
	obs_hotkey_pair_unregister(m_togglePreviewHotkeys);
	obs_hotkey_pair_unregister(m_contextBarHotkeys);
	obs_hotkey_pair_unregister(m_togglePreviewProgramHotkeys);
	obs_hotkey_unregister(m_forceStreamingStopHotkey);
	obs_hotkey_unregister(m_transitionHotkey);
	obs_hotkey_unregister(m_statsHotkey);
	obs_hotkey_unregister(m_screenshotHotkey);
	obs_hotkey_unregister(m_sourceScreenshotHotkey);

	if(m_replayBufferSave)
		obs_hotkey_unregister(m_replayBufferSave);
}

void AFHotkeyContext::RegisterHotkeyReplayBufferSave(obs_output_t* output)
{
	if (!output)
		return;

	m_replayBufferSave =
		obs_hotkey_register_output(output, "ReplayBuffer.Save", Str("ReplayBuffer.Save"),
			[](void* data, obs_hotkey_pair_id id, obs_hotkey_t* hotkey, bool pressed) {
				AFMainFrame& mainFrame = *static_cast<AFMainFrame*>(data);
				if (id && pressed) {
					mainFrame.qslotReplayBufferSave();
				}
			}, MAINFRAME);

}

void AFHotkeyContext::UnRegisterHotkeyReplayBufferSave()
{
	if (m_replayBufferSave)
		obs_hotkey_unregister(m_replayBufferSave);
}

obs_hotkey_id AFHotkeyContext::RegisterHotkey(const char* name, const char* description, obs_hotkey_func func)
{
	auto id = obs_hotkey_register_frontend(name, description, func, MAINFRAME);
	loadHotkey(id, name);
	return id;
}

void AFHotkeyContext::UnRegisterHotkey(obs_hotkey_id id)
{
	if(id)
		obs_hotkey_unregister(id);
}

OBSData AFHotkeyContext::loadHotkeyData(const char* name)
{
	const char* info = config_get_string(ACTIVECONFIG, "Hotkeys", name);
	if (!info)
		return {};

	OBSDataAutoRelease data = obs_data_create_from_json(info);
	if (!data)
		return {};

	return data.Get();
}

void AFHotkeyContext::loadHotkey(obs_hotkey_id id, const char* name)
{
	OBSDataArrayAutoRelease array = obs_data_get_array(loadHotkeyData(name), "bindings");
	obs_hotkey_load(id, array);
}

void AFHotkeyContext::loadHotkeyPair(obs_hotkey_pair_id id, const char* name0,
									 const char* name1, const char* oldName /*= NULL*/)
{
	config_t* activeConfig = ACTIVECONFIG;
	//
	if (oldName)
	{
		const auto info = config_get_string(activeConfig, "Hotkeys", oldName);
		if (info) {
			config_set_string(activeConfig, "Hotkeys", name0, info);
			config_set_string(activeConfig, "Hotkeys", name1, info);
			config_remove_value(activeConfig, "Hotkeys", oldName);
			config_save(activeConfig);
		}
	}
	OBSDataArrayAutoRelease array0 = obs_data_get_array(loadHotkeyData(name0), "bindings");
	OBSDataArrayAutoRelease array1 = obs_data_get_array(loadHotkeyData(name1), "bindings");

	obs_hotkey_pair_load(id, array0, array1);
}