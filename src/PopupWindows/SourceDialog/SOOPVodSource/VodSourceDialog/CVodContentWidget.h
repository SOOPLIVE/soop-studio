#pragma once

#include <QWidget>
#include <QFrame>
#include <QPushButton>
#include <QLayout>

#include "CoreModel/SOOPSource/CSoopMediaSourceManager.h"

namespace Ui {
	class AFQVodContentWidget;
}

class AFQVodContentWidget : public QFrame {
	Q_OBJECT

public:
	AFQVodContentWidget(QWidget* parent = nullptr, SOOP_VOD_TYPE type = SOOP_VOD_TYPE::NONE);
	~AFQVodContentWidget();

protected:
	void mousePressEvent(QMouseEvent* event) override;
	virtual void enterEvent(QEnterEvent* event) override;
	virtual void leaveEvent(QEvent* event) override;

signals:
	void qsignalContentItemClicked(int contentIdx);
	void qsignalHoverItem(QString);
	void qsignalLeaveItem();

public slots:
	void qslotImageDownloaded(QByteArray responseData);
	void qslotGoChannelButtonClicked();

public:
	void	SetVodContentInfo(VodContentInfo_s info);
	QString GetVodContentName();

private:
	Ui::AFQVodContentWidget* ui;

	int m_contentIdx = 0;
	QString m_contentName;
	QString m_vodChannelUrl;
};