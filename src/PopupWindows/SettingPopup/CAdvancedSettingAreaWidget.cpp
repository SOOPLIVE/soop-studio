#include "CAdvancedSettingAreaWidget.h"
#include "ui_advanced-setting-area.h"

#include <QStandardItemModel>
#include <QCompleter>

#include "Common/EventFilterUtils.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "include/qt-wrapper.h"
#include "platform/platform.hpp"
#include "Application/CApplication.h"

#include <util/platform.h>

#include "CSettingAreaUtils.h"

#define ADV_CHANGED     &AFQAdvancedSettingAreaWidget::qslotAdvancedChanged
#define ADV_RESTART     &AFQAdvancedSettingAreaWidget::qslotAdvancedChangedRestart
#define ESTIMATE_STR "Basic.Settings.Output.ReplayBuffer.Estimate"
#define ESTIMATE_TOO_LARGE_STR \
	"Basic.Settings.Output.ReplayBuffer.EstimateTooLarge"
#define ESTIMATE_UNKNOWN_STR \
	"Basic.Settings.Output.ReplayBuffer.EstimateUnknown"

static inline QString makeFormatToolTip()
{
	static const char* format_list[][2] = {
		{"CCYY", "FilenameFormatting.TT.CCYY"},
		{"YY", "FilenameFormatting.TT.YY"},
		{"MM", "FilenameFormatting.TT.MM"},
		{"DD", "FilenameFormatting.TT.DD"},
		{"hh", "FilenameFormatting.TT.hh"},
		{"mm", "FilenameFormatting.TT.mm"},
		{"ss", "FilenameFormatting.TT.ss"},
		{"%", "FilenameFormatting.TT.Percent"},
		{"a", "FilenameFormatting.TT.a"},
		{"A", "FilenameFormatting.TT.A"},
		{"b", "FilenameFormatting.TT.b"},
		{"B", "FilenameFormatting.TT.B"},
		{"d", "FilenameFormatting.TT.d"},
		{"H", "FilenameFormatting.TT.H"},
		{"I", "FilenameFormatting.TT.I"},
		{"m", "FilenameFormatting.TT.m"},
		{"M", "FilenameFormatting.TT.M"},
		{"p", "FilenameFormatting.TT.p"},
		{"s", "FilenameFormatting.TT.s"},
		{"S", "FilenameFormatting.TT.S"},
		{"y", "FilenameFormatting.TT.y"},
		{"Y", "FilenameFormatting.TT.Y"},
		{"z", "FilenameFormatting.TT.z"},
		{"Z", "FilenameFormatting.TT.Z"},
		{"FPS", "FilenameFormatting.TT.FPS"},
		{"CRES", "FilenameFormatting.TT.CRES"},
		{"ORES", "FilenameFormatting.TT.ORES"},
		{"VF", "FilenameFormatting.TT.VF"},
	};

	QString html = "<table>";

	for (auto f : format_list) {
		html += "<tr><th align='left'>%";
		html += f[0];
		html += "</th><td>";
		html += QTStr(f[1]);
		html += "</td></tr>";
	}

	html += "</table>";
	return html;
}


class SettingsEventFilter : public QObject {
	QScopedPointer<OBSEventFilter> shortcutFilter;

public:
	inline SettingsEventFilter()
		: shortcutFilter((OBSEventFilter*)CreateShortcutFilter())
	{
	}

protected:
	bool eventFilter(QObject* obj, QEvent* event) override
	{
		int key;

		switch(event->type()) {
			case QEvent::KeyPress:
			case QEvent::KeyRelease:
				key = static_cast<QKeyEvent*>(event)->key();
				if(key == Qt::Key_Escape) {
					return false;
				}
			default:
				break;
		}

		return shortcutFilter->filter(obj, event);
	}
};

AFQAdvancedSettingAreaWidget::AFQAdvancedSettingAreaWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFQAdvancedSettingAreaWidget)
{
    ui->setupUi(this);
}

AFQAdvancedSettingAreaWidget::~AFQAdvancedSettingAreaWidget()
{
	delete ui->lineEdit_FilenameFormatting->completer();

    delete ui;
}

