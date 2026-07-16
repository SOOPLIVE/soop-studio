#pragma once

#include <QWidget>

#include <obs.hpp>

namespace Ui
{
class AFAudioMixerWidget;
}

class AFAudioMixerWidget : public QWidget
{
	Q_OBJECT

public:
	explicit AFAudioMixerWidget(QWidget* parent = nullptr);
	~AFAudioMixerWidget();

public:
	void ActivateAudioSource(OBSSource source);
	void SetMixerLayout(bool vertical);

protected:
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	void Init();
	void SetScrollBarVisibility(bool transparent);

private:
	Ui::AFAudioMixerWidget* ui = nullptr;
};

