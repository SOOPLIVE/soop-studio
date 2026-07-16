
#pragma once

#include "COverlaySourceWidget.h"

#include <QLabel>

class OverlayLabelSourceWidget : public OverlaySourceWidget {

public:
    enum class Types {
        none, 
        time, 
        gift, 
        user, 
        up,
    };

    explicit OverlayLabelSourceWidget(QWidget* parent, bool editable, Types type);
    virtual ~OverlayLabelSourceWidget();

public:
    void Opacity(int opacity);

protected:
    void paintEvent(QPaintEvent* event) override;

private:    
    void _Timer();

private:
    static constexpr int fix_w = 120;
    static constexpr int fix_h = 50;

    bool editable = false;
    Types type = Types::none;

    QLabel* labelIcon = nullptr;
    QLabel* labelText = nullptr;

    int count = 0; // timer
#pragma endregion private var
};