static inline QString MakeMemorySizeString(int bitrate, int seconds)
{
	QString str = QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Advanced.StreamDelay.MemoryUsage"));
	int megabytes = bitrate * seconds / 1000 / 8;

	return str.arg(QString::number(megabytes));
}


void AFQAdvancedSettingAreaWidget::qslotAdvancedChanged()
{
	if (!m_bLoading)
	{
		m_bAdvancedSettingChanged = true;
		sender()->setProperty("changed", QVariant(true));
		emit qsignalAdvancedDataChanged();
	}
}

void AFQAdvancedSettingAreaWidget::qslotAdvancedChangedRestart()
{
	m_bRestartNeeded = true;
}

void AFQAdvancedSettingAreaWidget::qslotOutputReconnectEnableClicked()
{
	if (ui->checkBox_OutputReconnectEnable->isChecked()) {
		ui->label_OutputRetryDelay->setEnabled(true);
		ui->spinBox_ReconnectRetryDelay->setEnabled(true);
		ui->label_OutputMaxRetries->setEnabled(true);
		ui->spinBox_ReconnectMaxRetries->setEnabled(true);
	}
	else {
		ui->label_OutputRetryDelay->setEnabled(false);
		ui->spinBox_ReconnectRetryDelay->setEnabled(false);
		ui->label_OutputMaxRetries->setEnabled(false);
		ui->spinBox_ReconnectMaxRetries->setEnabled(false);
	}
}

void AFQAdvancedSettingAreaWidget::qslotFilenameFormattingTextEdited(const QString& text)
{
	QString safeStr = text;

#ifdef __APPLE__
	safeStr.replace(QRegularExpression("[:]"), "");
#elif defined(_WIN32)
	safeStr.replace(QRegularExpression("[<>:\"\\|\\?\\*]"), "");
#else
#endif

	if (text != safeStr)
		ui->lineEdit_FilenameFormatting->setText(safeStr);
}

void AFQAdvancedSettingAreaWidget::qslotResetAdvancedSettingUi()
{
	m_bRestartNeeded = false;
}

void AFQAdvancedSettingAreaWidget::AdvancedSettingAreaInit()
{
	connect(ui->checkBox_NetworkEnableNewSocketLoop, &QCheckBox::toggled,
		ui->checkBox_NetworkEnableLowLatencyMode, &QCheckBox::setEnabled);

	_SetAdvancedSettings();
	_ChangeLanguage();
	LoadAdvancedSettings();
	_SetAdvancedSettingSignal();
	m_bLoading = false;
}

