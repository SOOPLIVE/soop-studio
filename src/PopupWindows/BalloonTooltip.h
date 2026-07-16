#pragma once

#include <QWidget>


namespace Ui {
	class BalloonTooltip;
};

class BalloonTooltip : public QWidget
{
	Q_OBJECT

public:
	explicit BalloonTooltip(QString title, QString msg, QWidget* parent = nullptr);
	~BalloonTooltip();

	void SetTooltipPointPosY(int posY);

signals:
	void closeTooltipEvent();

private slots:
	void closeTooltip();

private:
	Ui::BalloonTooltip* ui = nullptr;
};