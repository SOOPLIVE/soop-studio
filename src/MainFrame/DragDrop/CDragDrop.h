#pragma once

#include <QDropEvent>

#include "../CMainFrame.h"

class CMainDragDrop : public QObject {
	Q_OBJECT

private:
	enum DropType {
		DropType_RawText,
		DropType_Text,
		DropType_Image,
		DropType_Media,
		DropType_Html,
		DropType_Url,
	};

public:
	CMainDragDrop(QObject* parent);
	~CMainDragDrop();

private:
	static void _SourceFileLoaded(void* data, calldata_t* params);

	void _AddDropSource(const char* data, DropType image);
	void _AddDropURL(const char* url, QString& name, obs_data_t* settings, const obs_video_info& ovi);
	void _ConfirmDropUrl(const QString& url);

public:
	void DropEvent(QDropEvent* event);

private:
	std::mutex m_signalMutex;
	std::unordered_map<obs_source_t*, std::unique_ptr<OBSSignal>> m_fileLoadSignals;
};
