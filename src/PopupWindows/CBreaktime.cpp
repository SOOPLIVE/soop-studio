
#include "CBreaktime.h"
#include "ui_break-time.h"

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"
#include "Application/CApplication.h"

#include "Utils/BreaktimeManager.h"
#include "CoreModel/OBSOutput/COutput.h" 
#include "CoreModel/Locale/CLocaleTextManager.h"

static inline bool IsDefaultSceneName(const QString& name) {
    return name == QTStr("breaktime.scene.default");
}

static inline QString fmtMMSS(int sec) {
    sec = qMax(0, sec);
    const int m = sec / 60;
    const int s = sec % 60;
    return QString("%1:%2")
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'));
}

AFBreaktime::AFBreaktime(QWidget *parent, int dlgPage) :
    AFTTopBaseWindow(parent),
    ui(new Ui::AFBreaktime)
{
    ui->setupUi(this);

    if (ui->stackedWidget_breaktime_dlg) {
        int idx = qBound(0, dlgPage, ui->stackedWidget_breaktime_dlg->count() - 1);
        ui->stackedWidget_breaktime_dlg->setCurrentIndex(idx);
    }
    MAIN_BLOCKMANAGER->ApplyMoveInAllArea(ui->titleFrame);
    
    connect(ui->pushButton_breaktime_close, &QPushButton::clicked, this, &AFBreaktime::qslotCloseButtonTriggered);
    ui->pushButton_breaktime_close->setProperty("buttonType", "closeButton");

    //polish
    PolishStyleSheet(ui->pushButton_QuestionMark_breaktime);
    m_defaultMessage = QTStr("breaktime.message.hint");
    m_editableMessage = QTStr("breaktime.message.editable");
    m_disabledMessage = QTStr("breaktime.message.disable");

    ui->pushButton_QuestionMark_2->SetExplanationText(QTStr("breaktime.title.tooltip"), ENUM_TOOLTIP_POSITION::BottomLeft);
    std::string currentLocale = LOCALE_CONTEXT.GetCurrentLocaleStr();
    if (dlgPage == 0)
    { 
        connect(ui->pushButton_breaktime_cancel, &QPushButton::clicked, this, &AFBreaktime::qslotCancelButtonTriggered);

        connect(ui->pushButton_breaktime_apply, &QPushButton::clicked, this, &AFBreaktime::qslotApplyButtonTriggered);
        ui->pushButton_breaktime_apply->setProperty("pushButtonTheme", "type2");

        PolishStyleSheet(ui->pushButton_breaktime_apply);

        for (int i = 1; i <= 10; ++i) {
            ui->comboBox_breaktime_time->addItem(QString::number(i) + QTStr("breaktime.time.option.min"), i);
        }
        ui->comboBox_breaktime_time->setCurrentIndex(2); // 3 Min 

        connect(ui->listWidget_breaktime, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current, QListWidgetItem* previous) {
                qslotSceneItemChanged(current, previous);
            });

        ui->listWidget_breaktime->setFocusPolicy(Qt::NoFocus);
        ui->listWidget_breaktime->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->listWidget_breaktime->viewport()->installEventFilter(this);
        ui->listWidget_breaktime->installEventFilter(this);
        ui->listWidget_breaktime->setGridSize(QSize(120, 40));

        QStringList sceneList;
        SceneItemVector& sceneItems = SCENE_CONTEXT.GetSceneItemVector();

        QHash<QString, QString> restrictedTmp;

        size_t sceneCount = sceneItems.size();
        for (size_t i = 0; i < sceneCount; i++) {
            OBSScene scene = sceneItems.at(i)->GetScene();
            const char* name = sceneItems.at(i)->GetSceneName();
            if (name && *name) {
                const QString qn = QString::fromUtf8(name);
                sceneList << qn;

                QString hitId;
                if (_isSceneRestricted(scene, &hitId)) {
                    restrictedTmp.insert(qn, hitId);
                }
            }
        }

        setBreaktimeScenes(sceneList);

        for (int row = 1; row < ui->listWidget_breaktime->count(); ++row) {
            if (auto* item = ui->listWidget_breaktime->item(row)) {
                const QString name = item->text();
                if (restrictedTmp.contains(name)) {
                    _markSceneItemRestricted(item, restrictedTmp.value(name));
                }
            }
        }
        m_restrictedScenes = restrictedTmp;

        connect(ui->comboBox_breaktime_time, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AFBreaktime::qslotBreaktimeChanged);
        ui->pushButton_QuestionMark_breaktime->SetExplanationText(QTStr("breaktime.bgm.title.tooltip"), ENUM_TOOLTIP_POSITION::BottomLeft);
        
        ui->comboBox_breaktime_bgm->addItem(QTStr("breaktime.bgm.combo1"), 0); // 0 = Not used
        ui->comboBox_breaktime_bgm->addItem(QTStr("breaktime.bgm.combo2"), 1); // 1 = Upbeat tempo
        ui->comboBox_breaktime_bgm->addItem(QTStr("breaktime.bgm.combo3"), 2); // 2 = Medium tempo
        ui->comboBox_breaktime_bgm->addItem(QTStr("breaktime.bgm.combo4"), 3); // 3 = Synthesizer
        ui->comboBox_breaktime_bgm->addItem(QTStr("breaktime.bgm.combo5"), 4); // 4 = Calm tempo
        ui->comboBox_breaktime_bgm->setCurrentIndex(0);

        connect(ui->comboBox_breaktime_bgm, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AFBreaktime::qslotBgmChanged);

        ui->pushButton_sound->setCheckable(true);
        connect(ui->pushButton_sound, &QPushButton::toggled, this, &AFBreaktime::qslotSoundButtonToggled);

        ui->horizontalSlider_breaktime->setRange(0, FADER_PRECISION);
        ui->horizontalSlider_breaktime->setValue(FADER_PRECISION);
        connect(ui->horizontalSlider_breaktime, &QSlider::valueChanged, this, &AFBreaktime::qslotVolumeChanged);

        
        ui->pushButton_breaktime_preview->setCheckable(true);

        std::string absPath;
        GetDataFilePath("assets/breaktime/default/play.svg", absPath);
        QString imgPath = QString::fromStdString(absPath);
        QIcon icon(imgPath);
        ui->pushButton_breaktime_preview->setIcon(icon);
        
        ui->pushButton_breaktime_preview->setText(QTStr("breaktime.bgm.preview"));

        PolishStyleSheet(ui->pushButton_breaktime_preview);

        connect(ui->pushButton_breaktime_preview, &QPushButton::toggled, this, &AFBreaktime::qslotPreviewButtonToggled);
        connect(this, &AFBreaktime::signalForcePreviewToggle, this, &AFBreaktime::qslotPreviewButtonToggled);

        ui->checkBox_breaktime->setChecked(true);
        connect(ui->checkBox_breaktime, &QCheckBox::toggled, this, &AFBreaktime::qslotCheckBoxBreaktimeToggled);

        if (ui->lineEdit_Breaktime)
            ui->lineEdit_Breaktime->setMaxLength(currentLocale.compare("en-US") == 0 ? 70 : 30);

        ui->lineEdit_Breaktime->installEventFilter(this);
        applyMessageState(true);
    }
    else
    {    
        ui->pushButton_breaktime_edit->setDefault(false);
        ui->pushButton_breaktime_edit->setAutoDefault(false);
        ui->pushButton_breaktime_edit_2->setDefault(false);
        ui->pushButton_breaktime_edit_2->setAutoDefault(false);
        ui->pushButton_QuestionMark_2->setDefault(false);
        ui->pushButton_QuestionMark_2->setAutoDefault(false);
        ui->pushButton_breaktime_close->setDefault(false);
        ui->pushButton_breaktime_close->setAutoDefault(false);

        connect(&BREAKTIME_MANAGER, &BreaktimeManager::signalTick, this, &AFBreaktime::qslotBreaktimeTicked);
        connect(&BREAKTIME_MANAGER, &BreaktimeManager::signalFinished, this, &AFBreaktime::qslotBreaktimeFinished);

        connect(ui->pushButton_breaktime_cancel_2, &QPushButton::clicked, this, &AFBreaktime::qslotCancelButtonTriggered);

        connect(ui->pushButton_breaktime_stop, &QPushButton::clicked, this, &AFBreaktime::qslotStopButtonTriggered);
        ui->pushButton_breaktime_stop->setProperty("pushButtonTheme", "type5");
        PolishStyleSheet(ui->pushButton_breaktime_stop);

        ui->pushButton_QuestionMark_5->SetExplanationText(QTStr("breaktime.bgm.title.tooltip"), ENUM_TOOLTIP_POSITION::BottomLeft);

        ui->comboBox_breaktime_bgm_2->clear();
        ui->comboBox_breaktime_bgm_2->addItem(QTStr("breaktime.bgm.combo1"), 0);
        ui->comboBox_breaktime_bgm_2->addItem(QTStr("breaktime.bgm.combo2"), 1);
        ui->comboBox_breaktime_bgm_2->addItem(QTStr("breaktime.bgm.combo3"), 2);
        ui->comboBox_breaktime_bgm_2->addItem(QTStr("breaktime.bgm.combo4"), 3);
        ui->comboBox_breaktime_bgm_2->addItem(QTStr("breaktime.bgm.combo5"), 4);
        ui->comboBox_breaktime_bgm_2->setEnabled(false);
                
        ui->pushButton_sound_2->setCheckable(true);
        ui->horizontalSlider_breaktime_2->setRange(0, FADER_PRECISION);

        connect(ui->pushButton_sound_2, &QPushButton::toggled, this, &AFBreaktime::qslotSoundButtonToggled);
        connect(ui->horizontalSlider_breaktime_2, &QSlider::valueChanged, this, &AFBreaktime::qslotVolumeChanged);

        if (ui->lineEdit_Breaktime_2)
            ui->lineEdit_Breaktime_2->setMaxLength(currentLocale.compare("en-US") == 0 ? 70 : 30);

        QWidget* viewPage = ui->page_breaktime_msg_view;
        QWidget* editPage = ui->page_breaktime_msg_edit;

        auto* sw = ui->stackedWidget_breaktime_msg;

        if (sw && viewPage)
            sw->setCurrentWidget(viewPage);

        connect(ui->pushButton_breaktime_edit, &QPushButton::clicked, this, [this, sw, viewPage, editPage]() {
            if (!sw || !editPage) 
                return;
            if (viewPage && sw->currentWidget() != viewPage) 
                return; 

            if (ui->label_breakrtime_text_msg && ui->lineEdit_Breaktime_2) {
                ui->lineEdit_Breaktime_2->setText(ui->label_breakrtime_text_msg->text());
                ui->lineEdit_Breaktime_2->setCursorPosition(ui->lineEdit_Breaktime_2->text().size());
                ui->lineEdit_Breaktime_2->setFocus();
                ui->lineEdit_Breaktime_2->selectAll();
            }
            sw->setCurrentWidget(editPage);
        });

        connect(ui->pushButton_breaktime_edit_2, &QPushButton::clicked, this, [this, sw, viewPage, editPage]() {
            if (!sw || !viewPage) 
                return;
            if (editPage && sw->currentWidget() != editPage) 
                return; 

            if (ui->label_breakrtime_text_msg && ui->lineEdit_Breaktime_2) {
                const QString txt = ui->lineEdit_Breaktime_2->text().trimmed();
                ui->label_breakrtime_text_msg->setText(txt.isEmpty() ? m_defaultMessage : txt);
            }
            sw->setCurrentWidget(viewPage);

            QString changedText = ui->label_breakrtime_text_msg ? ui->label_breakrtime_text_msg->text().trimmed(): QString();
            BREAKTIME_MANAGER.SetSceneText(changedText);
        });

        if (ui->lineEdit_Breaktime_2) {
            connect(ui->lineEdit_Breaktime_2, &QLineEdit::returnPressed, this, [this, sw, viewPage, editPage]() {
                if (!sw || !viewPage || !editPage) 
                    return;
                if (sw->currentWidget() != editPage) 
                    return;

                if (ui->label_breakrtime_text_msg) {
                    const QString txt = ui->lineEdit_Breaktime_2->text().trimmed();
                    ui->label_breakrtime_text_msg->setText(txt.isEmpty() ? m_defaultMessage : txt);
                }

                sw->setCurrentWidget(viewPage);

                QString changedText = ui->label_breakrtime_text_msg ? ui->label_breakrtime_text_msg->text().trimmed() : QString();
                BREAKTIME_MANAGER.SetSceneText(changedText);
            });
        }
    }
  
}

