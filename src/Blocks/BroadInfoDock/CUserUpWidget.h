#pragma once
#include <QWidget>

namespace Ui {
	class AFUserUpWidget;
}

class AFUserUpWidget : public QWidget
{
	Q_OBJECT

public:
	explicit AFUserUpWidget(QWidget* parent = nullptr);
	~AFUserUpWidget();

public slots:
	void qslotRefreshUpInfo(int result, QString msg);

private slots:
	//void _qslotClickedHideUserCount();

signals:
	//void qsignalHideUserCountChanged(bool hideUserCount);

private:
	void _Init();

private:
	Ui::AFUserUpWidget* ui = nullptr;
};