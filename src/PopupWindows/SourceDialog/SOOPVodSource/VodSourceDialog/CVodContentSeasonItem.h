#pragma once

#include <vector>

#include <QWidget>

#include "CVodListItem.h"

namespace Ui {
	class AFQVodContentSeasonItem;
}

class AFQVodContentSeasonItem : public QFrame
{
	Q_OBJECT

public:
	AFQVodContentSeasonItem(QWidget* parent = nullptr);
	~AFQVodContentSeasonItem();

signals:
	void qsignalAddContentSeasonVodList(QString season);

private slots:
	void _qslotAddContentSeasonVodList();

public:
	void	SetContentSeasonInfo(QString title);
	QString GetSeason() { return m_seasonTitle; };

private:
	Ui::AFQVodContentSeasonItem* ui;

	QString m_seasonTitle;
};