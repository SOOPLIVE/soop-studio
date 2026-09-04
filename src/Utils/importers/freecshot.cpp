#include "importers.hpp"

#include <QByteArray>

#include "Common/StudioDefine.h"

#include "MainFrame/CMainFrame.h"
#include "Utils/soop-crypt.hpp"

#ifdef _WIN32
#include <shlobj.h>
#include <io.h>
#include <Psapi.h>
#endif
#include <regex>

#include <initguid.h>
#include <uuids.h>

using namespace std;
using namespace json11;

#define translate_int(in_key, in, out_key, out, off) \
	out[out_key] = in[in_key].int_value() + off;
#define translate_string(in_key, in, out_key, out) out[out_key] = in[in_key];
#define translate_bool(in_key, in, out_key, out) \
	out[out_key] = in[in_key].int_value() == 1;

static int red_blue_swap(int color)
{
	int r = color / 256 / 256;
	int b = color % 256;

	return color - (r * 65536) - b + (b * 65536) + r;
}
#ifdef _WIN32
struct WinApiMonitorInfo {
	HMONITOR hMonitor;
	RECT rcMonitor;
	std::wstring deviceName;
	int enumOrderIndex;
};

static BOOL CALLBACK MonitorEnumProcCallback(HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData) {
	auto* monitorList = reinterpret_cast<std::vector<WinApiMonitorInfo>*>(dwData);
	if (!monitorList) return FALSE;

	MONITORINFOEXW miex;
	miex.cbSize = sizeof(miex);
	if (GetMonitorInfoW(hMonitor, &miex)) {
		monitorList->push_back({ hMonitor, miex.rcMonitor, std::wstring(miex.szDevice),
			static_cast<int>(monitorList->size()) });
	}
	return TRUE;
}

HWND FindWindowHandleByProcessName(DWORD processID) {

	HWND hwnd = FindWindow(nullptr, nullptr);
	while (hwnd) {
		DWORD windowProcessID;
		GetWindowThreadProcessId(hwnd, &windowProcessID);
		if (windowProcessID == processID) {
			return hwnd;
		}
		hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
	}

	return nullptr;
}

struct EnumData {
	std::string targetProcessName;
	std::vector<HWND> foundHwnds;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
	EnumData* pData = reinterpret_cast<EnumData*>(lParam);
	DWORD processId;
	GetWindowThreadProcessId(hwnd, &processId);

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
	if (hProcess == NULL) {
		return TRUE;
	}
	CHAR processPath[MAX_PATH];
	if (GetModuleFileNameExA(hProcess, NULL, processPath, MAX_PATH)) {
		std::string fullPath(processPath);
		size_t lastSlash = fullPath.find_last_of("\\");
		if (lastSlash != std::string::npos) {
			std::string processName = fullPath.substr(lastSlash + 1);
			if (stricmp(processName.c_str(), pData->targetProcessName.c_str()) == 0) {
				pData->foundHwnds.push_back(hwnd);
			}
		}
	}
	CloseHandle(hProcess);
	return TRUE;
}
#endif

static std::string find_class_name(std::string winname, std::string subprocname)
{
	std::string classname = "";
#ifdef _WIN32
	BPtr<wchar_t> wname;
	os_utf8_to_wcs_ptr(winname.c_str(), winname.size(), &wname);

	HWND hWnd = FindWindow(NULL, wname);
	if (hWnd == NULL) {
		EnumData data;
		data.targetProcessName = subprocname;

		EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
		if (!data.foundHwnds.empty()) {
			for (HWND findhwnd : data.foundHwnds) {
				if (IsWindow(findhwnd)) {
					hWnd = findhwnd;
					break;
				}
			}
			if (hWnd == NULL)
				return "";
		}
		else {
			return "";
		}		
	}

	char szClassname[256];
	if (GetClassNameA(hWnd, szClassname, sizeof(szClassname) / sizeof(char))) {
		classname = szClassname;
	}
#endif
	return classname;
}

static bool source_name_exists(const string& name, const Json::array& sources)
{
	for (size_t i = 0; i < sources.size(); i++) {
		if (sources.at(i)["name"].string_value() == name)
			return true;
	}

	return false;
}

static inline std::string deco_to_preset(int t) {
	switch (t) {
	case 8100: return "preset_2";
	case 8101: return "preset_3";
	case 8102: return "preset_4";
	case 8103: return "preset_5"; 
	case 8104: return "preset_6";
	case 8105: return "preset_horizontal_flip";
	case 8106: return "preset_3_rgb";
	default:   return "";
	}
}

static inline std::pair<int, int> deco_to_repeat(int t) {
	switch (t) {
	case 8100: return { 2, 1 };
	case 8101: return { 3, 1 };
	case 8102: return { 4, 1 };
	case 8103: return { 5, 1 };
	case 8104: return { 2, 3 };
	case 8105: return { 2, 1 };
	case 8106: return { 3, 1 };
	default:   return { 2, 1 };
	}
}

