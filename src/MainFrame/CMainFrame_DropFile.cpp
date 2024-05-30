﻿#include "CMainFrame.h"
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QUrlQuery>
#ifdef _WIN32
#include <QSettings>
#endif
#include <string>

#include "qt-wrappers.hpp"

#include "CoreModel/Scene/CSceneContext.h"
#include "Application/CApplication.h"
#include "UIComponent/CMessageBox.h"

static const char* textExtensions[] = {"txt", "log", nullptr};

static const char* imageExtensions[] = {"bmp", "gif", "jpeg", "jpg",
#ifdef _WIN32
                    "jxr",
#endif
                    "png", "tga", "webp", nullptr};

static const char* htmlExtensions[] = {"htm", "html", nullptr};

static const char* mediaExtensions[] = {
    "3ga",   "669",   "a52",   "aac",  "ac3",  "adt",  "adts", "aif",
    "aifc",  "aiff",  "amb",   "amr",  "aob",  "ape",  "au",   "awb",
    "caf",   "dts",   "flac",  "it",   "kar",  "m4a",  "m4b",  "m4p",
    "m5p",   "mid",   "mka",   "mlp",  "mod",  "mpa",  "mp1",  "mp2",
    "mp3",   "mpc",   "mpga",  "mus",  "oga",  "ogg",  "oma",  "opus",
    "qcp",   "ra",    "rmi",   "s3m",  "sid",  "spx",  "tak",  "thd",
    "tta",   "voc",   "vqf",   "w64",  "wav",  "wma",  "wv",   "xa",
    "xm",    "3g2",   "3gp",   "3gp2", "3gpp", "amv",  "asf",  "avi",
    "bik",   "crf",   "divx",  "drc",  "dv",   "evo",  "f4v",  "flv",
    "gvi",   "gxf",   "iso",   "m1v",  "m2v",  "m2t",  "m2ts", "m4v",
    "mkv",   "mov",   "mp2",   "mp2v", "mp4",  "mp4v", "mpe",  "mpeg",
    "mpeg1", "mpeg2", "mpeg4", "mpg",  "mpv2", "mts",  "mtv",  "mxf",
    "mxg",   "nsv",   "nuv",   "ogg",  "ogm",  "ogv",  "ogx",  "ps",
    "rec",   "rm",    "rmvb",  "rpl",  "thp",  "tod",  "ts",   "tts",
    "txd",   "vob",   "vro",   "webm", "wm",   "wmv",  "wtv",  nullptr};

static std::string GenerateSourceName(const char* base)
{
    std::string name;
    int inc = 0;

    for(;; inc++) {
        name = base;

        if(inc) {
            name += " (";
            name += std::to_string(inc + 1);
            name += ")";
        }

        OBSSourceAutoRelease source =
            obs_get_source_by_name(name.c_str());

        if(!source)
            return name;
    }
}

#ifdef _WIN32
static QString ReadWindowsURLFile(const QString& file)
{
    QSettings iniFile(file, QSettings::IniFormat);
    QVariant url = iniFile.value("InternetShortcut/URL");
    return url.toString();
}
#endif

