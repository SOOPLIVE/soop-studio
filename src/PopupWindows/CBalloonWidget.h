#ifndef CBALLOONWIDGET_H
#define CBALLOONWIDGET_H

#include <QWidget>
#include <QLabel>

namespace Ui {
class AFQBalloonWidget;
}

class AFQBalloonWidget : public QWidget
{
	Q_OBJECT;
public:
	explicit AFQBalloonWidget(QWidget* parent = nullptr);
	~AFQBalloonWidget();

#pragma region QT Field
public slots:
	
signals:
	void qsignalFromChild(bool);
#pragma endregion QT Field

#pragma region public func
public:
	void BalloonWidgetInit(QString tooltip = "");
	void BalloonWidgetInit(QMargins margin, int spacing);

	void ChangeTooltipText(QString text);
	void ShowBalloon(QPoint pos);

	bool AddWidgetToBalloon(QWidget* widget);
	bool AddTextToBalloon(QString text);
#pragma endregion public func

#pragma region protected func
protected:
	void paintEvent(QPaintEvent* e) override;
#pragma endregion protected func

#pragma region private func
private:
	void _AdjustBalloonGeometry();
#pragma endregion private func

#pragma region private member var
private:
	Ui::AFQBalloonWidget* ui;
	bool m_bIsTooltip = false;
	QLabel* m_Tooltiplabel = nullptr;

	QBrush m_LineBrush = QBrush(QColor(255, 255, 255, 26));
#pragma endregion private member var
};

#endif // CBALLOONWIDGET_H