AFBreaktime::~AFBreaktime()
{
    for (auto& slot : m_scenePreviewSlots) {
        if (slot.display)
            obs_display_remove_draw_callback(slot.display->GetDisplay(), _ScenePageRender, slot.ctx.get());
        if (slot.source)
            obs_source_dec_showing(slot.source);
    }
    for (auto& slot : m_scenePreviewSlots2) {
        if (slot.display)
            obs_display_remove_draw_callback(slot.display->GetDisplay(), _ScenePageRender, slot.ctx.get());
        if (slot.source)
            obs_source_dec_showing(slot.source);
    }
    m_scenePreviewSlots.clear();
    m_scenePreviewSlots2.clear();
        
    delete ui;
}

void AFBreaktime::qslotCloseButtonTriggered()
{
    if (auto dlg = qobject_cast<QDialog*>(this->window())) {
        dlg->reject();
    }
}

void AFBreaktime::qslotCancelButtonTriggered()
{
    if (auto dlg = qobject_cast<QDialog*>(this->window())) {
        dlg->reject();
    }
}

void AFBreaktime::qslotApplyButtonTriggered()
{    
    if (!AFOutputUtil::IsStreamActive()) {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", QTStr("breaktime.is.stream"), false, true, "", 0, 0, "type1");
    }

    auto& breaktime = BREAKTIME_MANAGER;

    if (breaktime.IsActive()) {
        if (auto dlg = qobject_cast<QDialog*>(this->window())) {
            dlg->reject();
        }
        return;
    }

    if (breaktime.IsNextActive()) {
        const int remainNext = qMax(0, breaktime.GetNextRemainTime());
        const int min = remainNext / 60;
        const int sec = remainNext % 60;

        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", QTStr("breaktime.next.wait").arg(min).arg(sec), false, true, "", 0, 0, "type1");
        return;
    }

    QString sceneName = GetCurrentSceneName();
    breaktime.SetTimer(GetSelectedMinutes() * 60);
    breaktime.SetAudio(GetSelectedBgm());
    breaktime.SetAudioMuted(IsSoundMuted());
    breaktime.SetAudioVolume(GetSliderVolume());
    breaktime.SetScene(sceneName);
    if (sceneName.isEmpty()) {
        QString msgText = ui->lineEdit_Breaktime ? ui->lineEdit_Breaktime->text().trimmed() : QString();
        breaktime.SetSceneText(msgText);
    }
    breaktime.BreaktimeStartAPI();

    SaveBreaktimeConfig();

    if (auto dlg = qobject_cast<QDialog*>(this->window())) {
        dlg->reject();
    }
}

