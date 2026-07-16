#pragma once
#include <QWidget>

namespace Ui {
	class AFUserCountWidget;
}

class AFUserCountWidget : public QWidget
{
	Q_OBJECT

public:
	explicit AFUserCountWidget(QWidget* parent = nullptr);
	~AFUserCountWidget();

public slots:
	void qslotRefreshViewer(int result, QString msg);

private slots:
	void _qslotClickedHideUserCount();
	void _qslotClickedStatistics();

signals:
	void qsignalHideUserCountChanged(bool hideUserCount);

private:
	void _Init();

private:
	Ui::AFUserCountWidget* ui = nullptr;

	bool m_showUserCountChanged = false;
};