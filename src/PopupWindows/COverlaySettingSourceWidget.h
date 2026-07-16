
#pragma once

#include "COverlaySourceWidget.h"

#include <QLabel>
#include "UIComponent/CMouseClickSlider.h"

class OverlaySettingSourceWidget : public OverlaySourceWidget {

public:
    explicit OverlaySettingSourceWidget(QWidget* parent = nullptr);
    virtual ~OverlaySettingSourceWidget();

public:
    int Opacity() { return opacitySlider->value(); }
    void Opacity(int opacity);

protected:
    void paintEvent(QPaintEvent* event) override;
    //void resizeEvent(QResizeEvent* event) override;

private:
    static constexpr int fix_w = 306;
    static constexpr int fix_h = 80;

    QLabel* opacityLabel = nullptr;
    AFQMouseClickSlider* opacitySlider = nullptr;
};

