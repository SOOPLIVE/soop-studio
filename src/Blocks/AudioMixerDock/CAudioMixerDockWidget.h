#pragma once

#include "Blocks/CBaseDockWidget.h"
#include <QButtonGroup>

namespace Ui
{
class AFAudioMixerDockWidget;
}

class AFAudioMixerDockWidget : public AFQBaseDockWidget
{
#pragma region QT Field, CTOR/DTOR
	Q_OBJECT
#pragma endregion QT Field, CTOR/DTOR

#pragma region class initializer, destructor
public:
	explicit AFAudioMixerDockWidget(QWidget* parent = nullptr);
	~AFAudioMixerDockWidget();
#pragma endregion class initializer, destructor


#pragma region public func
public:
	Ui::AFAudioMixerDockWidget* GetUIPtr() { return ui; }
#pragma endregion public func

#pragma region private member var
private:
	Ui::AFAudioMixerDockWidget* ui;
#pragma endregion private member var
};

