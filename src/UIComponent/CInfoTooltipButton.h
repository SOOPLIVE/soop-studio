#pragma once
#include <QPushButton>
#include <QPointer>
#include <QLabel>
#include <QTimer>

#include "UIComponent/CCustomMenu.h"

#define ENUM_TOOLTIP_POSITION AFQInfoTooltipButton::ToolTipPos

class AFQInfoTooltipButton : public QPushButton
{
#pragma region QT Field, CTOR/DTOR
	Q_OBJECT

public:
	enum class ToolTipPos
	{
		TopLeft,
		TopCenter,
		TopRight,
		LeftTop,
		LeftCenter,
		LeftBottom,
		BottomLeft,
		BottomCenter,
		BottomRight,
		RightTop,
		RightCenter,
		RightBottom
	};

	explicit AFQInfoTooltipButton(QWidget* parent = nullptr);
	~AFQInfoTooltipButton();

signals:
	void qsignalShowExplanationTriggered();

private slots:
	void _qslotShowExplanation();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
	void SetExplanationText(const QString& explanation, ToolTipPos balloonTipDir = ToolTipPos::RightCenter);
#pragma endregion public func

#pragma region private func
private:
	void _CreateExplanationWidget();
#pragma endregion private func

#pragma region private member var
private:
	QPointer<AFQCustomMenu> m_explanationWidget = nullptr;
	ToolTipPos m_explanationWidgetPos = ToolTipPos::RightCenter;
	QString m_explanationText = "";

	QLabel* m_explanationLabel = nullptr;
	QWidget* m_explanation = nullptr;
#pragma endregion private member var
};

