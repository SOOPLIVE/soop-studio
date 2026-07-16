#include "CMainFrame.h"
#include "ui_aneta-main-frame.h"

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Source/CSource.h"

#include "UIComponent/CNameDialog.h"
#include "UIComponent/CMessageBox.h"

#include "Blocks/AudioMixerDock/CAudioMixerDockWidget.h"
#include "Blocks/AudioMixerDock/CAudioAdvSettingWidget.h"

#include "AudioSource/CAudioSource.h"

void AFMainFrame::qslotActivateAudioSource(OBSSource source)
{
    if (AFSourceUtil::SourceMixerHidden(source))
        return;
    if (!obs_source_active(source))
        return;
    if (!obs_source_audio_active(source))
        return;

    QWidget* outBlock = nullptr;
    if (!MAIN_BLOCKMANAGER->FindBlock(ENUM_WINDOW_TYPE::AudioMixer, outBlock))
        return;

    AFAudioMixerWidget* audioMixer = reinterpret_cast<AFAudioMixerWidget*>(outBlock);

    if (audioMixer)
        audioMixer->ActivateAudioSource(source);
}

void AFMainFrame::qslotDeactivateAudioSource(OBSSource source)
{
    VolControlVector& volumes = SCENE_CONTEXT.GetVolControlVector();
    for (size_t i = 0; i < volumes.size(); i++) {
        if (volumes[i]->GetSource() == source)
        {
            delete volumes[i];
            volumes.erase(volumes.begin() + i);
            break;
        }
    }
}

void AFMainFrame::qslotRenameSources(OBSSource source, QString newName, QString prevName)
{
    VolControlVector& volumes = SCENE_CONTEXT.GetVolControlVector();
    for (size_t i = 0; i < volumes.size(); i++) {
        if (volumes[i]->GetName().compare(prevName) == 0)
            volumes[i]->SetName(newName);
    }
}

void AFMainFrame::qslotStackedMixerAreaContextMenuRequested()
{
    QAction unhideAllAction(Str("UnhideAll"), this);
    QAction advPropAction(Str("Basic.MainMenu.Edit.AdvAudio"), this);
    QAction toggleControlLayoutAction(Str("AudioMixer.VerticalLayout"), this);

    bool vertical = config_get_bool(USERCONFIG, "BasicWindow", "VerticalVolControl");
    if (vertical)
        toggleControlLayoutAction.setText(Str("AudioMixer.HorizontalLayout"));

    /* ------------------- */

    connect(&unhideAllAction, &QAction::triggered, this, &AFMainFrame::qslotUnhideAllAudioControls, Qt::DirectConnection);
    connect(&advPropAction, &QAction::triggered, this, &AFMainFrame::qslotAdvAudioPropertiesTriggered, Qt::DirectConnection);
    connect(&toggleControlLayoutAction, &QAction::triggered, this, &AFMainFrame::qslotToggleVolControlLayout, Qt::DirectConnection);

    /* ------------------- */

    AFQCustomMenu popup = new AFQCustomMenu(this);
    popup.addAction(&unhideAllAction);
    popup.addSeparator();
    popup.addAction(&toggleControlLayoutAction);
    popup.addSeparator();
    popup.addAction(&advPropAction);
    popup.exec(QCursor::pos());
}

