#include "CSplitEffectDialog.h"
#include "ui_split-effect-dialog.h"

#include "qt-wrappers.hpp"

#include "Blocks/CBlockManager.h"

#include "MainFrame/CMainFrame.h"

#define CHECKED_PROPERTY                    "checked"
#define SETTING_PRESET_TYPE_DEFAULT         "preset_default"
#define SETTING_PRESET_TYPE_TWOWAY          "preset_2"
#define SETTING_PRESET_TYPE_THREEWAY        "preset_3"
#define SETTING_PRESET_TYPE_FOURWAY         "preset_4"
#define SETTING_PRESET_TYPE_FIVEWAY         "preset_5"
#define SETTING_PRESET_TYPE_SIXWAY          "preset_6"
#define SETTING_PRESET_TYPE_HORIZONTALFLIP  "preset_horizontal_flip"
#define SETTING_PRESET_TYPE_COLOREDSPLIT    "preset_3_rgb"

#define SPLIT_FILTER_NAME "soop_split_shader_filter"

static OBSSource GetSource(OBSWeakSource weakSource) {
    return OBSGetStrongRef(weakSource);
}

AFQSplitEffectDialog::AFQSplitEffectDialog(QWidget* parent, OBSSource source) :
    AFTTopBaseDialog(parent, Qt::WindowFlags()),
    ui(new Ui::AFQSplitEffectDialog),
    m_weakSource(OBSGetWeakRef(source))
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose, true);

#ifdef _WIN32
    AFQBlockManager::ApplyMoveInAllArea(this);
#elif defined(__APPLE__)
    ui->titleFrame->hide();
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    setWindowTitle(QTStr("Effect.Split"));
#endif
    
    SetWidthResizeEnabled(false);
    SetHeightResizeEnabled(false);

    _Init();
 }

AFQSplitEffectDialog::~AFQSplitEffectDialog()
{
    if (m_currentSplitType == SplitType::None) {
        OBSSource source = GetSource(m_weakSource);
        OBSDataAutoRelease settings = obs_source_get_settings(source);
        
        const char* splitFilterName = obs_data_get_string(settings, SAVED_SPLIT_FILTER_NAME);
        if (splitFilterName && *splitFilterName != '\0')
        {
            OBSSourceAutoRelease filter = obs_source_get_filter_by_name(source, splitFilterName);
            if (filter)
                obs_source_filter_remove(source, filter);
            
            obs_data_erase(settings, SAVED_SPLIT_FILTER_NAME);
        }
    }

    m_weakSource = nullptr;
    delete ui;
}

void AFQSplitEffectDialog::_Init()
{
    OBSSource source = GetSource(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);

    SplitType currentSplitType = SplitType::None;
    bool filterApplied = false;

    const char* splitFilterName = obs_data_get_string(settings, SAVED_SPLIT_FILTER_NAME);
    if (splitFilterName && *splitFilterName != '\0')
    {
        OBSSourceAutoRelease filter = obs_source_get_filter_by_name(source, splitFilterName);
        if (filter) 
        {
            OBSDataAutoRelease filterData = obs_source_get_settings(filter);
            if (filterData)
            {
                const char* presetType = obs_data_get_string(filterData, PRESET_PROPERTY);
                filterApplied = true;
            
                if (!presetType || *presetType == '\0' || strcmp(presetType, "preset_default") == 0)
                {
                    presetType = "preset_default";
                    filterApplied = false;
                }

                currentSplitType = GetPresetType(presetType);
                _UnCheckAllFilterUi();
            }
        }
    }

    _SetCurrentCheckType(currentSplitType, filterApplied);

    connect(ui->widget_TwoWaySplit, &AFQHoverWidget::qsignalMouseClick,
            this, [this] { _qslotClickedSplit(SplitType::TwoWay); });
    connect(ui->widget_ThreeWaySplit, &AFQHoverWidget::qsignalMouseClick,
            this, [this] { _qslotClickedSplit(SplitType::ThreeWay); });
    connect(ui->widget_FourWaySplit, &AFQHoverWidget::qsignalMouseClick,
            this, [this] { _qslotClickedSplit(SplitType::FourWay); });
    connect(ui->widget_FiveWaySplit, &AFQHoverWidget::qsignalMouseClick,
            this, [this] { _qslotClickedSplit(SplitType::FiveWay); });
    connect(ui->widget_SixWaySplit, &AFQHoverWidget::qsignalMouseClick,
            this, [this] { _qslotClickedSplit(SplitType::SixWay); });
    connect(ui->widget_HorizontalFlipSplit, &AFQHoverWidget::qsignalMouseClick,
            this, [this] { _qslotClickedSplit(SplitType::HorizontalFlip); });
    connect(ui->widget_ColoredSplit, &AFQHoverWidget::qsignalMouseClick,
            this, [this] { _qslotClickedSplit(SplitType::ColoredSplit); });
    
    connect(ui->buttonClose, &QPushButton::clicked,
            this, &AFQSplitEffectDialog::close);
}