//즉시 종료
void AFBreaktime::qslotStopButtonTriggered()
{
    BREAKTIME_MANAGER.Stop();

    SaveBreaktimeConfig2();

    if (auto dlg = qobject_cast<QDialog*>(this->window())) {
        dlg->reject();
    }
}

void AFBreaktime::qslotBreaktimeChanged(int index)
{
    if (index < 0) 
        return;
    m_selectedMinutes = ui->comboBox_breaktime_time->itemData(index).toInt();
}

void AFBreaktime::qslotBgmChanged(int index)
{
    if (index < 0) 
        return;
    m_selectedBgm = ui->comboBox_breaktime_bgm->itemData(index).toInt();

    const bool enableAudioControls = (m_selectedBgm != 0);

    ui->pushButton_sound->setVisible(enableAudioControls);
    ui->horizontalSlider_breaktime->setVisible(enableAudioControls);
    ui->pushButton_breaktime_preview->setEnabled(enableAudioControls);

    if (m_isPreviewPlaying)
    {
        ui->pushButton_breaktime_preview->setChecked(false);
        emit signalForcePreviewToggle(false);

    }
}

QString AFBreaktime::GetCurrentSceneName() const
{
    int row = ui->listWidget_breaktime->currentRow();
    if (row == 0)
        return "";
    else if (row > 0 && (row - 1) < m_sceneNames.size())
        return m_sceneNames.at(row - 1);
    return {};
}

bool AFBreaktime::IsMessageChecked() const
{
    return ui->checkBox_breaktime->isChecked();
}

bool AFBreaktime::IsSoundMuted() const
{
    return ui->pushButton_sound->isChecked();
}

bool AFBreaktime::IsSoundMuted2() const
{
    return ui->pushButton_sound_2->isChecked();
}

