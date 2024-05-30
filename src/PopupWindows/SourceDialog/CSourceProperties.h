#ifndef CSOURCEPROPERTIES_H
#define CSOURCEPROPERTIES_H

#include <QDialog>
#include <QScrollArea>
#include <QDialogButtonBox>

#include "obs.hpp"

#include "UIComponent/CRoundedDialogBase.h"

class AFQPropertiesView;

namespace Ui {
class AFQSourceProperties;
}

//class AFQSourceProperties : public QDialog
class AFQSourceProperties : public AFQRoundedDialogBase
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

public:
    explicit AFQSourceProperties(QWidget *parent, OBSSource source_);
    ~AFQSourceProperties();

private slots:
    void qSlotButtonBoxClicked(QAbstractButton* button);
    void qSlotCloseButtonClicked();

    void qSlotAddPreviewButton();
    void qSlotStartTransPreview();

    void qslotSetWindowTitle(QString title);

#pragma endregion QT Field

#pragma region public func
public:
    OBSSource GetOBSSource() { return source; }
#pragma endregion public func

#pragma region protected func
protected:
    virtual bool nativeEvent(const QByteArray& eventType, void* message,
                             qintptr* result) override;

    virtual void closeEvent(QCloseEvent* event) override;
    virtual void reject() override;

#pragma endregion protected func


#pragma region private member var

private:
    Ui::AFQSourceProperties *ui = nullptr;
    bool m_acceptClicked = false;
    bool m_direction = true;

    OBSSource source;
    OBSSignal m_signalRemoved;
    OBSSignal m_signalRenamed;
    OBSSignal m_signalUpdateProperties;
    OBSData oldSettings;
    AFQPropertiesView* view = nullptr;
    QScrollArea* viewTest = nullptr;

    OBSSourceAutoRelease sourceA;
    OBSSourceAutoRelease sourceB;
    OBSSourceAutoRelease sourceClone;

    static void SourceRemoved(void* data, calldata_t* params);
    static void SourceRenamed(void* data, calldata_t* params);
    static void UpdateProperties(void* data, calldata_t*);
    static void DrawPreview(void* data, uint32_t cx, uint32_t cy);
    static void DrawTransitionPreview(void* data, uint32_t cx, uint32_t cy);


    void        Cleanup();
    // UI
private:
    QPushButton* m_buttonOK = nullptr;
    QPushButton* m_buttonCancel = nullptr;
    QPushButton* m_buttonRestore = nullptr;

#pragma endregion private member var

};

#endif // CSOURCEPROPERTIES_H