void AFQAdvancedSettingAreaWidget::LoadAdvancedSettings()
{
	config_t* configGlobalFile = AFConfigManager::GetSingletonInstance().GetGlobal();
	config_t* configBasicFile = AFConfigManager::GetSingletonInstance().GetBasic();

#ifdef __APPLE__
#elif _WIN32
	const char* processPriority = config_get_string(configGlobalFile, "General", "ProcessPriority");
	int idx = ui->comboBox_ProcessPriority->findData(processPriority);
	if (idx == -1)
		idx = ui->comboBox_ProcessPriority->findData("Normal");
	ui->comboBox_ProcessPriority->setCurrentIndex(idx);

	bool enableNewSocketLoop = config_get_bool(configBasicFile, "Output", "NewSocketLoopEnable");
	ui->checkBox_NetworkEnableNewSocketLoop->setChecked(enableNewSocketLoop);
	ui->checkBox_NetworkEnableNewSocketLoop->toggled(enableNewSocketLoop);


	bool enableLowLatencyMode = config_get_bool(configBasicFile, "Output", "LowLatencyEnable");
	ui->checkBox_NetworkEnableLowLatencyMode->setChecked(enableLowLatencyMode);
#endif

	bool confirmOnExit = config_get_bool(configGlobalFile, "General", "ConfirmOnExit");
	ui->checkBox_ConfirmOnExit->setChecked(confirmOnExit);
	
	const char* filename = config_get_string(configBasicFile, "Output", "FilenameFormatting");
	bool overwriteIfExists = config_get_bool(configBasicFile, "Output", "OverwriteIfExists");
	bool autoRemux = config_get_bool(configBasicFile, "Video", "AutoRemux");
	const char* rbPrefix = config_get_string(configBasicFile, "SimpleOutput", "RecRBPrefix");
	const char* rbSuffix = config_get_string(configBasicFile, "SimpleOutput", "RecRBSuffix");

	QStringList specList = QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str("FilenameFormatting.completer")).split(QRegularExpression("\n"));
	QCompleter* specCompleter = new QCompleter(specList);
	specCompleter->setCaseSensitivity(Qt::CaseSensitive);
	specCompleter->setFilterMode(Qt::MatchContains);
	ui->lineEdit_FilenameFormatting->setCompleter(specCompleter);
	ui->lineEdit_FilenameFormatting->setToolTip(makeFormatToolTip());

	ui->lineEdit_FilenameFormatting->setText(filename);
	ui->checkBox_OverwriteIfExists->setChecked(overwriteIfExists);
	ui->checkBox_AutoRemux->setChecked(autoRemux);
	ui->lineEdit_SimpleRBPrefix->setText(rbPrefix);
	ui->lineEdit_SimpleRBSuffix->setText(rbSuffix);
	
	bool reconnect = config_get_bool(configBasicFile, "Output", "Reconnect");
	ui->checkBox_OutputReconnectEnable->setChecked(reconnect);
	qslotOutputReconnectEnableClicked();

	int retryDelay = config_get_int(configBasicFile, "Output", "RetryDelay");
	ui->spinBox_ReconnectRetryDelay->setValue(retryDelay);
	int maxRetries = config_get_int(configBasicFile, "Output", "MaxRetries");
	ui->spinBox_ReconnectMaxRetries->setValue(maxRetries);
	
	const char* ipFamily = config_get_string(configBasicFile, "Output", "IPFamily");
	const char* bindIP = config_get_string(configBasicFile, "Output", "BindIP");
	AFSettingUtils::SetComboByValue(ui->comboBox_NetworkIPFamily, ipFamily);
	if (!AFSettingUtils::SetComboByValue(ui->comboBox_NetworkBindToIP, bindIP))
		AFSettingUtils::SetInvalidValue(ui->comboBox_NetworkBindToIP, bindIP, bindIP);
	bool dynBitrate = config_get_bool(configBasicFile, "Output", "DynamicBitrate");
	ui->checkBox_OutputDynamicBitrateBeta->setChecked(dynBitrate);

#if defined(_WIN32) || defined(__APPLE__)
	bool browserHWAccel = config_get_bool(configGlobalFile, "General", "BrowserHWAccel");
	ui->checkBox_EnableHardwareAcceleration->setChecked(browserHWAccel);
	m_bBrowserAccelerationEnabled = ui->checkBox_EnableHardwareAcceleration->isChecked();
#endif

	ToggleOnStreaming(false);
}