static inline std::pair<std::string, std::string>
SplitFaceStyle(std::string raw)
{
	auto trim = [](std::string s) {
		const char* ws = " \t\r\n";
		auto b = s.find_first_not_of(ws), e = s.find_last_not_of(ws);
		return (b == std::string::npos) ? std::string() : s.substr(b, e - b + 1);
	};
	auto tolower_copy = [](std::string s) {
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char c) { return (char)std::tolower(c); });
		return s;
	};
	auto collapse_spaces = [&](std::string s) {
		for (char& c : s) if (c == '_') c = ' ';
		std::string o; o.reserve(s.size()); bool sp = false;
		for (char c : s) {
			bool issp = std::isspace((unsigned char)c);
			if (issp) { if (!sp) { o.push_back(' '); sp = true; } }
			else { o.push_back(c); sp = false; }
		}
		return trim(o);
	};
	auto camel_to_spaced = [&](const std::string& s) {
		// "NotoSansKRExtraBold" → "Noto Sans KR Extra Bold"
		std::string out; out.reserve(s.size() * 2);
		for (size_t i = 0; i < s.size(); ++i) {
			unsigned char c = (unsigned char)s[i];
			bool up = std::isupper(c);
			bool prevLower = (i > 0) && std::islower((unsigned char)s[i - 1]);
			bool nextLower = (i + 1 < s.size()) && std::islower((unsigned char)s[i + 1]);
			if (i > 0 && up && (prevLower || nextLower)) out.push_back(' ');
			out.push_back((char)c);
		}
		return out;
	};

	raw = trim(raw);
	if (raw.empty()) return { "","Regular" };
	{
		auto pos = raw.find_last_of('-');
		if (pos != std::string::npos && pos + 1 < raw.size()) {
			std::string abbr = raw.substr(pos + 1);
			std::string left = raw.substr(0, pos);
			std::string style;
			std::string ab = tolower_copy(abbr);
			if (ab == "bi" || ab == "ib") style = "Bold Italic";
			else if (ab == "b") style = "Bold";
			else if (ab == "i") style = "Italic";
			else if (ab == "r") style = "Regular";
			else if (ab == "m") style = "Medium";
			else if (ab == "l") style = "Light";

			if (!style.empty()) {
				left = collapse_spaces(camel_to_spaced(left));
				return { left, style };
			}
		}
	}

	{
		auto pos = raw.find_last_of('-');
		if (pos != std::string::npos && pos + 1 < raw.size()) {
			std::string left = raw.substr(0, pos);
			std::string right = raw.substr(pos + 1);

			auto fix = [&](std::string s) {
				std::string t = camel_to_spaced(s);
				std::string L = tolower_copy(t);
				if (L == "extrabold") return std::string("Extra Bold");
				if (L == "ultrabold") return std::string("Ultra Bold");
				if (L == "extrablack") return std::string("Extra Black");
				if (L == "ultrablack") return std::string("Ultra Black");
				if (L == "semibold")  return std::string("Semi Bold");
				if (L == "demibold")  return std::string("Demi Bold");
				if (L == "bolditalic") return std::string("Bold Italic");
				if (L == "blackitalic") return std::string("Black Italic");
				if (L == "heavyitalic") return std::string("Heavy Italic");
				return collapse_spaces(t);
			};
			left = collapse_spaces(camel_to_spaced(left));
			right = fix(right);
			return { left, right.empty() ? std::string("Regular") : right };
		}
	}

	static const char* TOKENS[] = {
		"Bold Italic","Extra Black","Ultra Black","Extra Bold","Ultra Bold",
		"Semi Bold","Demi Bold","Black","Heavy","Bold","Italic",
		"Condensed","Expanded","Narrow","Compressed","Extended",
		"Medium","Regular","Book","Light","Thin"
	};
	{
		std::string spaced = collapse_spaces(camel_to_spaced(raw));
		std::string face = spaced, style = "Regular";
		auto L = tolower_copy(spaced);
		for (auto tok : TOKENS) {
			std::string lt = tolower_copy(std::string(" ") + tok);
			if (L.size() >= lt.size() && L.rfind(tolower_copy(lt)) == L.size() - lt.size()) {
				face = trim(spaced.substr(0, spaced.size() - lt.size()));
				style = tok;
				break;
			}
		}
		return { face, style };
	}
}

auto alpha255_to_trans_percent = [](int a) -> int {
	if (a < 0) a = 0;
	if (a > 255) a = 255;
	int opacity = static_cast<int>(std::round(a * 100.0 / 255.0));
	return 100 - opacity;
};

static std::string EncodeDShowIdPart(const std::string& value)
{
	std::string encoded;
	encoded.reserve(value.size());

	for (char ch : value) {
		if (ch == '#')
			encoded += "#22";
		else if (ch == ':')
			encoded += "#3A";
		else
			encoded += ch;
	}

	return encoded;
}

