#pragma once

#include <QLabel>

//Forward Class

class AFQRoundedTextLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(QBrush brush READ GetBrush WRITE SetBrush)

public:
    explicit AFQRoundedTextLabel(QWidget* parent = nullptr);
    ~AFQRoundedTextLabel() {};

    void SetContentMargins(int l, int t, int r, int b);
           
    QBrush GetBrush() const { return m_lineBrush; };
    void SetBrush(const QBrush& brush) { m_lineBrush = brush; };

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QBrush m_lineBrush = QBrush(QColor(255, 255, 255, 51));
};