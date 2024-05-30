#include "AFScreenshotObj.h"

#include <QPixmap>
#include <util/platform.h>

#include "qt-wrapper.h"

#ifdef _WIN32
#include <wincodec.h>
#include <wincodecsdk.h>
#include <wrl/client.h>
#pragma comment(lib, "windowscodecs.lib")
#endif

#include "Application/CApplication.h"
#include "MainFrame/CMainFrame.h"

#include "Common/StringMiscUtils.h"
#include "CoreModel/Config/CConfigManager.h"

#include "Blocks/SceneSourceDock/CSceneListItem.h"
#include "UIComponent/CMessageBox.h"

#define STAGE_SCREENSHOT 0
#define STAGE_DOWNLOAD 1
#define STAGE_COPY_AND_SAVE 2
#define STAGE_FINISH 3

static void ScreenshotTick(void* param, float)
{
	AFQScreenShotObj* data = reinterpret_cast<AFQScreenShotObj*>(param);

	if (data->stage == STAGE_FINISH) {
		return;
	}

	obs_enter_graphics();

	switch (data->stage) {
	case STAGE_SCREENSHOT:
		data->Screenshot();
		break;
	case STAGE_DOWNLOAD:
		data->Download();
		break;
	case STAGE_COPY_AND_SAVE:
		data->Copy();
		QMetaObject::invokeMethod(data, "Save");
		obs_remove_tick_callback(ScreenshotTick, data);
		break;
	}

	obs_leave_graphics();

	data->stage++;
}

AFQScreenShotObj::AFQScreenShotObj(obs_source_t* source, 
								   AFQScreenShotObj::Type type) :
	m_weakSource(OBSGetWeakRef(source)),
	screenshotType(type)
{
	obs_add_tick_callback(ScreenshotTick, this);
}

AFQScreenShotObj::~AFQScreenShotObj()
{
	obs_enter_graphics();
	gs_stagesurface_destroy(m_stageSurf);
	gs_texrender_destroy(m_texRender);
	obs_leave_graphics();

	obs_remove_tick_callback(ScreenshotTick, this);

	if (m_thread.joinable()) {
		m_thread.join();

		if (m_cx && m_cy) {

			if (screenshotType == AFQScreenShotObj::Type::Screenshot_SceneImage) {

				OBSSource source = OBSGetStrongRef(m_weakSource);
				const char* sceneName = obs_source_get_name(source);
				
				QString text = Str("Basic.SceneSourceDock.ScreenShot.Text"); 
				text = text.arg(sceneName).arg(QT_UTF8(m_filePath.c_str()));

				AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, App()->GetMainView(),
										   QT_UTF8(""), text, false, true);

			}

			//OBSBasic* main = OBSBasic::Get();
			//main->ShowStatusBarMessage(
			//	QTStr("Basic.StatusBar.ScreenshotSavedTo")
			//	.arg(QT_UTF8(path.c_str())));

			//main->lastScreenshot = path;

			//if (main->api)
			//	main->api->on_event(
			//		OBS_FRONTEND_EVENT_SCREENSHOT_TAKEN);
		}
	}
}

void AFQScreenShotObj::Screenshot()
{
	OBSSource source = OBSGetStrongRef(m_weakSource);

	if (source) {
		m_cx = obs_source_get_base_width(source);
		m_cy = obs_source_get_base_height(source);
	}
	else {
		obs_video_info ovi;
		obs_get_video_info(&ovi);
		m_cx = ovi.base_width;
		m_cy = ovi.base_height;
	}

	if (!m_cx || !m_cy) {
		blog(LOG_WARNING, "Cannot screenshot, invalid target size");
		obs_remove_tick_callback(ScreenshotTick, this);
		deleteLater();
		return;
	}

#ifdef _WIN32
	enum gs_color_space space =
		obs_source_get_color_space(source, 0, nullptr);
	if (space == GS_CS_709_EXTENDED) {
		/* Convert for JXR */
		space = GS_CS_709_SCRGB;
	}
#else
	/* Tonemap to SDR if HDR */
	const enum gs_color_space space = GS_CS_SRGB;
#endif
	const enum gs_color_format format = gs_get_format_from_space(space);

	m_texRender = gs_texrender_create(format, GS_ZS_NONE);
	m_stageSurf = gs_stagesurface_create(m_cx, m_cy, format);

	if (gs_texrender_begin_with_color_space(m_texRender, m_cx, m_cy, space)) {
		vec4 zero;
		vec4_zero(&zero);

		gs_clear(GS_CLEAR_COLOR, &zero, 0.0f, 0);
		gs_ortho(0.0f, (float)m_cx, 0.0f, (float)m_cy, -100.0f, 100.0f);

		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

		if (source) {
			obs_source_inc_showing(source);
			obs_source_video_render(source);
			obs_source_dec_showing(source);
		}
		else {
			obs_render_main_texture();
		}

		gs_blend_state_pop();
		gs_texrender_end(m_texRender);
	}
}

void AFQScreenShotObj::Download()
{
	gs_stage_texture(m_stageSurf, gs_texrender_get_texture(m_texRender));
}

void AFQScreenShotObj::Copy()
{
	uint8_t* videoData = nullptr;
	uint32_t videoLinesize = 0;

	if (gs_stagesurface_map(m_stageSurf, &videoData, &videoLinesize)) {
		if (gs_stagesurface_get_color_format(m_stageSurf) == GS_RGBA16F) {
			const uint32_t linesize = m_cx * 8;
			m_halfBytes.reserve(m_cx * m_cy * 8);

			for (uint32_t y = 0; y < m_cy; y++) {
				const uint8_t* const line =
					videoData + (y * videoLinesize);
				m_halfBytes.insert(m_halfBytes.end(), line,
					line + linesize);
			}
		}
		else {
			m_image = QImage(m_cx, m_cy, QImage::Format::Format_RGBX8888);

			int linesize = m_image.bytesPerLine();
			for (int y = 0; y < (int)m_cy; y++)
				memcpy(m_image.scanLine(y),
					   videoData + (y * videoLinesize),
					   linesize);
		}

		gs_stagesurface_unmap(m_stageSurf);
	}
}

