
#pragma once

#include "COverlaySourceWidget.h"

#include <QLabel>

#include <browser-panel.hpp> // QCefWidget

class OverlayCefSourceWidget : public OverlaySourceWidget {

public:
    enum class Types {
        none,
        chat,
    };

    explicit OverlayCefSourceWidget(QWidget* parent, bool editable, Types type);
    virtual ~OverlayCefSourceWidget();

public:
    void Opacity(int opacity);

protected:
    void paintEvent(QPaintEvent* event) override; 
    //void resizeEvent(QResizeEvent* event) override;

private:
    void _URL(const char* url);

private:
    static constexpr int min_w = 230;
    static constexpr int min_h = 330;

    bool editable = false;
    Types type = Types::none;

    QLabel* labelIcon = nullptr;
    QLabel* labelText = nullptr;
    QCefWidget* widget = nullptr;
};

