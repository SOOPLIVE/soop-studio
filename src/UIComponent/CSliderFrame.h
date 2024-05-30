#ifndef CSLIDERFRAME_H
#define CSLIDERFRAME_H

#include "UIComponent/CMouseClickSlider.h"
#include <QFrame>
#include <QTimer>

class AFQSysVolumeSlider final : public AFQMouseClickSlider
{
#pragma region QT Field, CTOR/DTOR
	Q_OBJECT

public:
	AFQSysVolumeSlider(QWidget* parent = 0);
	~AFQSysVolumeSlider() = default;

private slots:
	void qslotUpdate();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
	void SetCurrentPeak(float curPeak);
	void SetMuted(bool muted);
	bool IsMuted() { return m_bMuted; }
#pragma endregion public func

#pragma region protected func
protected:
	void paintEvent(QPaintEvent* event);
	void showEvent(QShowEvent* event);
	void hideEvent(QHideEvent* event);
#pragma endregion protected func

#pragma region private func, var
private:
	QPointer<QTimer> m_qTimer;
	const float m_fMinPeak = -60.0;
	float m_fMaxPeak = 0.f;
	float m_fCurrentPeak = -60.0;
	bool m_bMuted = false;
#pragma endregion private func, var
};

namespace Ui {
class AFQSliderFrame;
}

class AFQSliderFrame : public QFrame
{
#pragma region QT Field
	Q_OBJECT

#pragma region class initializer, destructor
public:
	explicit AFQSliderFrame(QWidget* parent = nullptr);
	~AFQSliderFrame();
#pragma endregion class initializer, destructor

signals:
	void qsignalMouseEnterSlider();
	void qsignalMouseLeave();
	void qsignalVolumeChanged(int volume);
	void qsignalMuteButtonClicked();
#pragma endregion QT Field

#pragma region public func
public:
	void InitSliderFrame(const char* imagepath, bool buttonchecked = true, int sliderTotal = 100, int volume = 0);
	int VolumeSize();
	void SetButtonProperty(const char* property);
	void SetVolumeSize(int volume);
	void SetVolumePeak(float peak);
	void SetVolumeMuted(bool muted);
	void SetVolumeSliderEnabled(bool enabled);
	bool ButtonIsChecked();
	void BlockSliderSignal(bool block);
	bool IsVolumeMuted();
#pragma endregion public func

#pragma region protected func
protected:
	bool event(QEvent* e) override;
#pragma endregion protected func

#pragma region private func
private:
	void _ApplyShadowEffect();
#pragma endregion private func

#pragma region private var
private:
    Ui::AFQSliderFrame *ui;
#pragma endregion private var
};

#endif // CSLIDERFRAME_H