void AFMainFrame::_AddDropSource(const char* data, DropType image)
{
    OBSDataAutoRelease settings = obs_data_create();
    const char* type = nullptr;
    QString name;

    obs_video_info ovi;
    obs_get_video_info(&ovi);

    switch(image) {
        case DropType_RawText:
            obs_data_set_string(settings, "text", data);
#ifdef _WIN32
            type = "text_gdiplus";
#else
            type = "text_ft2_source";
#endif
            break;
        case DropType_Text:
#ifdef _WIN32
            obs_data_set_bool(settings, "read_from_file", true);
            obs_data_set_string(settings, "file", data);
            name = QUrl::fromLocalFile(QString(data)).fileName();
            type = "text_gdiplus";
#else
            obs_data_set_bool(settings, "from_file", true);
            obs_data_set_string(settings, "text_file", data);
            type = "text_ft2_source";
#endif
            break;
        case DropType_Image:
            obs_data_set_string(settings, "file", data);
            name = QUrl::fromLocalFile(QString(data)).fileName();
            type = "image_source";
            break;
        case DropType_Media:
            obs_data_set_string(settings, "local_file", data);
            name = QUrl::fromLocalFile(QString(data)).fileName();
            type = "ffmpeg_source";
            break;
        case DropType_Html:
            obs_data_set_bool(settings, "is_local_file", true);
            obs_data_set_string(settings, "local_file", data);
            obs_data_set_int(settings, "width", ovi.base_width);
            obs_data_set_int(settings, "height", ovi.base_height);
            //name = QUrl::fromLocalFile(QString(data)).fileName();
            type = "browser_source";
            break;
        case DropType_Url:
            _AddDropURL(data, name, settings, ovi);
            type = "browser_source";
            break;
    }

    type = obs_get_latest_input_type_id(type);

    if(type == nullptr || !obs_source_get_display_name(type)) {
        return;
    }

    //if (name.isEmpty())
    //name = obs_source_get_display_name(type);
    QString displayText = AFSourceUtil::GetPlaceHodlerText(type);

    std::string sourceName = GenerateSourceName(QT_TO_UTF8(displayText));
    OBSSourceAutoRelease source =
        obs_source_create(type, sourceName.c_str(), settings, nullptr);
    if(source) {
        AFMainDynamicComposit* mainComposit = GetMainWindow();
        OBSDataAutoRelease wrapper = obs_save_source(source);

        AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
        OBSScene scene = sceneContext.GetCurrOBSScene();
        std::string sceneUUID =
            obs_source_get_uuid(obs_scene_get_source(scene));
        std::string sourceUUID = obs_source_get_uuid(source);

        auto undo = [sceneUUID, sourceUUID,
            mainComposit](const std::string&) {
            OBSSourceAutoRelease source = obs_get_source_by_uuid(sourceUUID.c_str());
            obs_source_remove(source);
            OBSSourceAutoRelease scene = obs_get_source_by_uuid(sceneUUID.c_str());
            mainComposit->SetCurrentScene(scene.Get(), true);
        };
        auto redo = [sceneUUID, sourceName, type,
            mainComposit](const std::string& data) {
            OBSSourceAutoRelease scene = obs_get_source_by_uuid(sceneUUID.c_str());
            mainComposit->SetCurrentScene(scene.Get(), true);

            OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
            OBSSourceAutoRelease source = obs_load_source(dat);
            obs_scene_add(obs_scene_from_source(scene),
                source.Get());
        };

        m_undo_s.AddAction(QTStr("Undo.Add").arg(sourceName.c_str()),
                           undo, redo, "", std::string(obs_data_get_json(wrapper)));
        obs_scene_add(scene, source);
    }
}

void AFMainFrame::_AddDropURL(const char* url, QString& name, obs_data_t* settings,
                              const obs_video_info& ovi)
{
    QUrl path = QString::fromUtf8(url);
    QUrlQuery query = QUrlQuery(path.query(QUrl::FullyEncoded));

    int cx = (int)ovi.base_width;
    int cy = (int)ovi.base_height;

    if(query.hasQueryItem("layer-width"))
        cx = query.queryItemValue("layer-width").toInt();
    if(query.hasQueryItem("layer-height"))
        cy = query.queryItemValue("layer-height").toInt();
    if(query.hasQueryItem("layer-css")) {
        // QUrl::FullyDecoded does NOT properly decode a
        // application/x-www-form-urlencoded space represented as '+'
        // Thus, this is manually filtered out before QUrl's
        // decoding kicks in again. This is to allow JavaScript's
        // default searchParams.append function to simply append css
        // to the query parameters, which is the intended usecase for this.
        QString fullyEncoded =
            query.queryItemValue("layer-css", QUrl::FullyEncoded);
        fullyEncoded = fullyEncoded.replace("+", "%20");
        QString decoded = QUrl::fromPercentEncoding(
            QByteArray::fromStdString(QT_TO_UTF8(fullyEncoded)));
        obs_data_set_string(settings, "css", QT_TO_UTF8(decoded));
    }

    obs_data_set_int(settings, "width", cx);
    obs_data_set_int(settings, "height", cy);

    name = query.hasQueryItem("layer-name")
        ? query.queryItemValue("layer-name", QUrl::FullyDecoded)
        : path.host();

    query.removeQueryItem("layer-width");
    query.removeQueryItem("layer-height");
    query.removeQueryItem("layer-name");
    query.removeQueryItem("layer-css");
    path.setQuery(query);

    obs_data_set_string(settings, "url", QT_TO_UTF8(path.url()));
}

