#ifndef CADDCATEGORYDIALOG_H
#define CADDCATEGORYDIALOG_H

#include "UIComponent/CTopBaseWindow.h"
#include "Blocks/CBlockManager.h"

#include "UIComponent/CMessageBox.h"
#include "Utils/SOOPAPIHandler.h"

namespace Ui {
    class AFQCategoryDialog;
}

class AFQCategoryDialog : public AFTTopBaseDialog
{
#pragma region QT Field, CTOR/DTOR

    Q_OBJECT
public:
    explicit AFQCategoryDialog(QWidget* parent, const std::string& categoryNum);
    ~AFQCategoryDialog();

signals:
    void qsignalCategoryChanged(const std::string& categoryNum, const std::string& categoryName);
    void qsignalGLCategoryChanged(int categoryNum, const QString& categoryName);

private slots:
    void _qslotCategoryDataReceived(const QCefQuery& query);
    void _qslotSoopGeoBlockCheckAPIResponse(const QByteArray& responseData);
#pragma endregion QT Field, CTOR/DTOR

#pragma region protected func
protected:
    virtual void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
#pragma endregion protected func

#pragma region private func
private:
    void _Init(const std::string& categoryNum);
#pragma endregion private func

#pragma region private var
private:
    Ui::AFQCategoryDialog* ui;
    QCefWidget* m_pCefWidget = nullptr;

    QString     m_pendingCategoryName;
    std::string m_pendingCategoryNum;
#pragma endregion private var
};

#endif // CADDCATEGORYDIALOG_H