void AFQSplitEffectDialog::_SetCurrentCheckType(SplitType type, bool checked)
{
    if (type == SplitType::None) {
        _UnCheckAllFilterUi();
        m_currentSplitType = SplitType::None;
    }
    else {
        if (!checked) {
            _UpdateUiStyle(type, false);
            m_currentSplitType = SplitType::None;
        }
        else {
            _UpdateUiStyle(m_currentSplitType, false);
            _UpdateUiStyle(type, true);
            m_currentSplitType = type;
        }
    }

    SetFilterData(m_currentSplitType, GetSource(m_weakSource));

    emit qsignalSplitFilterActivated();
}

void AFQSplitEffectDialog::_UpdateUiStyle(SplitType type, bool checked)
{
    QWidget* widget = nullptr;
    QLabel* label = nullptr;

    switch (type) 
    {
    case SplitType::TwoWay:
        widget = ui->widget_TwoWaySplit;
        label = ui->label_TwoWaySplitApplied;
        break;
    case SplitType::ThreeWay:
        widget = ui->widget_ThreeWaySplit;
        label = ui->label_ThreeWaySplitApplied;
        break;
    case SplitType::FourWay:
        widget = ui->widget_FourWaySplit;
        label = ui->label_FourWaySplitApplied;
        break;
    case SplitType::FiveWay:
        widget = ui->widget_FiveWaySplit;
        label = ui->label_FiveWaySplitApplied;
        break;
    case SplitType::SixWay:
        widget = ui->widget_SixWaySplit;
        label = ui->label_SixWaySplitApplied;
        break;
    case SplitType::HorizontalFlip:
        widget = ui->widget_HorizontalFlipSplit;
        label = ui->label_HorizontalFlipSplitApplied;
        break;
    case SplitType::ColoredSplit:
        widget = ui->widget_ColoredSplit;
        label = ui->label__ColoredSplitApplied;
        break;
    default:
        break;
    }

    if (widget) {
        widget->setProperty(CHECKED_PROPERTY, checked);
        PolishStyleSheet(widget);
    }

    if (label)
        label->setVisible(checked);
}

void AFQSplitEffectDialog::_UnCheckAllFilterUi()
{
    _UpdateUiStyle(SplitType::TwoWay, false);
    _UpdateUiStyle(SplitType::ThreeWay, false);
    _UpdateUiStyle(SplitType::FourWay, false);
    _UpdateUiStyle(SplitType::FiveWay, false);
    _UpdateUiStyle(SplitType::SixWay, false);
    _UpdateUiStyle(SplitType::HorizontalFlip, false);
    _UpdateUiStyle(SplitType::ColoredSplit, false);
}

