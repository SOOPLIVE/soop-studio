#pragma once

#include <thread>

#include <Qobject>
#include <QImage>

#include "obs.hpp"

class ScreenShotObj : public QObject {
	Q_OBJECT

public:
	ScreenShotObj(obs_source_t* source);
	ScreenShotObj(obs_source_t* source, bool internalSave);
	~ScreenShotObj() override;
	void Screenshot();
	void Download();
	void Copy();
	void MuxAndFinish();
	QPixmap GetPixmap();

	gs_texrender_t* texrender = nullptr;
	gs_stagesurf_t* stagesurf = nullptr;
	OBSWeakSource weakSource;
	std::string path;
	QImage image;
	std::vector<uint8_t> half_bytes;
	uint32_t cx;
	uint32_t cy;
	std::thread th;

	int stage = 0;

signals:
	void ScreenShotFinished();

private slots:
	void Save();

private:
	int internalSave = false;
};
