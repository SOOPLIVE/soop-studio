#pragma once

#include <QWidget>
#include <QFrame>
#include <QTimer>
#include <QPropertyAnimation>

#include "CoreModel/SOOPSource/CSoopMediaSourceManager.h"

#define VOD_PLAYLIST_ITEM_START_X   20
#define VOD_PLAYLIST_ITEM_START_Y   4
#define VOD_PLAYLIST_ITEM_SPACE		10
#define VOD_PLAYLIST_ITEM_WIDTH		399
#define VOD_PLAYLIST_ITEM_HEIGHT	69

namespace Ui {
	class AFQVodListItem;
}

class AFQVodListItem : public QFrame
{
	Q_OBJECT

	enum STATE {
		NONE = 0,
		NORMAL,
		HOVER,
		PLAYING,
	};

public:
	AFQVodListItem(QWidget* parent = nullptr);
	~AFQVodListItem();

signals:
	void qsignalPlayVOD(VodInfo_s info);

private slots:
	void qslotImageDownloaded(QByteArray responseData);
	void qslotRecentlyHighlightTimer();
	void qslotRefreshItemUI();

protected:
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void enterEvent(QEnterEvent* event) override;
	virtual void leaveEvent(QEvent* event) override;
		
public:
	void SetVodListItemStatus(bool playing, obs_media_state mediaState);
	void SetRecentyPlayVodStatus();

	void SetVodInfo(VodInfo_s& info);
	VodInfo_s& GetVodInfo() { return m_info; };

private:
	STATE _GetVODState();

	QPixmap _MakeRecentlyPlayPixmap(const QPixmap& src);
	QPixmap _MakeDurationPixmap(const QPixmap& src, QString duration);
	QPixmap _MakeDimAndTextPixmap(const QPixmap& src, const int fontSize, const QString& text);
	QPixmap _MakeSymbolPixmap(const QPixmap& src, const QString& symbolPath);
	QPixmap _MakeRoundedPixmap(const QPixmap& src, int radius);

	void _ApplyPixmap(STATE state);

private:
	Ui::AFQVodListItem* ui;

	QTimer m_timerHighlight;

	VodInfo_s m_info;

	QPixmap m_pixmapImage;
	QPixmap m_pixmapDuration;

	bool	m_playing = false;
	bool	m_hover = false;
};