void AFQSplitEffectDialog::RefreshSplitFilterUI()
{
    OBSSource source = GetSource(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);

    SplitType currentSplitType = SplitType::None;
    bool filterApplied = false;

    const char* splitFilterName = obs_data_get_string(settings, SAVED_SPLIT_FILTER_NAME);
    if (splitFilterName && *splitFilterName != '\0')
    {
        OBSSourceAutoRelease filter = obs_source_get_filter_by_name(source, splitFilterName);
        if (filter)
        {
            OBSDataAutoRelease filterData = obs_source_get_settings(filter);
            if (filterData)
            {
                const char* presetType = obs_data_get_string(filterData, PRESET_PROPERTY);
                filterApplied = true;

                if (!presetType || *presetType == '\0' || strcmp(presetType, "preset_default") == 0)
                {
                    presetType = "preset_default";
                    filterApplied = false;
                }

                currentSplitType = GetPresetType(presetType);
                _UnCheckAllFilterUi();
                if (SplitType::None != currentSplitType)
                {
                    _UpdateUiStyle(currentSplitType, true);
                }

                m_currentSplitType = currentSplitType;
            }
        }
    }
}

void AFQSplitEffectDialog::_qslotClickedSplit(SplitType type)
{
    QWidget* widget = nullptr;

    switch (type) {
    case SplitType::TwoWay: 
        widget = ui->widget_TwoWaySplit; 
        break;
    case SplitType::ThreeWay: 
        widget = ui->widget_ThreeWaySplit; 
        break;
    case SplitType::FourWay: 
        widget = ui->widget_FourWaySplit; 
        break;
    case SplitType::FiveWay: 
        widget = ui->widget_FiveWaySplit; 
        break;
    case SplitType::SixWay: 
        widget = ui->widget_SixWaySplit; 
        break;
    case SplitType::HorizontalFlip: 
        widget = ui->widget_HorizontalFlipSplit; 
        break;
    case SplitType::ColoredSplit: 
        widget = ui->widget_ColoredSplit; 
        break;
    default: 
        return;
    }

    if (widget) {
        bool isChecked = widget->property(CHECKED_PROPERTY).toBool();
        _SetCurrentCheckType(type, !isChecked);
    }
}

