#ifndef CSLIDERFRAME_H
#define CSLIDERFRAME_H

#include "UIComponent/CMouseClickSlider.h"
#include <QFrame>
#include <QTimer>
#include <QPointer>

#define MIN_PEAK -60;

class AFQSysVolumeSlider final : public AFQMouseClickSlider
{
	Q_OBJECT

public:
	AFQSysVolumeSlider(QWidget* parent = 0);
	~AFQSysVolumeSlider() = default;

private slots:
	void qslotUpdate();

public:
	void SetCurrentPeak(float curPeak);
	void SetMuted(bool muted);
	bool IsMuted() { return m_muted; }

protected:
	void paintEvent(QPaintEvent* event);
	void showEvent(QShowEvent* event);
	void hideEvent(QHideEvent* event);

private:
	QPointer<QTimer> m_timer;
	const float m_minPeak = MIN_PEAK;
	float m_maxPeak = 0.f;
	float m_currentPeak = MIN_PEAK;
	bool m_muted = false;
};

namespace Ui {
	class AFQSliderFrame;
}

class AFQSliderFrame : public QFrame
{
	Q_OBJECT

public:
	explicit AFQSliderFrame(QWidget* parent = nullptr);
	~AFQSliderFrame();

signals:
	void qsignalMouseEnterSlider();
	void qsignalMouseLeave();
	void qsignalVolumeChanged(float volume);
	void qsignalMuteButtonClicked();

private slots:
	void qslotSliderValueChanged(int sliderValue);

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

protected:
	bool event(QEvent* e) override;
	bool eventFilter(QObject* obj, QEvent* event);

private:
	void _SetSliderStateProperty(QString state);
	void _ApplyShadowEffect();

private:
    Ui::AFQSliderFrame *ui;
};

#endif // CSLIDERFRAME_H