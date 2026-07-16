#pragma once

#include <QWidget>

namespace Ui
{
class AFAdvanceControlsWidget;
}

class AFAdvanceControlsWidget final : public QWidget
{
	Q_OBJECT

public:
    explicit AFAdvanceControlsWidget(QWidget* parent = nullptr);
    ~AFAdvanceControlsWidget();

public:
    void EnableReplayBuffer(bool enable);

public slots:
    void ReplayBufferReleased();

private slots:
    void ReplayBufferButtonClicked();
    void SaveReplayBufferButtonClicked();
    void SaveReplayBufferButtonEnabled();

public:
    void SetReplayBufferStartStopStyle(bool bufferStart); // start, stop
    void SetReplayBufferStoppingStyle(); // stopping

private:
    void Initialize();

private:    
    Ui::AFAdvanceControlsWidget* ui = nullptr;
};