void AFQSplitEffectDialog::SetFilterData(SplitType type, OBSSource source, bool fromToolBar)
{
    if (!source)
        return;

    OBSDataAutoRelease settings = obs_source_get_settings(source);
    QString splitFilterName = QT_UTF8(obs_data_get_string(settings, SAVED_SPLIT_FILTER_NAME));
    
    // Set Filter Name
    if (splitFilterName.isEmpty()) {
        splitFilterName = SPLIT_FILTER_NAME;

        const int MAX_ATTEMPTS = 100;
        int attempt = 0;

        OBSSourceAutoRelease existingFilter = obs_source_get_filter_by_name(source, QT_TO_UTF8(splitFilterName));
        while (existingFilter && attempt < MAX_ATTEMPTS) {
            splitFilterName += "_";
            existingFilter = obs_source_get_filter_by_name(source, QT_TO_UTF8(splitFilterName));
            ++attempt;
        }

        if (attempt == MAX_ATTEMPTS) {
            qWarning() << "Failed to find an available split filter name";
            return;
        }

        obs_data_set_string(settings, SAVED_SPLIT_FILTER_NAME, QT_TO_UTF8(splitFilterName));
    }

    OBSSourceAutoRelease filter = obs_source_get_filter_by_name(source, QT_TO_UTF8(splitFilterName));

    // Create Filter
    if (!filter) {
        filter = obs_source_create(SPLIT_FILTER_ID, QT_TO_UTF8(splitFilterName), nullptr, nullptr);
        if (!filter)
            return;
        obs_source_filter_add(source, filter);
    }

    OBSDataAutoRelease filterData = obs_source_get_settings(filter);

    if (!filterData)
        filterData = obs_data_create();

    obs_properties_t* properties = obs_source_properties(filter);
    if (!properties) {
        qWarning() << "Failed to get properties for filter.";
        return;
    }

    obs_property_t* property = obs_properties_get(properties, PRESET_PROPERTY);
    if (!property) {
        qWarning() << "Failed to get preset property from properties.";
        return;
    }

    if (type == SplitType::TwoWay) {
        obs_data_set_string(filterData, PRESET_PROPERTY, SETTING_PRESET_TYPE_TWOWAY);
        obs_data_set_string(filterData, PRE_SPLIT_TYPE_PROPERTY, SETTING_PRESET_TYPE_TWOWAY);
    }
    else if (type == SplitType::ThreeWay) {
        obs_data_set_string(filterData, PRESET_PROPERTY, SETTING_PRESET_TYPE_THREEWAY);
        obs_data_set_string(filterData, PRE_SPLIT_TYPE_PROPERTY, SETTING_PRESET_TYPE_THREEWAY);
    }
    else if (type == SplitType::FourWay) {
        obs_data_set_string(filterData, PRESET_PROPERTY, SETTING_PRESET_TYPE_FOURWAY);
        obs_data_set_string(filterData, PRE_SPLIT_TYPE_PROPERTY, SETTING_PRESET_TYPE_FOURWAY);

    }
    else if (type == SplitType::FiveWay) {
        obs_data_set_string(filterData, PRESET_PROPERTY, SETTING_PRESET_TYPE_FIVEWAY);
        obs_data_set_string(filterData, PRE_SPLIT_TYPE_PROPERTY, SETTING_PRESET_TYPE_FIVEWAY);
    }
    else if (type == SplitType::SixWay) {
        obs_data_set_string(filterData, PRESET_PROPERTY, SETTING_PRESET_TYPE_SIXWAY);
        obs_data_set_string(filterData, PRE_SPLIT_TYPE_PROPERTY, SETTING_PRESET_TYPE_SIXWAY);
    }
    else if (type == SplitType::HorizontalFlip) {
        obs_data_set_string(filterData, PRESET_PROPERTY, SETTING_PRESET_TYPE_HORIZONTALFLIP);
        obs_data_set_string(filterData, PRE_SPLIT_TYPE_PROPERTY, SETTING_PRESET_TYPE_HORIZONTALFLIP);
    }
    else if (type == SplitType::ColoredSplit) {
        obs_data_set_string(filterData, PRESET_PROPERTY, SETTING_PRESET_TYPE_COLOREDSPLIT);
        obs_data_set_string(filterData, PRE_SPLIT_TYPE_PROPERTY, SETTING_PRESET_TYPE_COLOREDSPLIT);

    }
    else // SplitType::None
    {
        obs_data_set_string(filterData, PRESET_PROPERTY, SETTING_PRESET_TYPE_DEFAULT);
        if(!fromToolBar)
            obs_data_set_string(filterData, PRE_SPLIT_TYPE_PROPERTY, SETTING_PRESET_TYPE_TWOWAY);
    }

    obs_source_update(filter, filterData);
    obs_property_modified(property, filterData);

    obs_properties_destroy(properties);
}

AFQSplitEffectDialog::SplitType AFQSplitEffectDialog::GetPresetType(const char* type)
{
    if (strcmp(type, SETTING_PRESET_TYPE_TWOWAY) == 0)
        return AFQSplitEffectDialog::SplitType::TwoWay;
    else if (strcmp(type, SETTING_PRESET_TYPE_THREEWAY) == 0)
        return AFQSplitEffectDialog::SplitType::ThreeWay;
    else if (strcmp(type, SETTING_PRESET_TYPE_FOURWAY) == 0)
        return AFQSplitEffectDialog::SplitType::FourWay;
    else if (strcmp(type, SETTING_PRESET_TYPE_FIVEWAY) == 0)
        return AFQSplitEffectDialog::SplitType::FiveWay;
    else if (strcmp(type, SETTING_PRESET_TYPE_SIXWAY) == 0)
        return AFQSplitEffectDialog::SplitType::SixWay;
    else if (strcmp(type, SETTING_PRESET_TYPE_HORIZONTALFLIP) == 0)
        return AFQSplitEffectDialog::SplitType::HorizontalFlip;
    else if (strcmp(type, SETTING_PRESET_TYPE_COLOREDSPLIT) == 0)
        return AFQSplitEffectDialog::SplitType::ColoredSplit;
    else
        return AFQSplitEffectDialog::SplitType::None;
}
