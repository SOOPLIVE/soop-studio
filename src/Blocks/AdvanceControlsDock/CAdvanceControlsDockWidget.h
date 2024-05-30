#pragma once


#include "Blocks/CBaseDockWidget.h"
#include <QPushButton>

namespace Ui
{
class AFAdvanceControlsDockWidget;
}


class AFAdvanceControlsDockWidget final : public AFQBaseDockWidget
{
#pragma region QT Field
	Q_OBJECT
    
    enum class WidgetState {
        Default,
        Pressed
    };

#pragma region class initializer, destructor
public:
    explicit AFAdvanceControlsDockWidget(QWidget* parent = nullptr);
    ~AFAdvanceControlsDockWidget();
#pragma endregion class initializer, destructor

#pragma region class
public:
    void EnableReplayBuffer(bool enable);
#pragma endregion class
        
public slots:
    void                qslotReleasedReplayBuffer();

private slots:
    void                qslotClickedStudioMode();
    void                qslotClickedReplayBuffer();
    void                qslotReleasedStudioMode();
    void                qslotClickedDownloadReplayBuffer();
#pragma endregion QT Field

#pragma region public function
public:
    void                SetStudioMode(bool isStudioMode);
    void                SetReplayBufferStartStopStyle(bool bufferStart); // start, stop
    void                SetReplayBufferStoppingStyle(); // stopping
#pragma region public member

#pragma region private function
private:
    void                _Initialize();    
    void                _SetStudioModeText();    

    // Style
    void                _ChangeStudioWidgetStyle(WidgetState state); // pressed, normal
    void                _ChangeStudioIconStyle();
    void                _ChangeReplayBufferStyle(WidgetState state); // pressed, normal
#pragma region private member
private:
    bool                                        m_IsStudioMode = false; 
    QPushButton*                                m_qDownloadReplayBuffer;
    
    Ui::AFAdvanceControlsDockWidget*            ui;
#pragma endregion private member
};