void AFQAdvancedSettingAreaWidget::SaveAdvancedSettings()
{
#ifdef _WIN32
	std::string priority = QT_TO_UTF8(ui->comboBox_ProcessPriority->currentData().toString());
	config_set_string(AFConfigManager::GetSingletonInstance().GetGlobal(), "General", "ProcessPriority", priority.c_str());

	AFSettingUtils::SaveCheckBox(ui->checkBox_NetworkEnableNewSocketLoop, "Output", "NewSocketLoopEnable");
	AFSettingUtils::SaveCheckBox(ui->checkBox_NetworkEnableLowLatencyMode, "Output", "LowLatencyEnable");
#endif

	if (AFSettingUtils::WidgetChanged(ui->checkBox_ConfirmOnExit))
		config_set_bool(AFConfigManager::GetSingletonInstance().GetGlobal(), "General", "ConfirmOnExit",
						ui->checkBox_ConfirmOnExit->isChecked());

	AFSettingUtils::SaveEdit(ui->lineEdit_FilenameFormatting, "Output", "FilenameFormatting");
	AFSettingUtils::SaveCheckBox(ui->checkBox_OverwriteIfExists, "Output", "OverwriteIfExists");
	AFSettingUtils::SaveCheckBox(ui->checkBox_AutoRemux, "Video", "AutoRemux");
	AFSettingUtils::SaveEdit(ui->lineEdit_SimpleRBPrefix, "SimpleOutput", "RecRBPrefix");
	AFSettingUtils::SaveEdit(ui->lineEdit_SimpleRBSuffix, "SimpleOutput", "RecRBSuffix");

	AFSettingUtils::SaveCheckBox(ui->checkBox_OutputReconnectEnable, "Output", "Reconnect");
	AFSettingUtils::SaveSpinBox(ui->spinBox_ReconnectRetryDelay, "Output", "RetryDelay");
	AFSettingUtils::SaveSpinBox(ui->spinBox_ReconnectMaxRetries, "Output", "MaxRetries");
	AFSettingUtils::SaveComboData(ui->comboBox_NetworkBindToIP, "Output", "BindIP");
	AFSettingUtils::SaveComboData(ui->comboBox_NetworkIPFamily, "Output", "IPFamily");
	AFSettingUtils::SaveCheckBox(ui->checkBox_OutputDynamicBitrateBeta, "Output", "DynamicBitrate");

#if defined(_WIN32) || defined(__APPLE__)
	bool browserHWAccel = ui->checkBox_EnableHardwareAcceleration->isChecked();
	config_set_bool(AFConfigManager::GetSingletonInstance().GetGlobal(), "General", "BrowserHWAccel", browserHWAccel);
	m_bNeedRestartProgram = (ui->checkBox_EnableHardwareAcceleration && ui->checkBox_EnableHardwareAcceleration->isChecked() != m_bBrowserAccelerationEnabled);
#endif
}

void AFQAdvancedSettingAreaWidget::ToggleOnStreaming(bool streaming)
{
	bool useVideo = obs_video_active() ? false : true;

	ui->comboBox_NetworkIPFamily->setEnabled(useVideo);
	ui->comboBox_NetworkBindToIP->setEnabled(useVideo);
	ui->checkBox_OutputDynamicBitrateBeta->setEnabled(useVideo);

#ifdef _WIN32
	ui->checkBox_NetworkEnableNewSocketLoop->setEnabled(useVideo);
	ui->checkBox_NetworkEnableLowLatencyMode->setEnabled(useVideo);
#endif 
}

void AFQAdvancedSettingAreaWidget::SetAutoRemuxText(QString autoRemuxText)
{
	ui->checkBox_AutoRemux->setText(autoRemuxText);
}

void AFQAdvancedSettingAreaWidget::_SetAdvancedSettings()
{
	ui->widget_StreamDelay->hide();

#ifdef _WIN32
	static struct ProcessPriority {
		const char* name;
		const char* val;
	} processPriorities[] = {
		{"Basic.Settings.Advanced.General.ProcessPriority.High",
		 "High"},
		{"Basic.Settings.Advanced.General.ProcessPriority.AboveNormal",
		 "AboveNormal"},
		{"Basic.Settings.Advanced.General.ProcessPriority.Normal",
		 "Normal"},
		{"Basic.Settings.Advanced.General.ProcessPriority.BelowNormal",
		 "BelowNormal"},
		{"Basic.Settings.Advanced.General.ProcessPriority.Idle",
		 "Idle"},
	};

	for (ProcessPriority pri : processPriorities)
	{
		const char* translate = AFLocaleTextManager::GetSingletonInstance().Str(pri.name);
		QString qtranslate = QString::fromUtf8(translate);
		ui->comboBox_ProcessPriority->addItem(qtranslate, pri.val);
	}

#else
	ui->label_ProcessPriority->close();
	delete ui->label_ProcessPriority;
	ui->comboBox_ProcessPriority->close();
	delete ui->comboBox_ProcessPriority;
	ui->checkBox_NetworkEnableNewSocketLoop->close();
	delete ui->checkBox_NetworkEnableNewSocketLoop;
	ui->checkBox_NetworkEnableLowLatencyMode->close();
	delete ui->checkBox_NetworkEnableLowLatencyMode;
#ifdef __linux__
	ui->widget_Sources->close();
	delete ui->widget_Sources;
#endif

#endif
	installEventFilter(new SettingsEventFilter());

	obs_properties_t* ppts = obs_get_output_properties("rtmp_output");
	obs_property_t* p = obs_properties_get(ppts, "bind_ip");

	size_t count = obs_property_list_item_count(p);
	for (size_t i = 0; i < count; i++) {
		const char* name = obs_property_list_item_name(p, i);
		const char* val = obs_property_list_item_string(p, i);

		ui->comboBox_NetworkBindToIP->addItem(QT_UTF8(name), val);
	}

	p = obs_properties_get(ppts, "ip_family");

	count = obs_property_list_item_count(p);
	for (size_t i = 0; i < count; i++) {
		const char* name = obs_property_list_item_name(p, i);
		const char* val = obs_property_list_item_string(p, i);

		ui->comboBox_NetworkIPFamily->addItem(QT_UTF8(name), val);
	}

	obs_properties_destroy(ppts);
}

