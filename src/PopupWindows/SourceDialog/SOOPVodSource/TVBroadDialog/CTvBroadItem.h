#pragma once

#include <QFrame>

struct TV_BROAD_INFO {
	int cpno;
	int category;
	QString title;
};

namespace Ui {
	class AFQTvBroadItem;
}

class AFQTvBroadItem : public QFrame
{
	Q_OBJECT

public:
	AFQTvBroadItem(QWidget* parent = nullptr);
	~AFQTvBroadItem();

signals:
	void qsignalRequestOnetimeUrl(int idx);
	void qsignalStopTvBroad(int idx);	
	void qsignalHoverItem(QString);
	void qsignalLeaveItem();

private slots:
	void _qslotPlayButtonClicked();
	void _qslotStopButtonClicked();

protected:
	virtual void enterEvent(QEnterEvent* event) override;
	virtual void leaveEvent(QEvent* event) override;

public:
	void SetTvBroadInfo(int cpNo, QString cpTitle);
	int  GetTvBroadCpNo();
	void SetTvBroadPlayingStatus(bool playing);

private:
	Ui::AFQTvBroadItem* ui;
	int m_cpNo;

	bool m_playing = false;
};