static bool parse_source(Json::object& out, const Json& in, void* parent, std::string& uuid)
{
	bool result = false;
	
	const bool is_split_group = in["__GROUP_IS_SPLIT"].is_bool() ? in["__GROUP_IS_SPLIT"].bool_value() : false;
	const bool is_representative = in["__GROUP_IS_REP"].is_bool() ? in["__GROUP_IS_REP"].bool_value() : false;
	const int  classindex = in["CLASSINDEX"].int_value();

    if (is_split_group && classindex == 2) return false;
    if (is_split_group && !is_representative) return false;

	Json::object settings = Json::object{};

	switch (classindex)
	{
	case 1:
	{
		out["id"] = "window_area_capture";
		out["name"] = in["NAME"].string_value();

		std::vector<WinApiMonitorInfo> winApiOrderedMonitors;
		EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProcCallback, reinterpret_cast<LPARAM>(&winApiOrderedMonitors));

		POINT pt, pt_local;
		pt.x = in["SCREEN"]["XPOS"].int_value();
		pt.y = in["SCREEN"]["YPOS"].int_value();

		int monitor_index = -1;
		HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
		if (hMon) {
			for (const auto& winApiMonitor : winApiOrderedMonitors) {
				if (winApiMonitor.hMonitor == hMon) {
					monitor_index = winApiMonitor.enumOrderIndex;
					pt_local.x = pt.x - winApiMonitor.rcMonitor.left;
					pt_local.y = pt.y - winApiMonitor.rcMonitor.top;
					break;
				}
			}
		}
		if (monitor_index == -1) break;
				
		settings["desktop_monitor"] = true;
		settings["monitor"] = monitor_index;
		settings["use_subregion"] = true;
		settings["subregion_x"] = (int)pt_local.x;
		settings["subregion_y"] = (int)pt_local.y;
		settings["subregion_width"] = in["SCREEN"]["WIDTH"].int_value();
		settings["subregion_height"] = in["SCREEN"]["HEIGHT"].int_value();
		settings["cursor"] = in["SCREEN"]["CURSOR"].int_value() == 1 ? true : false;

		out["settings"] = settings;
		result = true;
	}
	break;
	case 2:
	{
		auto importer = static_cast<FreecShotImporter*>(parent);
		if (importer == nullptr)
			break;

		std::string videoName =
			in["DEVICE"]["VIDEO_FRIENDLY_NAME"].string_value();
		std::string videoPath =
			in["DEVICE"]["VIDEO_DEVICE_PATH"].string_value();

		std::string device_id =
			EncodeDShowIdPart(videoName) + ":" +
			EncodeDShowIdPart(videoPath);
			
		out["name"] = in["NAME"].string_value();
		out["id"] = "dshow_input";

		settings["video_device_id"] = device_id; // name + ":" + ID
		settings["last_video_device_id"] = device_id;

		int width = in["DEVICE"]["WIDTH"].int_value();
		int height = in["DEVICE"]["HEIGHT"].int_value(); 
		std::string resolution = std::to_string(width) + "x" + std::to_string(height);
		settings["resolution"] = resolution;
		settings["last_resolution"] = resolution;

		auto fps = in["DEVICE"]["FPS"].number_value();
		if (fps > 0) {
			int frame_intreval = static_cast<int>(static_cast<double>(10000000) / fps); // hns
			settings["frame_interval"] = frame_intreval;
		}

		auto media_type = in["DEVICE"]["MEDIA_TYPE"].string_value();
		std::wstring wstr(media_type.begin(), media_type.end());
		GUID guid;
		HRESULT hr = CLSIDFromString(wstr.c_str(), &guid);
		if (SUCCEEDED(hr)) {
			int format = 0; // VideoFormat::Any

			if (IsEqualGUID(guid, MEDIASUBTYPE_ARGB32))
				format = 100; // VideoFormat::ARGB
			else if (IsEqualGUID(guid, MEDIASUBTYPE_RGB32))
				format = 101; // VideoFormat::XRGB
			else if (IsEqualGUID(guid, MEDIASUBTYPE_RGB24))
				format = 102; // VideoFormat::RGB24
			else if (IsEqualGUID(guid, { 0x30323449, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} })) // MEDIASUBTYPE_I420
				format = 200; // VideoFormat::I420
			else if (IsEqualGUID(guid, MEDIASUBTYPE_NV12))
				format = 201; // VideoFormat::NV12
			else if (IsEqualGUID(guid, MEDIASUBTYPE_YV12))
				format = 202; // VideoFormat::YV12
			else if (IsEqualGUID(guid, { 0x30303859, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} })) // MEDIASUBTYPE_Y800
				format = 203; // VideoFormat::Y800
			else if (IsEqualGUID(guid, MEDIASUBTYPE_P010))
				format = 204; // VideoFormat::P010
			else if (IsEqualGUID(guid, MEDIASUBTYPE_YVYU))
				format = 300; // VideoFormat::YVYU
			else if (IsEqualGUID(guid, MEDIASUBTYPE_YUY2))
				format = 301; // VideoFormat::YUY2
			else if (IsEqualGUID(guid, MEDIASUBTYPE_UYVY))
				format = 302; // VideoFormat::UYVY
			else if (IsEqualGUID(guid, { 0x43594448, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} })) // MEDIASUBTYPE_HDYC
				format = 303; // VideoFormat::HDYC
			else if (IsEqualGUID(guid, MEDIASUBTYPE_MJPG))
				format = 400; // VideoFormat::MJPEG
			else if (IsEqualGUID(guid, MEDIASUBTYPE_H264))
				format = 401; // VideoFormat::H264
			else if (IsEqualGUID(guid, { 0x43564548, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} })) // MEDIASUBTYPE_HEVC
				format = 402; // VideoFormat::HEVC

			settings["video_format"] = format;
		}

		device_id = in["DEVICE"]["AUDIO_CAPTURE_FRIENDLY_NAME"].string_value();
		if (device_id.compare("N") != 0)
		{
			settings["use_custom_audio_device"] = true;
			settings["audio_device_id"] = EncodeDShowIdPart(device_id) + ":";
		}
		settings["res_type"] = 1;

		out["settings"] = settings;

		auto audio_render_display_name = in["DEVICE"]["AUDIO_RENDER_DISPLAY_NAME"].string_value();
		int monitoring_type = 0;
		if (!audio_render_display_name.compare("D"))
			monitoring_type = 1;

		out["monitoring_type"] = monitoring_type;

		result = true;
	}
	break;
	case 3:
	{
		out["id"] = "image_source";
		out["name"] = in["NAME"].string_value();

		settings["file"] = in["IMAGE"]["PATH"].string_value();
		out["settings"] = settings;
		result = true;
	}
	break;
	case 4:
	{
		out["id"] = "text_gdiplus";
		out["name"] = in["NAME"].string_value();

		settings["text"] = in["CAPTION"]["TEXT"].string_value();
		bool outline = in["CAPTION"]["USE_OUTLINECOLOR"].int_value() != 0 ? true : false;
		if (outline) {
			settings["outline"] = true;
			settings["outline_size"] = in["CAPTION"]["THICK"].int_value();
			settings["outline_color"] = in["CAPTION"]["OUTLINECOLOR"].int_value();
		}

		bool bgcolor = in["CAPTION"]["USE_BGCOLOR"].int_value() != 0 ? true : false;
		if (bgcolor) {
			settings["bk_color"] = in["CAPTION"]["BGCOLOR"].int_value();
		}

		{
			int alphaPercent = 100;
			if (in["VEFFECT"].is_object() && in["VEFFECT"]["ALPHA"].is_number()) {
				int alpha255 = in["VEFFECT"]["ALPHA"].int_value();
				alphaPercent = alpha255_to_trans_percent(alpha255);
			}

			if (alphaPercent < 0) alphaPercent = 0;
			if (alphaPercent > 100) alphaPercent = 100;

			settings["bk_opacity"] = (bgcolor ? alphaPercent : 100);
			settings["opacity"] = alphaPercent;
			if (outline) {
				settings["outline_opacity"] = alphaPercent;
			}
		}

		settings["color"] = in["CAPTION"]["TEXT_COLOR"].int_value();

		Json::object fontobj = Json::object{};

		std::string face, style;
		std::tie(face, style) = SplitFaceStyle(in["CAPTION"]["FONTNAME"].string_value());

		fontobj["face"] = face;
		fontobj["style"] = style;
		fontobj["size"] = in["CAPTION"]["FONTSIZE"].int_value();

		int flags = 0;
		if (in["CAPTION"]["BOLD"].int_value()) {
			flags |= OBS_FONT_BOLD;
			fontobj["style"] = "Bold";
		}
		if (in["CAPTION"]["UNDERLINE"].int_value())
			flags |= OBS_FONT_UNDERLINE;
		if (in["CAPTION"]["STRIKEOUT"].int_value())
			flags |= OBS_FONT_STRIKEOUT;

		if (flags != 0)
			fontobj["flags"] = flags;			
		
		settings["font"] = fontobj;
		out["settings"] = settings;
		result = true;
	}
	break;
	case 5:
	{
		out["id"] = "ffmpeg_source";
		out["name"] = in["NAME"].string_value();

		settings["input"] = in["STREAM"]["STREAM_PATH"].string_value();
		settings["is_local_file"] = false;
		out["settings"] = settings;
		out["monitoring_type"] = OBS_MONITORING_TYPE_MONITOR_ONLY;
		result = true;
	}
	break;
	case 6:
	{
		std::string wnd_name = in["GAME"]["WNDNAME"].string_value();
		std::string proc_name = in["GAME"]["PROCNAME"].string_value();
		if(wnd_name.empty() ||
		   proc_name.empty())
			break;

		out["id"] = "game_capture";
		out["name"] = in["NAME"].string_value();

		std::string classname = find_class_name(wnd_name, proc_name);
		if(classname.empty())
			classname = "(freecshot)";
		settings["capture_mode"] = "window";
		settings["window"] = wnd_name + ":"+ classname + ":" + proc_name;
		settings["cursor"] = in["GAME"]["CURSOR"].int_value() == 0 ? true : false;
		out["settings"] = settings;
		result = true;
	}
	break;
	case 7:
	{
		auto importer = static_cast<FreecShotImporter*>(parent);
		if (importer == nullptr)
			break;

		std::string name = in["NAME"].string_value();
		int browser_key = in["WEBURL"]["KEY"].int_value();
		result = importer->setBrowserUUID(browser_key);
		uuid = importer->getBrowserUUID(browser_key);
		if (result == false) {
			result = true;
			break;
		}

		out["id"] = "browser_source";
		out["uuid"] = uuid;
		out["name"] = in["NAME"].string_value();
		settings["width"] = in["WEBURL"]["WIDTH"].int_value();
		settings["height"] = in["WEBURL"]["HEIGHT"].int_value();
		settings["url"] = in["WEBURL"]["WEBURL_PATH"].string_value();
		out["settings"] = settings;
		out["monitoring_type"] = OBS_MONITORING_TYPE_MONITOR_ONLY;
		result = true;
	}
	break;
	case 8:
	{
		out["id"] = "ffmpeg_source";
		out["name"] = in["NAME"].string_value();

		settings["local_file"] = in["VIDEO_FILE"]["PATH"].string_value();
		settings["looping"] = in["VIDEO_FILE"]["REPEAT"].int_value() == 1 ? true : false;
		settings["restart_on_activate"] = in["VIDEO_FILE"]["REWIND"].int_value() == 1 ? true : false;
		settings["clear_on_media_end"] = in["VIDEO_FILE"]["HIDE"].int_value() == 1 ? true : false;
		out["settings"] = settings;
		out["monitoring_type"] = OBS_MONITORING_TYPE_MONITOR_ONLY;
		result = true;
	}
	break;
	case 9:
	{
		out["id"] = "slideshow";
		out["name"] = in["NAME"].string_value();

		Json::array slider_items = Json::array{};
		int slider_count = in["IMAGESLIDER"]["SLIDER_IMGCNT"].int_value();
		
		for (auto& slider_item : in["IMAGESLIDER"]["IMAGE"].array_items()) {
			Json::object item = Json::object{};
			item["value"] = slider_item["SLIDER_PATH"].string_value();
			item["selected"] = false;
			item["hidden"] = false;
			slider_items.push_back(item);
		}

		settings["files"] = slider_items;
		settings["slide_time"] = in["IMAGESLIDER"]["SLIDER_ELAPSE"].int_value();
		settings["randomize"] = in["IMAGESLIDER"]["SLIDER_REPEATSEQUENCE"].int_value() == 1 ? true : false;		
		settings["use_custom_size"] = "16:9";
		out["settings"] = settings;
		result = true;

	}
	break;
	case 11:
	{

	}
	break;
	case 12:
	{
		out["id"] = "ffmpeg_list_source";
		out["name"] = in["NAME"].string_value();

		Json::array slider_items = Json::array{};
		for (auto& slider_item : in["VIDEO_LIST"]["VIDEO_LIST"].array_items()) {
			Json::object item = Json::object{};
			item["value"] = slider_item["LIST"].string_value();
			item["selected"] = false;
			item["hidden"] = false;
			slider_items.push_back(item);
		}

		settings["playlist"] = slider_items;
		settings["looping"] = in["VIDEO_LIST"]["REPEAT"].int_value() == 1 ? true : false;
		settings["current_file_name"] = in["VIDEO_LIST"]["PATH"].string_value();
		out["settings"] = settings;
		out["monitoring_type"] = OBS_MONITORING_TYPE_MONITOR_ONLY;
		result = true;
	}
	break;
	case 13:
	{
		if (in["WINCAP"]["DESKTOP_CAPTURE"].int_value() == 0) {
			out["name"] = in["NAME"].string_value();
			out["id"] = "window_area_capture";
			std::string classname = find_class_name(in["WINCAP"]["WNDNAME"].string_value(), in["WINCAP"]["PROCNAME"].string_value());
			if (classname.empty())
				break;

			string windName = StringReplace(in["WINCAP"]["WNDNAME"].string_value(), "/", "\\\\");
			windName = StringReplace(windName, ":", "#3A");
			classname = StringReplace(classname, ":", "#3A");

			settings["window"] = windName + ":" + classname + ":" + in["WINCAP"]["PROCNAME"].string_value();
			settings["desktop_monitor"] = false;
			if (1 == in["WINCAP"]["ISREGION"].int_value()) {
				settings["use_subregion"] = true;
				settings["subregion_x"] = in["WINCAP"]["CLIP_X1"].int_value();
				settings["subregion_y"] = in["WINCAP"]["CLIP_Y1"].int_value();
				settings["subregion_width"] = in["WINCAP"]["CLIP_X2"].int_value();
				settings["subregion_height"] = in["WINCAP"]["CLIP_Y2"].int_value();
			}
			else {
				settings["use_subregion"] = false;
			}
				
			settings["cursor"] = in["WINCAP"]["CURSOR"].int_value() == 1 ? true : false;
			out["settings"] = settings;
			result = true;
		} else {
#ifdef _WIN32
			out["id"] = "window_area_capture";
			out["name"] = in["NAME"].string_value();
			settings["desktop_monitor"] = true;
			RECT rect;
			rect.left = in["WINCAP"]["WIN_X1"].int_value();
			rect.top = in["WINCAP"]["WIN_Y1"].int_value();
			rect.right = rect.left + in["WINCAP"]["WIN_X2"].int_value();
			rect.bottom = rect.top + in["WINCAP"]["WIN_Y2"].int_value();
			
			
			if (in["WINCAP"]["CLIP_X1"].int_value() == 0 && in["WINCAP"]["CLIP_X2"].int_value() == 0 &&
				in["WINCAP"]["CLIP_Y1"].int_value() == 0 && in["WINCAP"]["CLIP_Y2"].int_value() == 0) {				
				HMONITOR hMonitor = ::MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
				MONITORINFOEXA mi;
				mi.cbSize = sizeof(mi);
				if (GetMonitorInfoA(hMonitor, (LPMONITORINFO)&mi)) {
					DISPLAY_DEVICEA device;
					device.cb = sizeof(device);
					EnumDisplayDevicesA(mi.szDevice, 0, &device, EDD_GET_DEVICE_INTERFACE_NAME);
					settings["monitor"] = device.DeviceID;
					settings["use_subregion"] = false;
				}
			}
			else {
				std::vector<WinApiMonitorInfo> winApiOrderedMonitors;
				EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProcCallback, reinterpret_cast<LPARAM>(&winApiOrderedMonitors));

				int monitor_index = -1;
				HMONITOR hMonitor = ::MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
				if (hMonitor) {
					for (const auto& winApiMonitor : winApiOrderedMonitors) {
						if (winApiMonitor.hMonitor == hMonitor) {
							monitor_index = winApiMonitor.enumOrderIndex;
							break;
						}
					}
				}
				if (monitor_index == -1) monitor_index = 0;

				QPoint globalPos = QPoint(in["WINCAP"]["WIN_X1"].int_value(), in["WINCAP"]["WIN_Y1"].int_value());
				QScreen* screen = QGuiApplication::screenAt(globalPos);
				if (screen) {
					QPoint screenTopLeft = screen->geometry().topLeft();
					QPoint monitorLocalPos = globalPos - screenTopLeft;
					settings["monitor"] = monitor_index;
					settings["use_subregion"] = true;
					settings["subregion_x"] = monitorLocalPos.x();
					settings["subregion_y"] = monitorLocalPos.y();
					settings["subregion_width"] = in["WINCAP"]["WIN_X2"].int_value();
					settings["subregion_height"] = in["WINCAP"]["WIN_Y2"].int_value();
				}
			}
			settings["cursor"] = in["WINCAP"]["CURSOR"].int_value() == 1 ? true : false;
			out["settings"] = settings;
			result = true;
#endif
		}
	}
	break;
	case 15:
	{
		auto importer = static_cast<FreecShotImporter*>(parent);
		if (importer == nullptr)
			break;

		std::string dowoomiType = in["DOWOOMI"]["DOWOOMI_TYPE"].string_value();
		const std::string path = in["DOWOOMI"]["DOWOOMI_PATH"].string_value();

		int browser_key = in["DOWOOMI"]["KEY"].int_value();
		result = importer->setBrowserUUID(browser_key);
		uuid = importer->getBrowserUUID(browser_key);
		if (result == false) {
			result = true;
			break;
		}
		out["uuid"] = uuid;

		out["name"] = in["NAME"].string_value();
		settings["width"] = in["DOWOOMI"]["WIDTH"].int_value();
		settings["height"] = in["DOWOOMI"]["HEIGHT"].int_value();

		if (dowoomiType == "compete_team") {

			out["id"] = "soop_mission_source_battle_joinusers";
			out["versioned_id"] = "soop_mission_source_battle_joinusers";

			std::string newUrl = path;
			if (newUrl.find("szBroadId=") == std::string::npos) {
				const std::string kKey = "szBjId=";
				size_t pos = newUrl.find(kKey);
				if (pos != std::string::npos) {
					size_t valStart = pos + kKey.size();
					size_t valEnd = newUrl.find('&', valStart);
					std::string bjId = (valEnd == std::string::npos)
						? newUrl.substr(valStart)
						: newUrl.substr(valStart, valEnd - valStart);

					if (!bjId.empty()) {
						char inBuf[256] = { 0 };
						char outBuf[1024] = { 0 };

						size_t copyLen = std::min(bjId.size(), sizeof(inBuf) - 1);
						memcpy(inBuf, bjId.data(), copyLen);
						inBuf[copyLen] = '\0';

						int encryptedLen = soop_crypt_encrypt(inBuf, outBuf, (int)sizeof(outBuf));
						if (encryptedLen > 0 && encryptedLen < (int)sizeof(outBuf)) {
							std::string enc(outBuf, outBuf + encryptedLen);
							newUrl.replace(pos, kKey.size() + bjId.size(),
								std::string("szBroadId=") + enc);
						}
						else {
							break;
						}
					}
				}
			}
			settings["url"] = newUrl;
		}
		else if (dowoomiType == "compete_donation") {

			out["id"] = "soop_mission_source_battle_fundingrank";
			out["versioned_id"] = "soop_mission_source_battle_fundingrank";

			std::string newUrl = path;
			if (newUrl.find("szBroadId=") == std::string::npos) {
				const std::string kKey = "szBjId=";
				size_t pos = newUrl.find(kKey);
				if (pos != std::string::npos) {
					size_t valStart = pos + kKey.size();
					size_t valEnd = newUrl.find('&', valStart);
					std::string bjId = (valEnd == std::string::npos)
						? newUrl.substr(valStart)
						: newUrl.substr(valStart, valEnd - valStart);

					if (!bjId.empty()) {
						char inBuf[256] = { 0 };
						char outBuf[1024] = { 0 };

						size_t copyLen = std::min(bjId.size(), sizeof(inBuf) - 1);
						memcpy(inBuf, bjId.data(), copyLen);
						inBuf[copyLen] = '\0';

						int encryptedLen = soop_crypt_encrypt(inBuf, outBuf, (int)sizeof(outBuf));
						if (encryptedLen > 0 && encryptedLen < (int)sizeof(outBuf)) {
							std::string enc(outBuf, outBuf + encryptedLen);
							newUrl.replace(pos, kKey.size() + bjId.size(),
								std::string("szBroadId=") + enc);
						}
						else {
							break;
						}
					}
				}
			}
			settings["url"] = newUrl;
		}
		else {

			std::string source_type = "soop_chat_source_" + dowoomiType;
			out["id"] = source_type;

			settings["style_setting"] = path;
		}

		out["settings"] = settings;
		out["monitoring_type"] = OBS_MONITORING_TYPE_MONITOR_ONLY;
		result = true;
	}
	break;

	case 16:
	{
		std::string source_type = "soop_videoballoon_source";

		out["id"] = source_type;
		out["name"] = in["NAME"].string_value();

		out["settings"] = settings;
		out["monitoring_type"] = OBS_MONITORING_TYPE_MONITOR_ONLY;
		result = true;
	}
	break;
	case 17:
	{

	}
	break;
	case 20:
	{
		std::string source_type = "painter_source";

		out["id"] = source_type;
		out["name"] = in["NAME"].string_value();

		int BaseCX = config_get_int(ACTIVECONFIG, "Video", "BaseCX");
		int BaseCY = config_get_int(ACTIVECONFIG, "Video", "BaseCY");

		settings["width"] = BaseCX;
		settings["height"] = BaseCY;

		out["settings"] = settings;
		result = true;
	}
	break;
	case 21:
	{

	}
	break;
	case 23:
	{
		auto importer = static_cast<FreecShotImporter*>(parent);
		if (importer == nullptr)
			break;

		int browser_key = in["ANIMATIONTEXT"]["KEY"].int_value();
		result = importer->setBrowserUUID(browser_key);
		uuid = importer->getBrowserUUID(browser_key);
		if (result == false) {
			result = true;
			break;
		}

		out["uuid"] = uuid;

		out["id"] = "soop_chat_source_anmSubtitle";
		const std::string path = in["ANIMATIONTEXT"]["DOWOOMI_PATH"].string_value();
		const std::string style = in["ANIMATIONTEXT"]["DOWOOMI_STYLE"].string_value();

		out["name"] = in["NAME"].string_value();

		settings["skin_key"] = path;

		std::regex re("[0-9]+");
		std::smatch match;
		if (std::regex_search(style, match, re)) {
			int index = std::stoi(match.str(0)) - 1;
			settings["style_index"] = index;
		}
		else {
			settings["style_index"] = 0;
		}

		settings["width"] = in["ANIMATIONTEXT"]["WIDTH"].int_value();
		settings["height"] = in["ANIMATIONTEXT"]["HEIGHT"].int_value();
		out["settings"] = settings;
		result = true;
	}
	break;
	case 28:
	{
		out["id"] = "soop_spout2";
		out["name"] = in["NAME"].string_value();

		settings["spout2_sender"] = in["SPOUT"]["SENDER_NAME"].string_value();
		out["settings"] = settings;
		result = true;
	}
	break;
	case 29:
	{
		auto importer = static_cast<FreecShotImporter*>(parent);
		if (importer == nullptr)
			break;

		int browser_key = in["LIVECOMMERCE_GOAL"]["KEY"].int_value();
		result = importer->setBrowserUUID(browser_key);
		uuid = importer->getBrowserUUID(browser_key);
		if (result == false) {
			result = true;
			break;
		}
		out["uuid"] = uuid;

		out["id"] = "soop_commerce_source_goal";
		out["name"] = in["NAME"].string_value();

		settings["width"] = in["LIVECOMMERCE_GOAL"]["WIDTH"].int_value();
		settings["height"] = in["LIVECOMMERCE_GOAL"]["HEIGHT"].int_value();
		out["settings"] = settings;
		out["monitoring_type"] = OBS_MONITORING_TYPE_MONITOR_ONLY;
		result = true;
	}
	break;
	case 30:
	{
		auto importer = static_cast<FreecShotImporter*>(parent);
		if (importer == nullptr)
			break;

		int browser_key = in["LIVECOMMERCE_RANK"]["KEY"].int_value();
		result = importer->setBrowserUUID(browser_key);
		uuid = importer->getBrowserUUID(browser_key);
		if (result == false) {
			result = true;
			break;
		}
		out["uuid"] = uuid;

		out["id"] = "soop_commerce_source_rank";
		out["name"] = in["NAME"].string_value();

		settings["width"] = in["LIVECOMMERCE_RANK"]["WIDTH"].int_value();
		settings["height"] = in["LIVECOMMERCE_RANK"]["HEIGHT"].int_value();
		out["settings"] = settings;
		out["monitoring_type"] = OBS_MONITORING_TYPE_MONITOR_ONLY;
		result = true;
	}
	break;
	case 36:
	{
		char szClassname[256] = { 0, };
#ifdef _WIN32
		DWORD pID = in["APP_AUDIO"]["PID"].int_value();
		HWND hwnd = FindWindowHandleByProcessName(pID);
		if (hwnd) {
			GetClassNameA(hwnd, szClassname, sizeof(szClassname) / sizeof(char));
		}
#endif
		out["id"] = "wasapi_process_output_capture";
		out["name"] = in["NAME"].string_value();

		std::string name = in["APP_AUDIO"]["TEXT"].string_value() + ":" + szClassname + ":" + in["APP_AUDIO"]["NAME"].string_value() + ".exe";
		settings["window"] = name;        
		settings["priority"] = 2; // WINDOW_PRIORITY_EXE
		out["settings"] = settings;
		result = true;
	}
	break;
	case 99:
	{
		if (is_split_group && is_representative) {
			Json::object srcSettings;

			if (in["__GROUP_DONOR"].is_object()) {
				const Json& donor = in["__GROUP_DONOR"];
				const Json& dev = donor["DEVICE"];
				if (dev.is_object()) {
					const std::string friendly = dev["VIDEO_FRIENDLY_NAME"].string_value();
					const std::string path = dev["VIDEO_DEVICE_PATH"].string_value();

					auto importer = static_cast<FreecShotImporter*>(parent);
					if (importer == nullptr)
						break;

					std::string device_id =
						EncodeDShowIdPart(friendly) + ":" +
						EncodeDShowIdPart(path);

					out["name"] = in["NAME"].string_value();
					out["id"] = "dshow_input";

					srcSettings["video_device_id"] = device_id;
					srcSettings["last_video_device_id"] = device_id;

					const int w = dev["WIDTH"].int_value();
					const int h = dev["HEIGHT"].int_value();
					if (w > 0 && h > 0) {
						const std::string res = std::to_string(w) + "x" + std::to_string(h);
						srcSettings["resolution"] = res;
						srcSettings["last_resolution"] = res;
					}

					auto fps = dev["FPS"].number_value();
					if (fps > 0) {
						int frame_intreval = static_cast<int>(static_cast<double>(10000000) / fps); // hns
						srcSettings["frame_interval"] = frame_intreval;
					}

					auto media_type = dev["MEDIA_TYPE"].string_value();
					std::wstring wstr(media_type.begin(), media_type.end());
					GUID guid;
					HRESULT hr = CLSIDFromString(wstr.c_str(), &guid);
					if (SUCCEEDED(hr)) {
						int format = 0; // VideoFormat::Any

						if (IsEqualGUID(guid, MEDIASUBTYPE_ARGB32))
							format = 100; // VideoFormat::ARGB
						else if (IsEqualGUID(guid, MEDIASUBTYPE_RGB32))
							format = 101; // VideoFormat::XRGB
						else if (IsEqualGUID(guid, MEDIASUBTYPE_RGB24))
							format = 102; // VideoFormat::RGB24
						else if (IsEqualGUID(guid, { 0x30323449, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} })) // MEDIASUBTYPE_I420
							format = 200; // VideoFormat::I420
						else if (IsEqualGUID(guid, MEDIASUBTYPE_NV12))
							format = 201; // VideoFormat::NV12
						else if (IsEqualGUID(guid, MEDIASUBTYPE_YV12))
							format = 202; // VideoFormat::YV12
						else if (IsEqualGUID(guid, { 0x30303859, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} })) // MEDIASUBTYPE_Y800
							format = 203; // VideoFormat::Y800
						else if (IsEqualGUID(guid, MEDIASUBTYPE_P010))
							format = 204; // VideoFormat::P010
						else if (IsEqualGUID(guid, MEDIASUBTYPE_YVYU))
							format = 300; // VideoFormat::YVYU
						else if (IsEqualGUID(guid, MEDIASUBTYPE_YUY2))
							format = 301; // VideoFormat::YUY2
						else if (IsEqualGUID(guid, MEDIASUBTYPE_UYVY))
							format = 302; // VideoFormat::UYVY
						else if (IsEqualGUID(guid, { 0x43594448, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} })) // MEDIASUBTYPE_HDYC
							format = 303; // VideoFormat::HDYC
						else if (IsEqualGUID(guid, MEDIASUBTYPE_MJPG))
							format = 400; // VideoFormat::MJPEG
						else if (IsEqualGUID(guid, MEDIASUBTYPE_H264))
							format = 401; // VideoFormat::H264
						else if (IsEqualGUID(guid, { 0x43564548, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} })) // MEDIASUBTYPE_HEVC
							format = 402; // VideoFormat::HEVC

						srcSettings["video_format"] = format;
					}
					device_id = dev["AUDIO_CAPTURE_FRIENDLY_NAME"].string_value();
					if (device_id.compare("N") != 0)
					{
						srcSettings["use_custom_audio_device"] = true;
						srcSettings["audio_device_id"] = EncodeDShowIdPart(device_id) + ":";
					}

					srcSettings["res_type"] = 1;

					auto audio_render_display_name = dev["AUDIO_RENDER_DISPLAY_NAME"].string_value();
					int monitoring_type = 0;
					if (!audio_render_display_name.compare("D"))
						monitoring_type = 1;

					out["monitoring_type"] = monitoring_type;
				}
			}

			srcSettings["split_filter_name"] = "soop_split_shader_filter";

			int deco_type = -1;
			if (in["__GROUP_CHOSEN_TYPE"].is_number())
				deco_type = in["__GROUP_CHOSEN_TYPE"].int_value();
			else if (in["VEFFECT"].is_object())
				deco_type = in["VEFFECT"]["DECO_DIVISION_TYPE"].int_value();

			const std::string preset = deco_to_preset(deco_type);
			const auto [rx, ry] = deco_to_repeat(deco_type);

			Json::object filterSettings;
			if (!preset.empty()) {
				filterSettings["preset"] = preset;
				filterSettings["pre_split_effect_type"] = preset;
			}
			filterSettings["repeat_x"] = rx;
			filterSettings["repeat_y"] = ry;

			switch (deco_type) {
			case 8100:
			case 8105:
				filterSettings["crop_left_ratio"] = 25;
				filterSettings["crop_right_ratio"] = 25;
				filterSettings["crop_top_ratio"] = 0;
				filterSettings["crop_bottom_ratio"] = 0;
				if (deco_type == 8105) {
					filterSettings["flip_odd_index_horizontally"] = true;
				}
				break;

			case 8101:
			case 8106:
				filterSettings["crop_left_ratio"] = 33;
				filterSettings["crop_right_ratio"] = 33;
				filterSettings["crop_top_ratio"] = 0;
				filterSettings["crop_bottom_ratio"] = 0;
				if (deco_type == 8106) {
					filterSettings["colormap"] = "colormap_rgb";
				}
				break;
								
			case 8102:
				filterSettings["crop_left_ratio"] = 37;
				filterSettings["crop_right_ratio"] = 37;
				filterSettings["crop_top_ratio"] = 0;
				filterSettings["crop_bottom_ratio"] = 0;
				break;

				
			case 8103:
				filterSettings["crop_left_ratio"] = 40;
				filterSettings["crop_right_ratio"] = 40;
				filterSettings["crop_top_ratio"] = 0;
				filterSettings["crop_bottom_ratio"] = 0;
				break;

				
			case 8104:
				filterSettings["crop_left_ratio"] = 33;
				filterSettings["crop_right_ratio"] = 33;
				filterSettings["crop_top_ratio"] = 25;
				filterSettings["crop_bottom_ratio"] = 25;
				break;

			default:				
				break;
			}

			Json::object filterObj;
			filterObj["name"] = "soop_split_shader_filter";
			filterObj["id"] = "soop_shader_filter";
			filterObj["versioned_id"] = "soop_shader_filter";
			filterObj["settings"] = filterSettings;

			Json::array filters;
			filters.push_back(filterObj);

			out["settings"] = srcSettings;
			out["filters"] = filters;

			return true;
		}
		return false;
	}

	break;
	default :
		break;
	};
	
	const auto& audio = in["AUDIO"];
	if (!audio.is_null())
	{
		auto is_number = audio["DELAY"].is_number();
		auto sync = is_number ? audio["DELAY"].int_value() : 0; // ms
		if (sync < -950)
			sync = -950;
		else if (sync > 20000)
			sync = 20000;

		sync *= 1000000; // ms -> ns

		out["sync"] = sync;

		is_number = audio["MONO_MIX"].is_number();
		auto flags = is_number ? audio["MONO_MIX"].int_value() : 0;
		if (flags != 0)
			flags = 2; 

		out["flags"] = flags;

		is_number = audio["VOLUME"].is_number();
		auto volume = is_number ? (float)audio["VOLUME"].int_value() / (float)100 : (float)1;
		if (volume < (float)0)
			volume = (float)0;
		else if (volume > (float)20)
			volume = (float)20;

		out["volume"] = volume;

		is_number = audio["MUTE"].is_number();
		auto muted = is_number ? (audio["MUTE"].int_value() ? true : false) : false;
		out["muted"] = muted;

		auto filters = Json::array{};
		auto settings = Json::object{};
		if (audio["GAIN"].is_number())
		{
			settings["type"] = true; // %
			auto per = audio["GAIN"].int_value();

			if (per < 0)
				per = 0;
			else if (per > 3200)
				per = 3200;

			settings["per"] = per;
		}

		std::string filter_name = "게인";
		std::string try_name = filter_name;
		auto importer = static_cast<FreecShotImporter*>(parent);
		if (importer) {
			int n = 1;
			while (importer->existSourceName(try_name)) {
				try_name = filter_name + " " + std::to_string(n++);
			}
		}
		Json::object gain_filter = Json::object{
			{"name", try_name},
			{"id", "gain_filter"},
			{"versioned_id", "gain_filter"},
			{"settings", settings},
			{"enabled", true},
		};
		filters.push_back(gain_filter);
		out["filters"] = filters;
	}

	return result;
}

