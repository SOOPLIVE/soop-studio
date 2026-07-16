#include "CImageSourceToolbar.h"
#include "ui_image-source-toolbar.h"

#include <QFileInfo>
#include <QPushButton>

#include "qt-wrappers.hpp"

#include "Application/CApplication.h"

#include "CoreModel/Source/CSource.h"

#include "MainFrame/CMainFrame.h"

AFQImageSourceToolbar::AFQImageSourceToolbar(QWidget* parent, OBSSource source) :
	CBaseSourceToolbar(parent, source),
	m_signalFileLoad(obs_source_get_signal_handler(source), "media_file_load",
		AFQImageSourceToolbar::FSMediaFileLoaded, this),
	ui(new Ui::AFQImageSourceToolbar)
{
	ui->setupUi(this);

	connect(ui->pushButton_ImagePath, &QPushButton::clicked,
			this, &AFQImageSourceToolbar::_qslotBrowseImagePathClicked);

	//obs_module_t* mod = obs_get_module("image-source");
	//ui->editPath->setText(obs_module_get_locale_text(mod, "File"));

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	std::string file = obs_data_get_string(settings, "file");

	ui->lineEdit_Path->setText(file.c_str());
}

AFQImageSourceToolbar::~AFQImageSourceToolbar()
{
	delete ui;
}

void AFQImageSourceToolbar::FSMediaFileLoaded(void* data, calldata_t* params)
{
	OBSSource source((obs_source_t*)calldata_ptr(params, "source"));

	int width = calldata_int(params, "width");
	int height = calldata_int(params, "height");

	QMetaObject::invokeMethod(
		qApp, [source, width, height]() {

			QString filePath;
			std::string id = obs_source_get_id(source);
			OBSDataAutoRelease settings = obs_source_get_settings(source);

			OBSSceneItemAutoRelease item = obs_scene_sceneitem_from_source(SCENE_CONTEXT.GetCurrentScene(), source);

			if (!item)
				return;

			obs_transform_info oti;
			obs_sceneitem_get_info(item, &oti);

			uint32_t canvasW = (uint32_t)config_get_uint(ACTIVECONFIG, "Video", "BaseCX");
			uint32_t canvasH = (uint32_t)config_get_uint(ACTIVECONFIG, "Video", "BaseCY");


			const float adjustScaleW = 1.0f / 2.0f;
			const float adjustScaleH = 2.0f / 3.0f;

			if (oti.scale.x == 0.0f || oti.scale.y == 0.0f) {
				float targetScale = 1.0f;
				if (width >= height) {
					const float adjustW = canvasW * adjustScaleW;
					if (width > adjustW)
						targetScale = adjustW / (float)width;
				}
				else {
					const float adjustH = canvasH * adjustScaleH;
					if (height > adjustH)
						targetScale = adjustH / (float)height;
				}
				if (oti.scale.x != targetScale || oti.scale.y != targetScale) {
					oti.scale.x = oti.scale.y = targetScale;
					obs_sceneitem_set_info(item, &oti);
				}
			}

			if (0 == id.compare("image_source")) {
				const char* file = obs_data_get_string(settings, "file");
				filePath = QString::fromUtf8(file);
			}
			else {
				return;
			}

			bool is_changed_name = obs_data_get_bool(settings, "is_changed_name");
			if (!is_changed_name)
			{
				if (!filePath.isEmpty()) {

					QString fileName = QFileInfo(filePath).fileName();
					QString currentName = QString::fromUtf8(obs_source_get_name(source));

					if (0 == currentName.compare(fileName))
						return;

					QString displayText = fileName;

					// check source name 
					int i = 2;
					while (true) {
						OBSSourceAutoRelease s =
							obs_get_source_by_name(QT_TO_UTF8(displayText));

						if (!s || s == source)
							break;

						displayText = QString("%1 %2").arg(fileName).arg(i++);
					}

					if (displayText != currentName) {

						std::string prevName = obs_source_get_name(source);
						std::string newName = currentName.toStdString();

						UNDO_STACK.AddActionRename(prevName, newName, source);

						obs_source_set_name(source, displayText.toUtf8().constData());
					}
				}
			}

		}, Qt::QueuedConnection);
}

void AFQImageSourceToolbar::_qslotBrowseImagePathClicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	obs_property_t* p = obs_properties_get(props.get(), "file");
	const char* desc = obs_property_description(p);
	const char* filter = obs_property_path_filter(p);
	const char* default_path = obs_property_path_default_path(p);

	QString startDir = ui->lineEdit_Path->text();
	if(startDir.isEmpty())
		startDir = default_path;

	QString path = OpenFile(this, desc, startDir, filter);
	if (path.isEmpty()) {
		return;
	}

	ui->lineEdit_Path->setText(path);

	SaveOldProperties(source);
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "file", QT_TO_UTF8(path));
	obs_source_update(source, settings);

    QString filePath = path;
    if (!filePath.isEmpty()) {

        QString fileName = QFileInfo(filePath).fileName();
        QString currentName = QString::fromUtf8(obs_source_get_name(source));

		if (0 != currentName.compare(fileName))
		{
			QString displayText = fileName;

			// check source name 
			int i = 2;
			while (true) {
				OBSSourceAutoRelease s =
					obs_get_source_by_name(QT_TO_UTF8(displayText));

				if (!s || s == source)
					break;

				displayText = QString("%1 %2").arg(fileName).arg(i++);
			}

			if (displayText != currentName) {

				std::string prevName = obs_source_get_name(source);
				std::string newName = currentName.toStdString();

				UNDO_STACK.AddActionRename(prevName, newName, source);

				obs_source_set_name(source, displayText.toUtf8().constData());
			}
		}
    }

	SetUndoProperties(source);
}