void AFBreaktime::qslotCheckBoxBreaktimeToggled(bool checked)
{
    if (!IsDefaultSceneSelected()) {
        ui->checkBox_breaktime->setEnabled(false);
        applyDisabledState();
        return;
    }

    applyMessageState(checked);
}

void AFBreaktime::applyMessageState(bool checked, bool focusOnEdit)
{
    if (checked) {
        ui->lineEdit_Breaktime->setEnabled(false);
        ui->lineEdit_Breaktime->setReadOnly(true);
        ui->lineEdit_Breaktime->setStyleSheet("color : rgba(213, 215, 220, 0.4)");
        ui->lineEdit_Breaktime->setText(m_defaultMessage);
    }
    else {
        ui->lineEdit_Breaktime->setEnabled(true);
        ui->lineEdit_Breaktime->setReadOnly(false);

        const QString cur = ui->lineEdit_Breaktime->text();
        if (cur.trimmed().isEmpty() ||
            cur == m_defaultMessage ||
            cur == m_disabledMessage)
        {
            ui->lineEdit_Breaktime->setText(m_editableMessage);
            ui->lineEdit_Breaktime->setStyleSheet("color : rgba(213, 215, 220, 0.4)");
        }
        else
        {
            ui->lineEdit_Breaktime->setStyleSheet("color : rgba(213, 215, 220, 1.0)");
        }
    }
}

void AFBreaktime::applyDisabledState()
{
    ui->lineEdit_Breaktime->setEnabled(false);
    ui->lineEdit_Breaktime->setText(m_disabledMessage);
}

