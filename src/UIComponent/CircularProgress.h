#ifndef CIRCULARPROGRESS_H
#define CIRCULARPROGRESS_H

#include <QObject>
#include <QWidget>
#include <QTimer>

class CircularProgress : public QWidget
{
    Q_OBJECT
public:
    explicit CircularProgress(QWidget* parent = nullptr);
    void setMaxValue(int v);

public slots:
    void setTimeValue(int v);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int value;
    int maxValue;
};

#endif // CIRCULARPROGRESS_H
