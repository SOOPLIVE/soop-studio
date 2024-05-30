#ifndef CLOGINTOGGLEFRAME_H
#define CLOGINTOGGLEFRAME_H

#include <QFrame>

namespace Ui {
class AFQLoginToggleFrame;
}

class AFQLoginToggleFrame : public QFrame
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

public:
    explicit AFQLoginToggleFrame(QWidget *parent = nullptr);
    ~AFQLoginToggleFrame();

    void VisibleToggleButton(bool show);
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void LoginToggleFrameInit(QString name = "", QString path = "");
#pragma endregion public func

#pragma region private member var
private:
    Ui::AFQLoginToggleFrame *ui;
#pragma region private member var
};

#endif // CLOGINTOGGLEFRAME_H
