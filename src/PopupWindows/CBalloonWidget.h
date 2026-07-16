#ifndef CBALLOONWIDGET_H
#define CBALLOONWIDGET_H

#include <QWidget>
#include <QLabel>

namespace Ui {
class AFQBalloonWidget;
}

class AFQBalloonWidget : public QWidget
{
	Q_OBJECT

public:
	explicit AFQBalloonWidget(QWidget* parent = nullptr);
	~AFQBalloonWidget();

signals:
	void qsignalFromChild(bool);

public:
	void BalloonWidgetInit(QString tooltip = "");
	void BalloonWidgetInit(QMargins margin, int spacing);

	void ChangeTooltipText(QString text);
	void ShowBalloon(QPoint pos);

	bool AddWidgetToBalloon(QWidget* widget);
	bool AddTextToBalloon(QString text);

protected:
	void paintEvent(QPaintEvent* e) override;

private:
	void _AdjustBalloonGeometry();

private:
	Ui::AFQBalloonWidget* ui;
	bool m_isTooltip = false;
	QLabel* m_pTooltiplabel = nullptr;
};

#endif // CBALLOONWIDGET_H

// Not Used
//QBrush m_lineBrush = QBrush(QColor(255, 255, 255, 26));
//