void AFMainFrame::qslotVolControlContextMenu()
{
    AFQVolControl* vol = reinterpret_cast<AFQVolControl*>(sender());

    QAction lockAction(Str("LockVolume"), this);
    lockAction.setCheckable(true);
    lockAction.setChecked(AFSourceUtil::SourceVolumeLocked(vol->GetSource()));

    QAction hideAction(Str("Hide"), this);
    QAction unhideAllAction(Str("UnhideAll"), this);
    QAction mixerRenameAction(Str("Rename"), this);

    QAction copyFiltersAction(Str("Copy.Filters"), this);
    QAction pasteFiltersAction(Str("Paste.Filters"), this);
    QAction toggleControlLayoutAction(Str("AudioMixer.VerticalLayout"), this);

    bool vertical = config_get_bool(USERCONFIG, "BasicWindow", "VerticalVolControl");
    if (vertical)
        toggleControlLayoutAction.setText(Str("AudioMixer.HorizontalLayout"));

    QAction filtersAction(Str("Filters"), this);
    QAction propertiesAction(Str("Properties"), this);
    QAction advPropAction(Str("Basic.MainMenu.Edit.AdvAudio"), this);

    connect(&hideAction, &QAction::triggered, this, &AFMainFrame::qslotHideAudioControl, Qt::DirectConnection);
    connect(&unhideAllAction, &QAction::triggered, this, &AFMainFrame::qslotUnhideAllAudioControls, Qt::DirectConnection);
    connect(&lockAction, &QAction::toggled, this, &AFMainFrame::qslotLockVolumeControl, Qt::DirectConnection);
    connect(&mixerRenameAction, &QAction::triggered, this, &AFMainFrame::qslotMixerRenameSource, Qt::DirectConnection);

    connect(&copyFiltersAction, &QAction::triggered, this, &AFMainFrame::qslotAudioMixerCopyFilters, Qt::DirectConnection);
    connect(&pasteFiltersAction, &QAction::triggered, this, &AFMainFrame::qslotAudioMixerPasteFilters, Qt::DirectConnection);

    connect(&toggleControlLayoutAction, &QAction::triggered, this, &AFMainFrame::qslotToggleVolControlLayout, Qt::DirectConnection);
    connect(&filtersAction, &QAction::triggered, this, &AFMainFrame::qslotGetAudioSourceFilters, Qt::DirectConnection);
    connect(&propertiesAction, &QAction::triggered, this, &AFMainFrame::qslotGetAudioSourceProperties, Qt::DirectConnection);
    connect(&advPropAction, &QAction::triggered, this, &AFMainFrame::qslotAdvAudioPropertiesTriggered, Qt::DirectConnection);

    hideAction.setProperty("volControl", QVariant::fromValue<AFQVolControl*>(vol));
    lockAction.setProperty("volControl", QVariant::fromValue<AFQVolControl*>(vol));
    mixerRenameAction.setProperty("volControl", QVariant::fromValue<AFQVolControl*>(vol));

    copyFiltersAction.setProperty("volControl", QVariant::fromValue<AFQVolControl*>(vol));
    pasteFiltersAction.setProperty("volControl", QVariant::fromValue<AFQVolControl*>(vol));

    filtersAction.setProperty("volControl", QVariant::fromValue<AFQVolControl*>(vol));
    propertiesAction.setProperty("volControl", QVariant::fromValue<AFQVolControl*>(vol));

    copyFiltersAction.setEnabled(obs_source_filter_count(vol->GetSource()) > 0);
    OBSSourceAutoRelease source = SCENE_CONTEXT.GetMixerCopyFilter();
    if (source)
        pasteFiltersAction.setEnabled(true);
    else
        pasteFiltersAction.setEnabled(false);

    AFQCustomMenu popup = new AFQCustomMenu(this);
    vol->SetContextMenu(&popup);
    popup.addAction(&lockAction);
    popup.addSeparator();
    popup.addAction(&unhideAllAction);
    popup.addAction(&hideAction);
    popup.addAction(&mixerRenameAction);
    popup.addSeparator();
    popup.addAction(&copyFiltersAction);
    popup.addAction(&pasteFiltersAction);
    popup.addSeparator();
    popup.addAction(&toggleControlLayoutAction);
    popup.addSeparator();
    popup.addAction(&filtersAction);
    popup.addAction(&propertiesAction);
    popup.addAction(&advPropAction);

    // toggleControlLayoutAction deletes and re-creates the volume controls
    // meaning that "vol" would be pointing to freed memory.
    if (popup.exec(QCursor::pos()) != &toggleControlLayoutAction)
        vol->SetContextMenu(nullptr);
}

void AFMainFrame::qslotHideAudioControl()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    AFQVolControl* vol = action->property("volControl").value<AFQVolControl*>();
    obs_source_t* source = vol->GetSource();

    if (!AFSourceUtil::SourceMixerHidden(source)) {
        AFSourceUtil::SetSourceMixerHidden(source, true);

        /* Due to a bug with QT 6.2.4, the version that's in the Ubuntu
        * 22.04 ppa, hiding the audio mixer causes a crash, so defer to
        * the next event loop to hide it. Doesn't seem to be a problem
        * with newer versions of QT. */
        QMetaObject::invokeMethod(this, "qslotDeactivateAudioSource",
            Qt::QueuedConnection,
            Q_ARG(OBSSource, OBSSource(source)));
    }
}

void AFMainFrame::qslotUnhideAllAudioControls()
{
    auto UnhideAudioMixer = [this](obs_source_t* source) /* -- */
    {
        if (!obs_source_active(source))
            return true;
        if (!AFSourceUtil::SourceMixerHidden(source))
            return true;

        AFSourceUtil::SetSourceMixerHidden(source, false);
        qslotActivateAudioSource(source);
        return true;
    };

    using UnhideAudioMixer_t = decltype(UnhideAudioMixer);

    auto PreEnum = [](void* data, obs_source_t* source) -> bool /* -- */
    {
        return (*reinterpret_cast<UnhideAudioMixer_t*>(data))(source);
    };

    obs_enum_sources(PreEnum, &UnhideAudioMixer);
}

