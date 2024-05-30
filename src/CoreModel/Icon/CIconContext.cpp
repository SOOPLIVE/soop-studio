#pragma once
#include "CIconContext.h"

#include <obs.hpp>

#include "qt-wrapper.h"


void AFIconContext::InitContext()
{
	_LoadStudioIcon();
}

QIcon AFIconContext::GetSourceIcon(const char* id)
{
	obs_icon_type type = obs_source_get_icon_type(id);
	switch (type) {
	case OBS_ICON_TYPE_GAME_CAPTURE:
		return _GetGameCapIcon();
	case OBS_ICON_TYPE_DESKTOP_CAPTURE:
		return _GetDesktopCapIcon();
	case OBS_ICON_TYPE_WINDOW_CAPTURE:
		return _GetWindowCapIcon();
	case OBS_ICON_TYPE_CAMERA:
		return _GetVideoCapIcon();
	case OBS_ICON_TYPE_AUDIO_INPUT:
		return _GetAudioInputCaptureIcon();
	case OBS_ICON_TYPE_AUDIO_OUTPUT:
		return _GetAudioOutputCaptureIcon();
	case OBS_ICON_TYPE_PROCESS_AUDIO_OUTPUT:
		return _GetAudioProcessCaptureIcon();
	case OBS_ICON_TYPE_BROWSER:
		return _GetBrowserCaptureIcon();
	case OBS_ICON_TYPE_IMAGE:
		return _GetImageSourceIcon();
	case OBS_ICON_TYPE_SLIDESHOW:
		return _GetImageSliderIcon();
	case OBS_ICON_TYPE_MEDIA:
		return _GetMediaSourceIcon();
	case OBS_ICON_TYPE_TEXT:
		return _GetTextSourceIcon();
	case OBS_ICON_TYPE_COLOR:
		return _GetColorPaletteIcon();

	default:
		break;
	}

	return QIcon();
}

void AFIconContext::_LoadStudioIcon()
{
	// Source List View
	_LoadGameCaptureIcon();
	_LoadVideoCaptureIcon();
	_LoadDesktopCaptureIcon();
	_LoadWindowCaptureIcon();
	_LoadAudioInputCaptureIcon();
	_LoadAudioOutputCaptureIcon();
	_LoadAudioProcessCaptureIcon();
	_LoadBrowserCaptureIcon();
	_LoadImageSourceIcon();
	_LoadImageSliderIcon();
	_LoadMediaSourceIcon();
	_LoadTextSourceIcon();
	_LoadColorPaletteIcon();
	_LoadSceneIcon();
	_LoadGroupIcon();

	_LoadHotkeyConflictIcon();
}

void AFIconContext::_LoadGameCaptureIcon() {
    LoadIconFromABSPath("assets/scene-source-block/source-icon/ic_game_capture.svg", gameCapIcon);
}

void AFIconContext::_LoadVideoCaptureIcon() {
    LoadIconFromABSPath("assets/scene-source-block/source-icon/ic_video_capture.svg", cameraIcon);
}

void AFIconContext::_LoadDesktopCaptureIcon() {
    LoadIconFromABSPath("assets/scene-source-block/source-icon/ic_desktop_capture.svg", desktopCapIcon);
}

void AFIconContext::_LoadWindowCaptureIcon() {
    LoadIconFromABSPath("assets/scene-source-block/source-icon/ic_window_capture.svg", windowCapIcon);
}

void AFIconContext::_LoadAudioInputCaptureIcon() {
    LoadIconFromABSPath("assets/scene-source-block/source-icon/ic_audio_input_capture.svg", audioInputIcon);
}

void AFIconContext::_LoadAudioOutputCaptureIcon() {
    LoadIconFromABSPath("assets/scene-source-block/source-icon/ic_audio_output_capture.svg", audioOutputIcon);
}

void AFIconContext::_LoadAudioProcessCaptureIcon() {
    LoadIconFromABSPath("assets/scene-source-block/source-icon/ic_audio_process_capture.svg", audioProcessOutputIcon);
}

void AFIconContext::_LoadBrowserCaptureIcon() {
    LoadIconFromABSPath("assets/scene-source-block/source-icon/ic_browser_capture.svg", browserIcon);
}

void AFIconContext::_LoadImageSourceIcon() {
    LoadIconFromABSPath("assets/scene-source-block/source-icon/ic_image_source.svg", imageIcon);
}

void AFIconContext::_LoadImageSliderIcon() {
    LoadIconFromABSPath("assets/scene-source-block/source-icon/ic_image_slider.svg", slideshowIcon);
}

void AFIconContext::_LoadMediaSourceIcon() {
    LoadIconFromABSPath("assets/scene-source-block/source-icon/ic_media_source.svg", mediaIcon);
}

void AFIconContext::_LoadTextSourceIcon() {
    LoadIconFromABSPath("assets/scene-source-block/source-icon/ic_text_source.svg", textIcon);
}

void AFIconContext::_LoadColorPaletteIcon() {
    LoadIconFromABSPath("assets/scene-source-block/source-icon/ic_color_palette.svg", colorIcon);
}

void AFIconContext::_LoadSceneIcon() {
    LoadIconFromABSPath("assets/scene-source-block/source-icon/ic_scene.svg", sceneIcon);
}

void AFIconContext::_LoadGroupIcon() {
    LoadIconFromABSPath("assets/scene-source-block/source-icon/ic_group.svg", groupIcon);
}

void AFIconContext::_LoadHotkeyConflictIcon() {
	LoadIconFromABSPath("assets/setting-dialog/Hotkey/button_conflict_hotkey.svg", hotkeyConflictIcon);
}

QIcon AFIconContext::_GetGameCapIcon() const
{
	return gameCapIcon;
}
QIcon AFIconContext::_GetVideoCapIcon() const
{
	return cameraIcon;
}
QIcon AFIconContext::_GetDesktopCapIcon() const
{
	return desktopCapIcon;
}
QIcon AFIconContext::_GetWindowCapIcon() const
{
	return windowCapIcon;
}
QIcon AFIconContext::_GetAudioInputCaptureIcon() const
{
	return audioInputIcon;
}
QIcon AFIconContext::_GetAudioOutputCaptureIcon() const
{
	return audioOutputIcon;
}
QIcon AFIconContext::_GetAudioProcessCaptureIcon() const
{
	return audioProcessOutputIcon;
}
QIcon AFIconContext::_GetBrowserCaptureIcon() const
{
	return browserIcon;
}
QIcon AFIconContext::_GetImageSourceIcon() const
{
	return imageIcon;
}
QIcon AFIconContext::_GetImageSliderIcon() const
{
	return slideshowIcon;
}
QIcon AFIconContext::_GetMediaSourceIcon() const
{
	return mediaIcon;
}
QIcon AFIconContext::_GetTextSourceIcon() const
{
	return textIcon;
}
QIcon AFIconContext::_GetColorPaletteIcon() const
{
	return colorIcon;
}
QIcon AFIconContext::GetSceneIcon() const
{
	return sceneIcon;
}
QIcon AFIconContext::GetGroupIcon() const
{
	return groupIcon;
}

QIcon AFIconContext::GetHotkeyConflictIcon() const
{
	return hotkeyConflictIcon;
}