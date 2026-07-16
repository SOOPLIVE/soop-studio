#ifndef CADDTAGDIALOG_H
#define CADDTAGDIALOG_H

#include "UIComponent/CTopBaseWindow.h"
#include "Blocks/CBlockManager.h"

namespace Ui {
    class AFQAddTagDialog;
}

class AFQAddTagDialog : public AFTTopBaseDialog
{
#pragma region QT Field, CTOR/DTOR

    Q_OBJECT
public:
    explicit AFQAddTagDialog(QWidget* parent = nullptr);
    ~AFQAddTagDialog();

signals:
    void qsignalTagChanged(const std::vector<std::string>& tags);

private slots:
    void _qslotAddTagDataReceived(const QCefQuery& query);
#pragma endregion QT Field, CTOR/DTOR

#pragma region protected func
protected:
    void closeEvent(QCloseEvent* event) override;
#pragma endregion protected func

#pragma region private func
private:
    void _Init();
#pragma endregion private func

#pragma region private var
private:
    Ui::AFQAddTagDialog* ui;
    QCefWidget* m_pCefWidget = nullptr;
#pragma endregion private var
};

#endif // CADDTAGDIALOG_H