#ifdef _WIN32
static HRESULT SaveJxrImage(LPCWSTR path, uint8_t* pixels, uint32_t cx,
	uint32_t cy, IWICBitmapFrameEncode* frameEncode,
	IPropertyBag2* options)
{
	wchar_t lossless[] = L"Lossless";
	PROPBAG2 bag = {};
	bag.pstrName = lossless;
	VARIANT value = {};
	value.vt = VT_BOOL;
	value.bVal = TRUE;
	HRESULT hr = options->Write(1, &bag, &value);
	if (FAILED(hr))
		return hr;

	hr = frameEncode->Initialize(options);
	if (FAILED(hr))
		return hr;

	hr = frameEncode->SetSize(cx, cy);
	if (FAILED(hr))
		return hr;

	hr = frameEncode->SetResolution(72, 72);
	if (FAILED(hr))
		return hr;

	WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat64bppRGBAHalf;
	hr = frameEncode->SetPixelFormat(&pixelFormat);
	if (FAILED(hr))
		return hr;

	if (memcmp(&pixelFormat, &GUID_WICPixelFormat64bppRGBAHalf,
		sizeof(WICPixelFormatGUID)) != 0)
		return E_FAIL;

	hr = frameEncode->WritePixels(cy, cx * 8, cx * cy * 8, pixels);
	if (FAILED(hr))
		return hr;

	hr = frameEncode->Commit();
	if (FAILED(hr))
		return hr;

	return S_OK;
}

static HRESULT SaveJxr(LPCWSTR path, uint8_t* pixels, uint32_t cx, uint32_t cy)
{
	Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(factory.GetAddressOf()));
	if (FAILED(hr))
		return hr;

	Microsoft::WRL::ComPtr<IWICStream> stream;
	hr = factory->CreateStream(stream.GetAddressOf());
	if (FAILED(hr))
		return hr;

	hr = stream->InitializeFromFilename(path, GENERIC_WRITE);
	if (FAILED(hr))
		return hr;

	Microsoft::WRL::ComPtr<IWICBitmapEncoder> encoder = NULL;
	hr = factory->CreateEncoder(GUID_ContainerFormatWmp, NULL,
		encoder.GetAddressOf());
	if (FAILED(hr))
		return hr;

	hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
	if (FAILED(hr))
		return hr;

	Microsoft::WRL::ComPtr<IWICBitmapFrameEncode> frameEncode;
	Microsoft::WRL::ComPtr<IPropertyBag2> options;
	hr = encoder->CreateNewFrame(frameEncode.GetAddressOf(),
		options.GetAddressOf());
	if (FAILED(hr))
		return hr;

	hr = SaveJxrImage(path, pixels, cx, cy, frameEncode.Get(),
		options.Get());
	if (FAILED(hr))
		return hr;

	encoder->Commit();
	return S_OK;
}
#endif // #ifdef _WIN32

void AFQScreenShotObj::MuxAndFinish()
{
	if (m_halfBytes.empty()) {
		m_image.save(QT_UTF8(m_filePath.c_str()));
		blog(LOG_INFO, "Saved screenshot to '%s'", m_filePath.c_str());
	}
	else {
#ifdef _WIN32
		wchar_t* path_w = nullptr;
		os_utf8_to_wcs_ptr(m_filePath.c_str(), 0, &path_w);
		if (path_w) {
			SaveJxr(path_w, m_halfBytes.data(), m_cx, m_cy);
			bfree(path_w);
		}
#endif // #ifdef _WIN32
	}

	deleteLater();
}


QPixmap AFQScreenShotObj::GetPixmap()
{
	if (m_halfBytes.empty())
		return QPixmap::fromImage(m_image);
	else
		return QPixmap();
}

void AFQScreenShotObj::Save()
{
	if (Screenshot_SceneImage == screenshotType) {

		AFConfigManager& config = AFConfigManager::GetSingletonInstance();

		const char* mode = config_get_string(config.GetBasic(), "Output", "Mode");
		const char* type = config_get_string(config.GetBasic(), "AdvOut", "RecType");
		const char* adv_path =
						strcmp(type, "Standard")
						? config_get_string(config.GetBasic(), "AdvOut", "FFFilePath")
						: config_get_string(config.GetBasic(), "AdvOut", "RecFilePath");

		const char* rec_path =
						strcmp(mode, "Advanced")
						? config_get_string(config.GetBasic(), "SimpleOutput", "FilePath")
						: adv_path;

		bool noSpace =
					config_get_bool(config.GetBasic(), "SimpleOutput", "FileNameWithoutSpace");
		const char* filenameFormat =
					config_get_string(config.GetBasic(), "Output", "FilenameFormatting");
		bool overwriteIfExists =
					config_get_bool(config.GetBasic(), "Output", "OverwriteIfExists");


		const char* ext = m_halfBytes.empty() ? "png" : "jxr";
		m_filePath = GetOutputFilename(
							rec_path, ext, noSpace, overwriteIfExists,
							GetFormatString(filenameFormat, "Screenshot", nullptr).c_str());

		m_thread = std::thread([this] { MuxAndFinish(); });
	}
	else {
		emit qsignalSetPreview();
	}
}