void AFMainFrame::_ConfirmDropUrl(const QString& url)
{
    if(url.left(7).compare("http://", Qt::CaseInsensitive) == 0 ||
        url.left(8).compare("https://", Qt::CaseInsensitive) == 0) {

        activateWindow();

        QString msg = QTStr("AddUrl.Text");
        msg += "\n\n";
        msg += QTStr("AddUrl.Text.Url").arg(url);

        int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this, "", msg);

        if(result == QDialog::Accepted)
            _AddDropSource(QT_TO_UTF8(url), DropType_Url);
    }
}

void AFMainFrame::dragEnterEvent(QDragEnterEvent* event)
{
    // refuse drops of our own widgets
    if(event->source() != nullptr) {
        event->setDropAction(Qt::IgnoreAction);
        return;
    }

    event->acceptProposedAction();
}

void AFMainFrame::dragLeaveEvent(QDragLeaveEvent* event)
{
    event->accept();
}

void AFMainFrame::dragMoveEvent(QDragMoveEvent* event)
{
    event->acceptProposedAction();
}

void AFMainFrame::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();

    if(mimeData->hasUrls()) {
        QList<QUrl> urls = mimeData->urls();

        for(int i = 0; i < urls.size(); i++) {
            QUrl url = urls[i];
            QString file = url.toLocalFile();
            QFileInfo fileInfo(file);

            if(!fileInfo.exists()) {
                _ConfirmDropUrl(url.url());
                continue;
            }

#ifdef _WIN32
            if(fileInfo.suffix().compare(
                "url", Qt::CaseInsensitive) == 0) {
                QString urlTarget = ReadWindowsURLFile(file);
                if(!urlTarget.isEmpty()) {
                    _ConfirmDropUrl(urlTarget);
                }
                continue;
            } else if(fileInfo.isShortcut()) {
                file = fileInfo.symLinkTarget();
                fileInfo = QFileInfo(file);
                if(!fileInfo.exists()) {
                    continue;
                }
            }
#endif

            QString suffixQStr = fileInfo.suffix();
            QByteArray suffixArray = suffixQStr.toUtf8();
            const char* suffix = suffixArray.constData();
            bool found = false;

            const char** cmp;

#define CHECK_SUFFIX(extensions, type)                         \
	cmp = extensions;                                          \
	while (*cmp) {                                             \
		if (astrcmpi(*cmp, suffix) == 0) {                     \
			_AddDropSource(QT_TO_UTF8(file), type);             \
			found = true;                                      \
			break;                                             \
		}                                                      \
                                                               \
		cmp++;                                                 \
	}                                                          \
                                                               \
	if (found)                                                 \
		continue;

            CHECK_SUFFIX(textExtensions, DropType_Text);
            CHECK_SUFFIX(htmlExtensions, DropType_Html);
            CHECK_SUFFIX(imageExtensions, DropType_Image);
            CHECK_SUFFIX(mediaExtensions, DropType_Media);

#undef CHECK_SUFFIX
        }
    } else if(mimeData->hasText()) {
        _AddDropSource(QT_TO_UTF8(mimeData->text()), DropType_RawText);
    }

}