bool AFBreaktime::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == ui->lineEdit_Breaktime && ev->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(ev);
        const int key = ke->key();
        if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            ui->lineEdit_Breaktime->clearFocus();
            return true;
        }
    }
    else if (obj == ui->lineEdit_Breaktime && ev->type() == QEvent::FocusIn)
    {
        if (ui->lineEdit_Breaktime->text() == m_editableMessage)
        {
            ui->lineEdit_Breaktime->setStyleSheet("color : rgba(213, 215, 220, 1.0)");
            ui->lineEdit_Breaktime->setText("");
        }
    }
    else if (obj == ui->lineEdit_Breaktime && ev->type() == QEvent::FocusOut)
    {
        if (ui->lineEdit_Breaktime->text().length() == 0)
        {
            ui->lineEdit_Breaktime->setStyleSheet("color : rgba(213, 215, 220, 0.4)");
            ui->lineEdit_Breaktime->setText(m_editableMessage);
        }
    }
        
    if (obj == ui->listWidget_breaktime->viewport() && ev->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(ev);
        if (auto* item = ui->listWidget_breaktime->itemAt(me->pos())) {
            int row = ui->listWidget_breaktime->row(item);
            if (row > 0) {
                const bool restricted = item->data(RoleRestricted).toBool();
                if (restricted) {
                    const QString id = item->data(RoleRestrictedId).toString();
                    _showRestrictedAlert(id);
                    return true;
                }
            }
        }
    }

    if (obj == ui->listWidget_breaktime && ev->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(ev);
        const int cur = ui->listWidget_breaktime->currentRow();
        int target = cur;
        switch (ke->key()) {
        case Qt::Key_Up:        target = qMax(0, cur - 1); break;
        case Qt::Key_Down:      target = qMin(ui->listWidget_breaktime->count() - 1, cur + 1); break;
        case Qt::Key_Home:      target = 0; break;
        case Qt::Key_End:       target = ui->listWidget_breaktime->count() - 1; break;
        case Qt::Key_PageUp:    target = qMax(0, cur - 10); break;
        case Qt::Key_PageDown:  target = qMin(ui->listWidget_breaktime->count() - 1, cur + 10); break;
        default: break;
        }
        if (target != cur && target > 0) {
            if (auto* item = ui->listWidget_breaktime->item(target)) {
                const bool restricted = item->data(RoleRestricted).toBool();
                if (restricted) {
                    const QString id = item->data(RoleRestrictedId).toString();
                    _showRestrictedAlert(id);
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(obj, ev);
}

bool AFBreaktime::IsDefaultSceneSelected() const {
    return ui->listWidget_breaktime->currentRow() == 0;
}

void AFBreaktime::setBreaktimeScenes(const QStringList& sceneNames)
{
    m_sceneNames.clear();
    for (const auto& n : sceneNames) {
        if (n.isEmpty())            continue;
        if (IsDefaultSceneName(n))  continue;
        if (!m_sceneNames.contains(n))
            m_sceneNames << n;
    }

    ui->listWidget_breaktime->clear();
    auto* defItem = new QListWidgetItem(QTStr("breaktime.scene.default"), ui->listWidget_breaktime);
    defItem->setSizeHint(QSize(defItem->sizeHint().width(), 40));

    for (const auto& name : m_sceneNames) {
        auto* item = new QListWidgetItem(name, ui->listWidget_breaktime);
        item->setSizeHint(QSize(item->sizeHint().width(), 40));
    }

    rebuildScenePages();
    ui->listWidget_breaktime->setCurrentRow(0);
}

void AFBreaktime::rebuildScenePages()
{
    for (auto& slot : m_scenePreviewSlots) {
        if (slot.display) {
            obs_display_remove_draw_callback(slot.display->GetDisplay(),
                _ScenePageRender, slot.ctx.get());
        }
        if (slot.source) {
            obs_source_dec_showing(slot.source);
        }
    }
    m_scenePreviewSlots.clear();

    while (ui->stackedWidget_breaktime_scene->count() > 0) {
        QWidget* w = ui->stackedWidget_breaktime_scene->widget(0);
        ui->stackedWidget_breaktime_scene->removeWidget(w);
        w->deleteLater();
    }

    {
        QWidget* page = new QWidget(ui->stackedWidget_breaktime_scene);
        auto* lay = new QVBoxLayout(page);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(0);

        QLabel* label = new QLabel(page);
        label->setAlignment(Qt::AlignCenter);
        label->setScaledContents(true);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
             
        std::string absPath;
        if (LOCALE_CONTEXT.GetCurrentLocaleStr().compare("ko-KR") == 0)
            GetDataFilePath("assets/breaktime/breaktime.png", absPath);
        else
            GetDataFilePath("assets/breaktime/breaktime_en.png", absPath);
        QString imgPath = QString::fromStdString(absPath);
        QPixmap pix(imgPath);
        
        label->setPixmap(pix);

        if (!pix.isNull()) {
            label->setPixmap(pix.scaled(260, 146, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        label->setAlignment(Qt::AlignCenter);
        lay->addWidget(label);


        ui->stackedWidget_breaktime_scene->addWidget(page);

        ScenePreviewSlot slot;
        slot.display = nullptr;
        slot.source = nullptr;
        slot.ctx = nullptr;

        m_scenePreviewSlots.push_back(std::move(slot));
    }

    for (const auto& name : m_sceneNames) {
        QWidget* page = new QWidget(ui->stackedWidget_breaktime_scene);
        auto* lay = new QVBoxLayout(page);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(0);

        auto* display = new AFQTDisplay(page);
        display->setMinimumSize(260, 146);
        lay->addWidget(display);

        ui->stackedWidget_breaktime_scene->addWidget(page);

        OBSSource src = _getSourceForSceneName(name);

        ScenePreviewSlot slot;
        slot.display = display;
        slot.source = src;
        slot.ctx = std::make_unique<BreaktimeScenePreviewCtx>();
        if (src) {
            obs_source_inc_showing(src);
            slot.ctx->weak = OBSGetWeakRef(src);
        }

        connect(display, &AFQTDisplay::qsignalDisplayCreated, this,
            [display, ctx = slot.ctx.get()]() {
                obs_display_add_draw_callback(display->GetDisplay(), _ScenePageRender, ctx);
                obs_display_set_background_color(display->GetDisplay(),
                GREY_COLOR_BACKGROUND);
            });

        m_scenePreviewSlots.push_back(std::move(slot));
    }

    ui->listWidget_breaktime->setCurrentRow(0);
}

void AFBreaktime::selectSceneByName(const QString& name)
{
    if (name == QTStr("breaktime.scene.default")) {
        ui->listWidget_breaktime->setCurrentRow(0);
    }
    else {
        int row = m_sceneNames.indexOf(name);
        if (row >= 0)
            ui->listWidget_breaktime->setCurrentRow(row + 1);
    }
}

void AFBreaktime::addBreaktimeScene(const QString& sceneName)
{
    if (sceneName.isEmpty() || m_sceneNames.contains(sceneName))
        return;

    m_sceneNames << sceneName;
    auto* item = new QListWidgetItem(sceneName, ui->listWidget_breaktime);
    item->setSizeHint(QSize(item->sizeHint().width(), 40));

    QWidget* page = new QWidget(ui->stackedWidget_breaktime_scene);
    auto* lay = new QVBoxLayout(page);
    lay->setContentsMargins(8, 8, 8, 8);

    auto* display = new AFQTDisplay(page);
    display->setMinimumSize(260, 146);
    lay->addWidget(display);

    ui->stackedWidget_breaktime_scene->addWidget(page);

    OBSSource src = _getSourceForSceneName(sceneName);

    ScenePreviewSlot slot;
    slot.display = display;
    slot.source = src;
    slot.ctx = std::make_unique<BreaktimeScenePreviewCtx>();
    if (src) {
        obs_source_inc_showing(src);
        slot.ctx->weak = OBSGetWeakRef(src);
    }

    connect(display, &AFQTDisplay::qsignalDisplayCreated, this,
        [display, ctx = slot.ctx.get()]() {
            obs_display_add_draw_callback(display->GetDisplay(),_ScenePageRender, ctx);
            obs_display_set_background_color(display->GetDisplay(),
            GREY_COLOR_BACKGROUND);
        });

    m_scenePreviewSlots.push_back(std::move(slot));

    ui->listWidget_breaktime->setCurrentItem(item);
}


int AFBreaktime::GetSliderVolume() const {
    return ui->horizontalSlider_breaktime->value();
}

int AFBreaktime::GetSliderVolume2() const {
    return ui->horizontalSlider_breaktime_2->value();
}

void AFBreaktime::qslotVolumeChanged(int value)
{
    const int sliderValue = qBound(0, value, FADER_PRECISION);

    BREAKTIME_MANAGER.SetAudioVolume(sliderValue);

    if (ui->horizontalSlider_breaktime) {
        QSignalBlocker b(ui->horizontalSlider_breaktime);
        ui->horizontalSlider_breaktime->setValue(sliderValue);
    }
    if (ui->horizontalSlider_breaktime_2) {
        QSignalBlocker b2(ui->horizontalSlider_breaktime_2);
        ui->horizontalSlider_breaktime_2->setValue(sliderValue);
    }
}

void AFBreaktime::qslotSoundButtonToggled(bool checked)
{
    BREAKTIME_MANAGER.SetAudioMuted(checked);

    if (ui->pushButton_sound) {
        QSignalBlocker b(ui->pushButton_sound);
        ui->pushButton_sound->setChecked(checked);
    }
    if (ui->pushButton_sound_2) {
        QSignalBlocker b2(ui->pushButton_sound_2);
        ui->pushButton_sound_2->setChecked(checked);
    }
}

QString AFBreaktime::GetMessageText() const
{
    return ui->lineEdit_Breaktime->text();
}

QString AFBreaktime::GetEditableMessageText() const
{
    return m_editableMessage;
}


void AFBreaktime::SetInitialState(int minutes, int bgm, bool msgChecked, bool soundMuted, int volume, const QString& sceneName, const QString& messageText)
{
    minutes = qBound(1, minutes, 10);
    bgm = qBound(0, bgm, 4);
    volume = qBound(0, volume, FADER_PRECISION);

    {
        QSignalBlocker b1(ui->comboBox_breaktime_time);
        ui->comboBox_breaktime_time->setCurrentIndex(minutes - 1);
        m_selectedMinutes = minutes;
    }
    {
        QSignalBlocker b2(ui->comboBox_breaktime_bgm);
        int bgmIdx = ui->comboBox_breaktime_bgm->findData(bgm);
        if (bgmIdx >= 0) ui->comboBox_breaktime_bgm->setCurrentIndex(bgmIdx);
        m_selectedBgm = bgm;

        ui->pushButton_sound->setVisible(m_selectedBgm != 0);
        ui->horizontalSlider_breaktime->setVisible(m_selectedBgm != 0);
        ui->pushButton_breaktime_preview->setEnabled(m_selectedBgm != 0);
    }
    {
        QSignalBlocker b3(ui->horizontalSlider_breaktime);
        ui->horizontalSlider_breaktime->setValue(volume);
    }
    ui->pushButton_sound->setChecked(soundMuted);

    if (!sceneName.isEmpty()) {

        int row = ui->listWidget_breaktime->currentRow(); 
        if (row > 0) {
            if (auto* item = ui->listWidget_breaktime->item(row)) {
                const bool restricted = item->data(RoleRestricted).toBool();
                if (restricted) {
                    ui->listWidget_breaktime->setCurrentRow(0);
                    if (ui->stackedWidget_breaktime_scene)
                        ui->stackedWidget_breaktime_scene->setCurrentIndex(0);
                    m_lastValidSceneRow = 0;

                    selectSceneByName(QTStr("breaktime.scene.default"));
                }
                else
                {
                    selectSceneByName(sceneName);
                }
            }
        }
        else
        {
            selectSceneByName(sceneName);
        }
    }

    const bool isDefaultScene = IsDefaultSceneSelected();

    if (isDefaultScene) {
        {
            QSignalBlocker b4(ui->checkBox_breaktime);
            ui->checkBox_breaktime->setEnabled(true);
            ui->checkBox_breaktime->setChecked(msgChecked);
        }

        if (!msgChecked) {
            const QString txt = messageText.trimmed();
            if (!txt.isEmpty() && txt != m_editableMessage)
                ui->lineEdit_Breaktime->setText(txt);
        }

        applyMessageState(msgChecked, false);
    }
    else {
        ui->checkBox_breaktime->setEnabled(false);
        applyDisabledState();
    }
}


void AFBreaktime::SetInitialState_Playing(int nRemainingSec, int bgm, bool msgChecked, bool soundMuted, int volume, const QString& sceneName, const QString& messageText)
{
    UpdateRemainingTimeLabel(nRemainingSec);

    bgm = qBound(0, bgm, 4);
    volume = qBound(0, volume, FADER_PRECISION);

    _buildPlayingScene(sceneName);
                
    ui->label_breaktime_scene->setText( QStringLiteral("%1 %2").arg(QTStr("breaktime.scene"), sceneName));
    
    {
        QSignalBlocker b(ui->comboBox_breaktime_bgm_2);
        int bgmIdx = ui->comboBox_breaktime_bgm_2->findData(bgm);
        if (bgmIdx < 0) bgmIdx = 0;
        ui->comboBox_breaktime_bgm_2->setCurrentIndex(bgmIdx);
        ui->comboBox_breaktime_bgm_2->setEnabled(false);

        ui->pushButton_sound_2->setVisible(bgmIdx != 0);
        ui->horizontalSlider_breaktime_2->setVisible(bgmIdx != 0);
    }
    ui->pushButton_sound_2->setCheckable(true);
    ui->pushButton_sound_2->setChecked(soundMuted);
    
    ui->horizontalSlider_breaktime_2->setRange(0, FADER_PRECISION);
    {
        QSignalBlocker b(ui->horizontalSlider_breaktime_2);
        ui->horizontalSlider_breaktime_2->setValue(volume);
    }

    const bool isDefault = IsDefaultSceneName(sceneName);
    if (isDefault) {
        if (ui->widget_14) {
            ui->widget_14->setVisible(true);
            ui->widget_14->setEnabled(true);
        }
        if (ui->lineEdit_Breaktime_2) {
            const QString txt = messageText.trimmed();
            ui->lineEdit_Breaktime_2->setVisible(true);
            ui->lineEdit_Breaktime_2->setEnabled(true);
            ui->lineEdit_Breaktime_2->setText(!txt.isEmpty() ? txt : m_defaultMessage);
        }
    }
    else {
        if (ui->widget_14) {
            ui->widget_14->setVisible(false);
            ui->widget_14->setEnabled(false);
        }
    }

    if (auto dlg = qobject_cast<QDialog*>(this->window())) {
        dlg->resize(dlg->width(), isDefault ? 790 : 685);
    }

    if (ui->label_breakrtime_text_msg) {
        const QString txt = messageText.trimmed();
        ui->label_breakrtime_text_msg->setText(!txt.isEmpty() ? txt : m_defaultMessage);
    }

    int totalSec = this->property("bt_totalSec").toInt();
    bool enableStop = false;
    if (totalSec > 0) {
        const int elapsed = totalSec - nRemainingSec;
        enableStop = (elapsed >= 60);
    }
    if (ui->pushButton_breaktime_stop)
        ui->pushButton_breaktime_stop->setEnabled(enableStop);
}


void AFBreaktime::qslotPreviewButtonToggled(bool checked)
{
    if (m_isPreviewPlaying == checked)
        return;

    m_isPreviewPlaying = checked;

    auto& breaktime = BREAKTIME_MANAGER;
    breaktime.SetAudio(GetSelectedBgm());
    breaktime.SetAudioMuted(IsSoundMuted());
    breaktime.SetAudioVolume(GetSliderVolume());
    
    std::string absPath;
    if (checked) {
        breaktime.PlayAudio();
        GetDataFilePath("assets/breaktime/default/stop.svg", absPath);
    } else {
        breaktime.StopAudio();
        GetDataFilePath("assets/breaktime/default/play.svg", absPath);
    }

    QString imgPath = QString::fromStdString(absPath);
    QIcon icon(imgPath);
    ui->pushButton_breaktime_preview->setIcon(icon);

    PolishStyleSheet(ui->pushButton_breaktime_preview);
}

OBSSource AFBreaktime::_getSourceForSceneName(const QString& name) const
{
    SceneItemVector& items = SCENE_CONTEXT.GetSceneItemVector();
    for (size_t i = 0; i < items.size(); ++i) {
        const char* sname = items.at(i)->GetSceneName();
        if (!sname || !*sname) continue;
        if (name == QString::fromUtf8(sname)) {
            OBSScene scene = items.at(i)->GetScene();
            return obs_scene_get_source(scene); // strong ref
        }
    }
    return nullptr;
}

static void gs_push_states(bool& prevLinear)
{
    gs_viewport_push();
    gs_projection_push();
    prevLinear = gs_set_linear_srgb(true);
}

static void gs_pop_states(bool prevLinear)
{
    gs_set_linear_srgb(prevLinear);
    gs_projection_pop();
    gs_viewport_pop();
}

static void compute_scaled_viewport(uint32_t srcW, uint32_t srcH,
    uint32_t viewW, uint32_t viewH,
    int& x, int& y, int& newW, int& newH, float& scale)
{
    if (srcW == 0) srcW = 1;
    if (srcH == 0) srcH = 1;
    const float sx = float(viewW) / float(srcW);
    const float sy = float(viewH) / float(srcH);
    scale = std::min(sx, sy);
    newW = int(scale * float(srcW));
    newH = int(scale * float(srcH));
    x = (int(viewW) - newW) / 2;
    y = (int(viewH) - newH) / 2;
}

void AFBreaktime::_ScenePageRender(void* data, uint32_t cx, uint32_t cy)
{
    auto* ctx = static_cast<BreaktimeScenePreviewCtx*>(data);
    if (!ctx) return;

    OBSSource strong = OBSGetStrongRef(ctx->weak);
    if (!strong) return;

    uint32_t sw = std::max(obs_source_get_width(strong), 1u);
    uint32_t sh = std::max(obs_source_get_height(strong), 1u);

    int x, y, vw, vh; float scale;
    compute_scaled_viewport(sw, sh, cx, cy, x, y, vw, vh, scale);

    bool prevLinear = false;
    gs_push_states(prevLinear);

    gs_ortho(0.0f, float(sw), 0.0f, float(sh), -100.0f, 100.0f);
    gs_set_viewport(x, y, vw, vh);

    obs_source_video_render(strong);

    gs_pop_states(prevLinear);
}

void AFBreaktime::_buildPlayingScene(const QString& sceneName)
{
    for (auto& slot : m_scenePreviewSlots2) {
        if (slot.display)
            obs_display_remove_draw_callback(slot.display->GetDisplay(), _ScenePageRender, slot.ctx.get());
        if (slot.source)
            obs_source_dec_showing(slot.source);
    }
    m_scenePreviewSlots2.clear();

    while (ui->stackedWidget_breaktime_scene_2->count() > 0) {
        QWidget* w = ui->stackedWidget_breaktime_scene_2->widget(0);
        ui->stackedWidget_breaktime_scene_2->removeWidget(w);
        w->deleteLater();
    }

    const bool isDefault = (sceneName == QTStr("breaktime.scene.default"));

    QWidget* page = new QWidget(ui->stackedWidget_breaktime_scene_2);
    auto* lay = new QVBoxLayout(page);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    ScenePreviewSlot slot;

    if (isDefault) {
        auto* label = new QLabel(page);
        label->setAlignment(Qt::AlignCenter);
        label->setScaledContents(true);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        
        std::string absPath;
        if (LOCALE_CONTEXT.GetCurrentLocaleStr().compare("ko-KR") == 0)
            GetDataFilePath("assets/breaktime/breaktime.png", absPath);
        else
            GetDataFilePath("assets/breaktime/breaktime_en.png", absPath);
        QString imgPath = QString::fromStdString(absPath);
        QPixmap pix(imgPath);
        if (!pix.isNull()) {
            label->setPixmap(pix.scaled(260, 146, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        lay->addWidget(label);
    }
    else {
        auto* display = new AFQTDisplay(page);
        display->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        lay->addWidget(display);

        OBSSource src = _getSourceForSceneName(sceneName);
        slot.display = display;
        slot.source = src;
        slot.ctx = std::make_unique<BreaktimeScenePreviewCtx>();

        if (src) {
            obs_source_inc_showing(src);
            slot.ctx->weak = OBSGetWeakRef(src);
        }
        connect(display, &AFQTDisplay::qsignalDisplayCreated, this,
            [display, ctx = slot.ctx.get()]() {
                obs_display_add_draw_callback(display->GetDisplay(), _ScenePageRender, ctx);
                obs_display_set_background_color(display->GetDisplay(), GREY_COLOR_BACKGROUND);
            });
    }

    ui->stackedWidget_breaktime_scene_2->addWidget(page);
    ui->stackedWidget_breaktime_scene_2->setCurrentIndex(0);

    m_scenePreviewSlots2.push_back(std::move(slot));
}

void AFBreaktime::UpdateRemainingTimeLabel(int remainingSec)
{
    if (ui->label_breaktime_remaining_time)
        ui->label_breaktime_remaining_time->setText(fmtMMSS(remainingSec));
}

void AFBreaktime::qslotBreaktimeTicked(int remainingSec, int totalSec)
{
    UpdateRemainingTimeLabel(remainingSec);

    const int elapsed = qMax(0, totalSec - remainingSec);
    if (ui->pushButton_breaktime_stop)
        ui->pushButton_breaktime_stop->setEnabled(elapsed >= 60);
}


void AFBreaktime::qslotBreaktimeFinished()
{
    BREAKTIME_MANAGER.Stop();

    SaveBreaktimeConfig2();

    if (ui->pushButton_breaktime_stop) {
        ui->pushButton_breaktime_stop->setEnabled(false);
    }
        
    if (auto dlg = qobject_cast<QDialog*>(this->window())) {
        dlg->accept();
    } 
}

bool AFBreaktime::_isSceneRestricted(OBSScene scene, QString* hitId)
{
    if (!scene) return false;
    struct Ctx { bool found = false; QString id; } ctx;

    obs_scene_enum_items(scene,
        [](obs_scene_t*, obs_sceneitem_t* item, void* param)->bool {
            auto* c = static_cast<Ctx*>(param);
            if (c->found) return false;
            if (obs_source_t* src = obs_sceneitem_get_source(item)) {
                const char* cid = obs_source_get_id(src);
                if (cid && *cid) {
                    const QString id = QString::fromUtf8(cid);
                    static const QSet<QString> restrictedIds = {
                        "soop_tv_cable_source",
                        "soop_dramavod_source",
                        "soop_movievod_source",
                        "soop_anivod_source",
                        "soop_sportvod_source",
                        "soop_directbroad_source"
                    };
                    if (restrictedIds.contains(id)) {
                        c->found = true;
                        c->id = id;
                        return false;
                    }
                }
            }
            return true;
        }, &ctx);

    if (ctx.found && hitId) *hitId = ctx.id;
    return ctx.found;
}

void AFBreaktime::_markSceneItemRestricted(QListWidgetItem* item, const QString& hitId)
{
    if (!item) return;
    item->setData(RoleRestricted, true);
    item->setData(RoleRestrictedId, hitId);
}

void AFBreaktime::_showRestrictedAlert(const QString& id)
{
    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", QTStr("breaktime.restricted"), false, true, "", 0, 0, "type1");
}


void AFBreaktime::qslotSceneItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    if (!current) return;
    const int row = ui->listWidget_breaktime->row(current);

    if (row == 0) {
        m_lastValidSceneRow = 0;
        ui->stackedWidget_breaktime_scene->setCurrentIndex(0);
        ui->checkBox_breaktime->setEnabled(true);
        applyMessageState(ui->checkBox_breaktime->isChecked());
        return;
    }

    if (current->data(RoleRestricted).toBool()) {
        _showRestrictedAlert(current->data(RoleRestrictedId).toString());
        return;
    }

    m_lastValidSceneRow = row;
    ui->stackedWidget_breaktime_scene->setCurrentIndex(row);
    ui->checkBox_breaktime->setEnabled(false);
    applyDisabledState();
}


bool AFBreaktime::SaveBreaktimeConfig()
{
    int minutes = qBound(1, GetSelectedMinutes(), 10);
    int bgm = qBound(0, GetSelectedBgm(), 4);
    int volume = qBound(0, GetSliderVolume(), FADER_PRECISION);
    bool muted = IsSoundMuted();
    bool msgChk = IsMessageChecked();
    QString sceneName = GetCurrentSceneName();

    QString msgText = ui && ui->lineEdit_Breaktime
        ? ui->lineEdit_Breaktime->text().trimmed()
        : QString();
        
    int row = ui && ui->listWidget_breaktime ? ui->listWidget_breaktime->currentRow() : -1;
    if (row == 0) {
        sceneName = QTStr("breaktime.scene.default");
    }

    auto config = ACTIVECONFIG;
    if (!config) return false;

    config_set_string(config, "BreakTime", "BreakTime.Scene", sceneName.toUtf8().constData());
    config_set_int(config, "BreakTime", "BreakTime.Time", minutes);
    config_set_int(config, "BreakTime", "BreakTime.BGM", bgm);
    config_set_int(config, "BreakTime", "BreakTime.MSG.Check", msgChk ? 1 : 0);
    config_set_string(config, "BreakTime", "BreakTime.MSG.Text", msgText.toUtf8().constData());

    config_save(config);
    return true;
}

bool AFBreaktime::SaveBreaktimeConfig2()
{
    int volume = qBound(0, GetSliderVolume2(), FADER_PRECISION);
    bool muted = IsSoundMuted2();

    auto config = ACTIVECONFIG;
    if (!config) return false;

    if (ui && ui->widget_14 && ui->widget_14->isVisible() && ui->label_breakrtime_text_msg) {
        QString msgText = ui->label_breakrtime_text_msg->text().trimmed();
        config_set_string(config, "BreakTime", "BreakTime.MSG.Text", msgText.toUtf8().constData());
    }

    config_save(config);
    return true;
}