void AFQAdvancedSettingAreaWidget::_SetAdvancedSettingSignal()
{
#if defined(_WIN32)
	AFSettingUtils::HookWidget(ui->comboBox_ProcessPriority, this, COMBO_CHANGED, ADV_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_NetworkEnableNewSocketLoop, this, CHECK_CHANGED, ADV_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_NetworkEnableLowLatencyMode, this, CHECK_CHANGED, ADV_CHANGED);
#endif
	AFSettingUtils::HookWidget(ui->checkBox_ConfirmOnExit, this, CHECK_CHANGED, ADV_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_OutputReconnectEnable, this, CHECK_CHANGED, ADV_CHANGED);
	AFSettingUtils::HookWidget(ui->spinBox_ReconnectRetryDelay, this, SCROLL_CHANGED, ADV_CHANGED);
	AFSettingUtils::HookWidget(ui->spinBox_ReconnectMaxRetries, this, SCROLL_CHANGED, ADV_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_NetworkIPFamily, this, COMBO_CHANGED, ADV_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_NetworkBindToIP, this, COMBO_CHANGED, ADV_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_OutputDynamicBitrateBeta, this, CHECK_CHANGED, ADV_CHANGED);
	AFSettingUtils::HookWidget(ui->lineEdit_FilenameFormatting, this, EDIT_CHANGED, ADV_CHANGED);
	AFSettingUtils::HookWidget(ui->lineEdit_SimpleRBPrefix, this, EDIT_CHANGED, ADV_CHANGED);
	AFSettingUtils::HookWidget(ui->lineEdit_SimpleRBSuffix, this, EDIT_CHANGED, ADV_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_OverwriteIfExists, this, CHECK_CHANGED, ADV_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AutoRemux, this, CHECK_CHANGED, ADV_CHANGED);

#if defined(_WIN32) || defined(__APPLE__)
	AFSettingUtils::HookWidget(ui->checkBox_EnableHardwareAcceleration, this, CHECK_CHANGED, ADV_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_EnableHardwareAcceleration, this, CHECK_CHANGED, ADV_RESTART);
#endif

	connect(ui->checkBox_OutputReconnectEnable, &QCheckBox::toggled, this, &AFQAdvancedSettingAreaWidget::qslotOutputReconnectEnableClicked);
	connect(ui->lineEdit_FilenameFormatting, &QLineEdit::textChanged, this, &AFQAdvancedSettingAreaWidget::qslotFilenameFormattingTextEdited);
}

void AFQAdvancedSettingAreaWidget::_ChangeLanguage()
{
	QList<QLabel*> labelList = findChildren<QLabel*>();
	QList<QCheckBox*> checkboxList = findChildren<QCheckBox*>();

	foreach(QLabel * label, labelList)
	{
		label->setText(QT_UTF8(
			AFLocaleTextManager::GetSingletonInstance().Str(label->text().toUtf8().constData())));
	}

	foreach(QCheckBox* checkbox, checkboxList)
	{
		checkbox->setText(QT_UTF8(
			AFLocaleTextManager::GetSingletonInstance().Str(checkbox->text().toUtf8().constData())));
	}
}

void AFQAdvancedSettingAreaWidget::_UpdateAdvNetworkGroup()
{
}