void AFMainFrame::qslotLockVolumeControl(bool lock)
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    AFQVolControl* vol = action->property("volControl").value<AFQVolControl*>();
    obs_source_t* source = vol->GetSource();

    OBSDataAutoRelease priv_settings =
        obs_source_get_private_settings(source);
    obs_data_set_bool(priv_settings, "volume_locked", lock);

    vol->EnableSlider(!lock);
}

void AFMainFrame::qslotMixerRenameSource()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    AFQVolControl* vol = action->property("volControl").value<AFQVolControl*>();
    OBSSource source = vol->GetSource();

    const char* prevName = obs_source_get_name(source);

    for (;;) {
        std::string name;
        bool accepted = AFQNameDialog::AskForName(
                                        this, 
                                        Str("Basic.Main.MixerRename.Title"),
                                        Str("Basic.Main.MixerRename.Text"), 
                                        name,
                                        QT_UTF8(prevName));
        if (!accepted)
            return;

        if (name.empty()) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
                                       this,
                                       "",
                                       Str("NoNameEntered.Text"));
            continue;
        }

        OBSSourceAutoRelease sourceTest =
            obs_get_source_by_name(name.c_str());

        if (sourceTest) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
                                       this,
                                       "",
                                       Str("NameExists.Text"));
            continue;
        }

        obs_source_set_name(source, name.c_str());
        vol->SetName(QString::fromStdString(name));

        break;
    }
}

void AFMainFrame::qslotAudioMixerCopyFilters()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    AFQVolControl* vol = action->property("volControl").value<AFQVolControl*>();
    obs_source_t* source = vol->GetSource();

    SCENE_CONTEXT.SetMixerCopyFilter(source);
}

void AFMainFrame::qslotAudioMixerPasteFilters()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    AFQVolControl* vol = action->property("volControl").value<AFQVolControl*>();
    obs_source_t* dstSource = vol->GetSource();

    OBSSourceAutoRelease source = SCENE_CONTEXT.GetMixerCopyFilter();

    if (source == dstSource)
        return;

    obs_source_copy_filters(dstSource, source);
}

void AFMainFrame::qslotToggleVolControlLayout()
{
    bool vertical = !config_get_bool(USERCONFIG, "BasicWindow", "VerticalVolControl");
    config_set_bool(USERCONFIG, "BasicWindow", "VerticalVolControl", vertical);

    ToggleMixerLayout(vertical);

    VolControlVector& volumes = SCENE_CONTEXT.GetVolControlVector();
    std::vector<OBSSource> sources;
    for (size_t i = 0; i != volumes.size(); i++)
        sources.emplace_back(volumes[i]->GetSource());

    m_pMainAudioSource->ClearVolumeControls();

#pragma region _SOOP_BREAKTIME
    for(auto it = sources.rbegin(); it != sources.rend(); ++it)
        qslotActivateAudioSource(*it);
#pragma endregion
}

void AFMainFrame::qslotGetAudioSourceFilters()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    AFQVolControl* vol = action->property("volControl").value<AFQVolControl*>();
    obs_source_t* source = vol->GetSource();

    CreateFiltersWindow(source);
}

void AFMainFrame::qslotGetAudioSourceProperties()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    AFQVolControl* vol = action->property("volControl").value<AFQVolControl*>();
    obs_source_t* source = vol->GetSource();

    CreateSourceProperties(source);
}

void AFMainFrame::qslotAdvAudioPropertiesTriggered()
{
    if (m_advAudioSettingPopup != nullptr) {
        m_advAudioSettingPopup->raise();
        if (IsSmallResolution())
            m_advAudioSettingPopup->setMinimumWidth(800);
        else
            m_advAudioSettingPopup->setMinimumWidth(1240);

        return;
    }

    m_advAudioSettingPopup = new AFQAudioAdvSettingDialog(this);
    m_blockManager->ApplyMoveInAllArea(m_advAudioSettingPopup);
    m_advAudioSettingPopup->show();

    if (IsSmallResolution())
    {
        m_advAudioSettingPopup->setMinimumWidth(800);
        m_advAudioSettingPopup->resize(800, m_advAudioSettingPopup->height());
    }
    else
        m_advAudioSettingPopup->setMinimumWidth(1240);
}


void AFMainFrame::ToggleMixerLayout(bool vertical) 
{
    QWidget* outBlock = nullptr;
    if (!MAIN_BLOCKMANAGER->FindBlock(ENUM_WINDOW_TYPE::AudioMixer, outBlock))
        return;

    AFAudioMixerWidget* audioMixer = reinterpret_cast<AFAudioMixerWidget*>(outBlock);
    if (!audioMixer)
        return;

    audioMixer->SetMixerLayout(vertical);
}
