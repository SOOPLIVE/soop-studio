#ifndef CSOURCEPROPERTIES_H
#define CSOURCEPROPERTIES_H

#include <QDialog>
#include <QScrollArea>
#include <QDialogButtonBox>

#include "obs.hpp"

#include "UIComponent/CTopBaseWindow.h"

class OBSPropertiesView;

namespace Ui {
    class AFQSourceProperties;
}

class AFQSourceProperties : public AFTTopBaseDialog
{
    Q_OBJECT

public:
    explicit AFQSourceProperties(QWidget *parent, OBSSource source);
    ~AFQSourceProperties();

private slots:
    void qslotButtonBoxClicked(QAbstractButton* button);
    void qslotCloseButtonClicked();
    void qslotAddPreviewButton();
    void qslotStartTransPreview();
    void qslotSetWindowTitle(QString title);

    void qslotAddDrawCallback();
    void qslotAddTransitionDrawCallback();
    void qslotUpdatePropsCallback();

private:
    static void _SourceRemoved(void* data, calldata_t* params);
    static void _SourceRenamed(void* data, calldata_t* params);
    static void _UpdateProperties(void* data, calldata_t*);
    static void _DrawPreview(void* data, uint32_t cx, uint32_t cy);
    static void _DrawTransitionPreview(void* data, uint32_t cx, uint32_t cy);

    void _Cleanup();

public:
    OBSSource GetOBSSource() { return m_obsSource; }
    enum obs_source_type GetSourceType() { return m_sourceType; }
    void CloseSourcePropertise();

protected:
    virtual bool nativeEvent(const QByteArray& eventType, void* message,
                             qintptr* result) override;

    virtual void closeEvent(QCloseEvent* event) override;
    virtual void reject() override;
    virtual void showEvent(QShowEvent* event) override;

private:
    Ui::AFQSourceProperties *ui = nullptr;

    bool m_acceptClicked = false;
    bool m_direction = true;

    // For OBS Data 
    OBSSource m_obsSource;
    OBSSignal m_signalRemoved;
    OBSSignal m_signalRenamed;
    OBSSignal m_signalUpdateProperties;
    OBSSignal m_signalMediaStop;
    OBSData   m_dataOldSetting;
    OBSSourceAutoRelease m_obsSourceA;
    OBSSourceAutoRelease m_obsSourceB;
    OBSSourceAutoRelease m_obsSourceClone;

    enum obs_source_type m_sourceType;

    // For Props UI
    OBSPropertiesView* m_pViewProps = nullptr;
};

#endif // CSOURCEPROPERTIES_H