static Json::object parse_sources(const Json::array& in, Json::array& out, std::string scenename, int sceneidx, void* parent)
{
	Json::object res;
	res["id"] = "scene";

	std::string try_name = scenename;
	auto importer = static_cast<FreecShotImporter*>(parent);
	if (importer) {
		int n = 1;
		while (importer->existSourceName(try_name)) {
			try_name = scenename + " " + std::to_string(n++);
		}
	}
	res["name"] = try_name;	

	Json::object settings_obj;
	Json::array sources_items;

	std::map<std::string, std::vector<int>> group_idxs;   // gidx -> indices in 'in'
	std::map<std::string, int>              rep_idx;      // gidx -> representative index
	std::map<std::string, int>              chosen_type;  // gidx -> 810x
	std::map<std::string, std::string>      chosen_target;// gidx -> target
	std::map<std::string, bool>             split_groups; // gidx -> true

	for (int i = 0; i < (int)in.size(); ++i) {
		const Json& src = in[i];
		const int classindex = src["CLASSINDEX"].int_value();

		std::string gidx;
		if (classindex == 99) {
			if (src["GROUP"].is_object())
				gidx = src["GROUP"]["INDEX"].string_value();
		}
		else {
			if (src["LAYOUT"].is_object())
				gidx = src["LAYOUT"]["GROUP_INDEX"].string_value();
		}

		if (gidx.empty())
			continue;

		group_idxs[gidx].push_back(i);

		if (!rep_idx.count(gidx) && classindex != 99)
			rep_idx[gidx] = i;

		if (src["VEFFECT"].is_object()) {
			int t = src["VEFFECT"]["DECO_DIVISION_TYPE"].int_value();
			if (t >= 8100 && t <= 8106) {
				split_groups[gidx] = true;
				if (!chosen_type.count(gidx) || t > chosen_type[gidx])
					chosen_type[gidx] = t;
				if (!chosen_target.count(gidx)) {
					std::string cand = src["VEFFECT"]["DECO_TARGET"].string_value();
					if (!cand.empty()) chosen_target[gidx] = cand;
				}
			}
		}
	}

	for (auto& kv : group_idxs) {
		const std::string& gidx = kv.first;
		const auto& idxs = kv.second;
		const bool is_split = (split_groups.count(gidx) > 0);

		if (is_split) {
			int idx99 = -1;
			for (int i : idxs) {
				if (in[i]["CLASSINDEX"].int_value() == 99) {
					idx99 = i; break;
				}
			}
			if (idx99 >= 0) {
				rep_idx[gidx] = idx99;
			}
			else {
				rep_idx.erase(gidx);
			}
		}
		else {
			if (!rep_idx.count(gidx) && !idxs.empty())
				rep_idx[gidx] = idxs.front();
		}
	}

	res["settings"] = settings_obj;
	return res;
}


