
#pragma once

#include "COverlaySettingSourceWidget.h"
#include "COverlayCefSourceWidget.h"
#include "COverlayLabelSourceWidget.h"

class OverlaySceneWidget : public QWidget {
#pragma region QT Field
    Q_OBJECT
#pragma endregion QT Field
#pragma region class initializer, destructor
public:
    explicit OverlaySceneWidget(QWidget* parent, bool editable);
    virtual ~OverlaySceneWidget() {}
#pragma endregion class initializer, destructor
#pragma region public func
public:
    void Chat(bool editable);
    
    void Time(bool editable);
    void Gift(bool editable);
    void User(bool editable);
    void Up(bool editable);

    void Opacity(int value);

    static constexpr int min_w = 800;
    static constexpr int min_h = 450;
#pragma endregion public func
#pragma region protected func
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
#pragma endregion protected func
#pragma region private func
private:
    template<typename T>
    void _Widget(T*& widget, bool editable, typename T::Types type);
#pragma endregion private func
#pragma region private var
private:
    OverlaySettingSourceWidget* setting = nullptr;

    OverlayCefSourceWidget* chat = nullptr;
    
    OverlayLabelSourceWidget* time = nullptr;
    OverlayLabelSourceWidget* gift = nullptr;
    OverlayLabelSourceWidget* user = nullptr;
    OverlayLabelSourceWidget* up = nullptr;
#pragma endregion private var
};
