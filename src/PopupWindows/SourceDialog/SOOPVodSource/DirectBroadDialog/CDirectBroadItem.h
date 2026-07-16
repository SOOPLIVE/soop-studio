#pragma once

#include <QFrame>

#include "CoreModel/SOOPSource/CSoopMediaSourceManager.h"

#include "MainFrame/CMainFrame.h"

namespace Ui {
	class AFQDirectBroadItem;
}

class AFQDirectBroadItem : public QFrame
{
	Q_OBJECT

public:
	AFQDirectBroadItem(QWidget* parent = nullptr);
	~AFQDirectBroadItem();

signals:
	void qsignalRequestOnetimeUrl(int idx);
	void qsignalStopDirectBroad(int idx);
	void qsignalHoverItem(QString);
	void qsignalLeaveItem();

private slots:
	void _qslotPlayButtonClicked();
	void _qslotStopButtonClicked();

protected:
	virtual void enterEvent(QEnterEvent* event) override;
	virtual void leaveEvent(QEvent* event) override;

public:
	void SetDirectBroadInfo(const DirectBroadInfo_s& info);
	DirectBroadInfo_s& GetDirectBroadInfo() { return m_directBroadInfo; };

	void SetDirectBroadPlayingStatus(bool playing);

private:
	Ui::AFQDirectBroadItem* ui;

	DirectBroadInfo_s m_directBroadInfo;
	bool m_playing = false;
};