int FreecShotImporter::ImportScenes(const string &path, string &name, Json &res)
{
	BPtr<char> file_data = os_quick_read_utf8_file(path.c_str());
	if (!file_data)
		return IMPORTER_FILE_WONT_OPEN;

	if (name.empty())
		name = "FreecShot Import";
	source_name_set.clear();
	
	std::string out_str = json11::Json(res).dump();
	QDir dir(path.c_str());

	TranslateOSStudio(res);
	TranslatePaths(res, QDir::cleanPath(dir.filePath("..")).toStdString());

	return IMPORTER_SUCCESS;
}

bool FreecShotImporter::Check(const string &path)
{
	BPtr<char> file_data = os_quick_read_utf8_file(path.c_str());

	if (!file_data)
		return false;

	bool check = false;
	char szStudioJson[256] = { 0, };
	strcpy(szStudioJson, "{\n    \"MAINFRM\":");
	if (strncmp(file_data, szStudioJson, strlen(szStudioJson)) == 0)
		check = true;

	return check;
}

OBSImporterFiles FreecShotImporter::FindFiles()
{
	OBSImporterFiles res;
#ifdef _WIN32
	wchar_t path_utf16[MAX_PATH];
	char path_utf8[MAX_PATH] = {};

	SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT,
		path_utf16);

	os_wcs_to_utf8(path_utf16, wcslen(path_utf16), path_utf8, MAX_PATH);

	std::string romaingpath = std::string(path_utf8)+ "\\afreeca\\freecshot\\settings\\studio2.json";
	if (access(romaingpath.c_str(), 0) == -1)
		return res;

	res.push_back(romaingpath);
#endif
	return res;
}

bool FreecShotImporter::setBrowserUUID(int browser_key) {
	std::string uuid;
	auto it = browserMap.find(browser_key);
	if (it == browserMap.end())
	{
		uuid = os_generate_uuid();
		browserMap[browser_key] = uuid;
	}

	return uuid.length() != 0 ? true : false;
}

bool FreecShotImporter::existSourceName(std::string name) {
	if (source_name_set.find(name) != source_name_set.end()) {
		return true;
	}
	else {		
		source_name_set.insert(name);
		return false;
	}
}