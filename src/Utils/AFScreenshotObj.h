#pragma once

#include <thread>

#include <Qobject>
#include <QImage>

#include "obs.hpp"

class AFQSceneListItem;

class AFQScreenShotObj : public QObject {
	Q_OBJECT

public:
	enum Type {
		Screenshot_SceneImage,
		Screenshot_SceneButton,
	};

public:
	AFQScreenShotObj(obs_source_t* source, 
					 AFQScreenShotObj::Type type = Screenshot_SceneImage);
	~AFQScreenShotObj() override;
	void Screenshot();
	void Download();
	void Copy();
	void MuxAndFinish();
	QPixmap GetPixmap();

	gs_texrender_t* m_texRender = nullptr;
	gs_stagesurf_t* m_stageSurf = nullptr;
	OBSWeakSource	m_weakSource;
	std::string		m_filePath;
	QImage			m_image;

	std::vector<uint8_t> m_halfBytes;
	uint32_t m_cx;
	uint32_t m_cy;
	std::thread m_thread;

	int stage = 0;

	AFQScreenShotObj::Type screenshotType = Screenshot_SceneImage;

signals:
	void qsignalSetPreview();

private slots:
	void Save();
};