#pragma once
#include "CIconContext.h"

#include <obs.hpp>

#include "qt-wrappers.hpp"

#include "CoreModel/Source/CSource.h"

void AFIconContext::InitContext()
{
	_LoadStudioIcon();
}

QIcon AFIconContext::GetSourceIcon(const char* id)
{
	obs_icon_type type = obs_source_get_icon_type(id);
	switch (type) {
	case OBS_ICON_TYPE_GAME_CAPTURE:
		return _GetSourceIcon("game_capture");
	case OBS_ICON_TYPE_DESKTOP_CAPTURE:
		return _GetSourceIcon("monitor_capture");
	case OBS_ICON_TYPE_WINDOW_CAPTURE:
	{
		QString strId = id;
		if (0 == strId.compare("window_area_capture"))
			return _GetSourceIcon("window_area_capture");
		else
			return _GetSourceIcon("window_capture");
	}		
	case OBS_ICON_TYPE_CAMERA:
		return _GetSourceIcon("dshow_input");
	case OBS_ICON_TYPE_AUDIO_INPUT:
		return _GetSourceIcon("wasapi_input_capture");
	case OBS_ICON_TYPE_AUDIO_OUTPUT:
		return _GetSourceIcon("wasapi_output_capture");
	case OBS_ICON_TYPE_PROCESS_AUDIO_OUTPUT:
		return _GetSourceIcon("wasapi_process_output_capture");
	case OBS_ICON_TYPE_BROWSER:
	{
		QString strId = id;
		if(0 == strId.compare("browser_source"))
			return _GetSourceIcon("browser_source");
		else if (AFSourceUtil::IsSoopKBOSource(id))
			return _GetSourceIcon("soop_kbo_graphic_source");
		else if (AFSourceUtil::IsSoopFootballSource(id))
			return _GetSourceIcon("soop_football_graphic_source");
		else 
			return _GetSourceIcon(strId);
	}
	case OBS_ICON_TYPE_IMAGE:
		return _GetSourceIcon("image_source");
	case OBS_ICON_TYPE_SLIDESHOW:
		return _GetSourceIcon("slideshow");
	case OBS_ICON_TYPE_MEDIA:
	{
		QString strId = id;
		if (0 == strId.compare("ffmpeg_source"))
			return _GetSourceIcon("ffmpeg_source");
		else
			return _GetSourceIcon(strId);
	}
		return _GetSourceIcon("game_capture");
	case OBS_ICON_TYPE_TEXT:
		return _GetSourceIcon("text_gdiplus");
	case OBS_ICON_TYPE_COLOR:
		return _GetSourceIcon("color_source");
	case SOOP_ICON_TYPE_SPOUT2:
		return _GetSourceIcon("soop_spout2");

	default:
		return _GetSourceIcon("default_source");
	}

	return _GetSourceIcon("default_source");
}

void AFIconContext::_LoadStudioIcon()
{
	// Source List View
	_LoadSourceIcon("game_capture");
	_LoadSourceIcon("monitor_capture");
	_LoadSourceIcon("window_capture");
	_LoadSourceIcon("window_area_capture");
	_LoadSourceIcon("dshow_input");
	_LoadSourceIcon("wasapi_input_capture");
	_LoadSourceIcon("wasapi_output_capture");
	_LoadSourceIcon("wasapi_process_output_capture");
	_LoadSourceIcon("browser_source");
	_LoadSourceIcon("image_source");
	_LoadSourceIcon("slideshow");
	_LoadSourceIcon("ffmpeg_source");
	_LoadSourceIcon("ffmpeg_list_source");
	_LoadSourceIcon("text_gdiplus");
	_LoadSourceIcon("color_source");

	_LoadSourceIcon("soop_spout2");
	_LoadSourceIcon("soop_directbroad_source");
	_LoadSourceIcon("soop_tv_cable_source");
	_LoadSourceIcon("soop_anivod_source");
	_LoadSourceIcon("soop_sportvod_source");
	_LoadSourceIcon("soop_dramavod_source");
	_LoadSourceIcon("soop_movievod_source");

	_LoadSourceIcon("soop_chat_source_chat");
	_LoadSourceIcon("soop_chat_source_notice");
	_LoadSourceIcon("soop_chat_source_goal");
	_LoadSourceIcon("soop_chat_source_banner");
	_LoadSourceIcon("soop_chat_source_subtitle");
	_LoadSourceIcon("soop_chat_source_c_mission");
	_LoadSourceIcon("soop_chat_source_timer");
	_LoadSourceIcon("soop_chat_source_score");
	_LoadSourceIcon("soop_chat_source_mood_check");
	_LoadSourceIcon("soop_kbo_graphic_source");
	_LoadSourceIcon("soop_football_graphic_source");
	_LoadSourceIcon("soop_commerce_source_goal");
	_LoadSourceIcon("soop_commerce_source_rank");
	_LoadSourceIcon("soop_videoballoon_source");
	_LoadSourceIcon("soop_particle_effect_source");
	_LoadSourceIcon("soop_chat_source_anmSubtitle");

	_LoadSourceIcon("painter_source");
	_LoadSourceIcon("soop_aimanager_source");
	_LoadSourceIcon("default_source");

	_LoadSceneIcon();
	_LoadGroupIcon();

	_LoadHotkeyConflictIcon();
}

void AFIconContext::_LoadSourceIcon(QString id)
{
	QString path = QString("assets/scene-source-block/source-icon/ic_%1.svg")
						   .arg(id);

	QIcon icon;
	LoadIconFromABSPath(path.toStdString().c_str(), icon);
	m_sourceIcons[id] = icon;
}

void AFIconContext::_LoadSceneIcon() {
    LoadIconFromABSPath("assets/scene-source-block/source-icon/ic_scene.svg", m_sceneIcon);
}

void AFIconContext::_LoadGroupIcon() {
    LoadIconFromABSPath("assets/scene-source-block/source-icon/ic_group.svg", m_groupIcon);
}

void AFIconContext::_LoadHotkeyConflictIcon() {
	LoadIconFromABSPath("assets/setting-dialog/Hotkey/button_conflict_hotkey.svg", m_hotkeyConflictIcon);
}

QIcon AFIconContext::_GetSourceIcon(QString id) const 
{	
	auto it = m_sourceIcons.find(id);
	if (it == m_sourceIcons.end()) {
		it = m_sourceIcons.find("default_source");
		return (*it).second;
	}

	return (*it).second;
}

QIcon AFIconContext::GetSceneIcon() const
{
	return m_sceneIcon;
}

QIcon AFIconContext::GetGroupIcon() const
{
	return m_groupIcon;
}

QIcon AFIconContext::GetHotkeyConflictIcon() const
{
	return m_hotkeyConflictIcon;
}
