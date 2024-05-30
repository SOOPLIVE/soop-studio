#include "COutputSettingAreaWidget.h"

#include <util/dstr.hpp>
#include <sstream>
#include <string>
#include <QCompleter>

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "UIComponent/CMessageBox.h"
#include "../Common/SettingsMiscDef.h"
#include "../CoreModel/Output/COutput.inl"

using namespace std;


enum class ListOpt : int {
	ShowAll = 1,
	Custom,
	WHIP,
};

inline bool AFQOutputSettingAreaWidget::_IsCustomService() const
{
	return 1 == (int)ListOpt::Custom;
}

static inline const char* RecTypeFromIdx(int idx)
{
	if (idx == 1)
		return "FFmpeg";
	else
		return "Standard";
}

static inline const char* OutputSetModeFromBool(bool mode)
{
	if (mode)
		return "Advanced";
	else
		return "Simple";
}

static inline const char* SplitFileTypeFromIdx(int idx)
{
	if (idx == 1)
		return "Time";
	else if (idx == 2)
		return "Size";
	else
		return "Manual";
}

static void WriteJsonData(AFQPropertiesView* view, const char* path)
{
	char full_path[512];

	if (!view || !AFSettingUtilsA::WidgetChanged(view))
		return;

	int ret = AFConfigManager::GetSingletonInstance().GetProfilePath(full_path, sizeof(full_path), path);
	if (ret > 0) {
		obs_data_t* settings = view->GetSettings();
		if (settings) {
			obs_data_save_json_safe(settings, full_path, "tmp",
				"bak");
		}
	}
}

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

static void DisableIncompatibleSimpleContainer(QComboBox* cbox,
	const QString& currentFormat,
	const QString& vEncoder,
	const QString& aEncoder)
{
	string vCodec = obs_get_encoder_codec(
		get_simple_output_encoder(QT_TO_UTF8(vEncoder)));
	string aCodec = aEncoder.toStdString();

	bool currentCompatible = true;
	for (int idx = 0; idx < cbox->count(); idx++) {
		QString format = cbox->itemData(idx).toString();
		string formatStr = format.toStdString();

		QStandardItemModel* model =
			dynamic_cast<QStandardItemModel*>(cbox->model());
		QStandardItem* item = model->item(idx);

		if (ContainerSupportsCodec(formatStr, vCodec) &&
			ContainerSupportsCodec(formatStr, aCodec)) {
			item->setFlags(Qt::ItemIsSelectable |
				Qt::ItemIsEnabled);
		}
		else {
			if (format == currentFormat)
				currentCompatible = false;

			item->setFlags(Qt::NoItemFlags);
		}
	}

	if (!currentCompatible)
		cbox->setCurrentIndex(-1);
}

static void DisableIncompatibleSimpleCodecs(QComboBox* cbox,
	const QString& format)
{
	QString encoder = cbox->currentData().toString();

	bool currentCompatible = true;
	for (int idx = 0; idx < cbox->count(); idx++) {
		QString encName = cbox->itemData(idx).toString();
		QString codec;

		if (encName == "opus" || encName == "aac") {
			codec = encName;
		}
		else {
			const char* encoder_id =
				get_simple_output_encoder(QT_TO_UTF8(encName));
			codec = obs_get_encoder_codec(encoder_id);
		}

		QStandardItemModel* model =
			dynamic_cast<QStandardItemModel*>(cbox->model());
		QStandardItem* item = model->item(idx);

		if (ContainerSupportsCodec(format.toStdString(),
			codec.toStdString())) {
			item->setFlags(Qt::ItemIsSelectable |
				Qt::ItemIsEnabled);
		}
		else {
			if (encoder == encName)
				currentCompatible = false;

			item->setFlags(Qt::NoItemFlags);
		}
	}

	if (!currentCompatible)
		cbox->setCurrentIndex(-1);
}

AFQOutputSettingAreaWidget::AFQOutputSettingAreaWidget(QWidget* parent):
	QWidget(parent), ui(new Ui::AFQOutputSettingAreaWidget)
{
	ui->setupUi(this);

	_FillSimpleRecordingValues();
	_EventHookConnect();
	_ChangeLanguage();
}

AFQOutputSettingAreaWidget::~AFQOutputSettingAreaWidget()
{
	delete ui;
}

void AFQOutputSettingAreaWidget::LoadOutputSettings(bool reset) {
	m_bLoading = true;
	if (!service) {
		service =
			obs_service_create("rtmp_common", NULL, NULL, nullptr);
		obs_service_release(service);
	}
	protocol = QT_UTF8(obs_service_get_protocol(service));

	_ResetEncoders(false);
	auto basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();

	_LoadFormats();
	LoadSimpleOutputSettings();
	LoadOutputStandardSettings(reset);
	_LoadAdvancedOutputFFmpegSettings();
	_LoadAdvReplayBuffer();
	_LoadOutputRecordingEncoderProperties();

	qslotSimpleRecordingQualityChanged();
	qslotAdvOutSplitFileChanged();
	qslotAdvOutRecCheckCodecs();
	qslotAdvOutRecCheckWarnings();
	qslotUpdateAutomaticReplayBufferCheckboxes();
	qslotAdvOutFFFormatCurrentIndexChanged(ui->comboBox_AdvOutFFFormat->currentIndex());

	uint32_t cx = config_get_uint(basicConfig, "Video", "BaseCX");
	uint32_t cy = config_get_uint(basicConfig, "Video", "BaseCY");
	uint32_t out_cx = config_get_uint(basicConfig, "Video", "OutputCX");
	uint32_t out_cy = config_get_uint(basicConfig, "Video", "OutputCY");
	string outputResString = AFSettingUtilsA::ResString(out_cx, out_cy);
	ResetDownscales(cx, cy);

	qslotOutputTypeChanged(ui->comboBox_AdvOutRecType->currentIndex());
	qslotAdvOutSplitFileChanged();
	qslotAdvOutFFTypeCurrentIndexChanged(ui->comboBox_AdvOutFFType->currentIndex());

	ToggleOnStreaming(false);
	_SetSettingModeUi(m_bIsAdvancedMode);

	m_bLoading = false;
}

void AFQOutputSettingAreaWidget::LoadSimpleOutputSettings()
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();
	const char* path = config_get_string(basicConfig, "SimpleOutput", "FilePath");
	bool noSpace = config_get_bool(basicConfig, "SimpleOutput", "FileNameWithoutSpace");
	const char* recQual = config_get_string(basicConfig, "SimpleOutput", "RecQuality");
	const char* format = config_get_string(basicConfig, "SimpleOutput", "RecFormat2");
	const char* recEnc = config_get_string(basicConfig, "SimpleOutput", "RecEncoder");
	const char* recAudioEnc = config_get_string(basicConfig, "SimpleOutput", "RecAudioEncoder");
	const char* muxCustom = config_get_string(basicConfig, "SimpleOutput", "MuxerCustom");
	bool replayBuf = config_get_bool(basicConfig, "SimpleOutput", "RecRB");
	int rbTime = config_get_int(basicConfig, "SimpleOutput", "RecRBTime");
	int rbSize = config_get_int(basicConfig, "SimpleOutput", "RecRBSize");
	int tracks = config_get_int(basicConfig, "SimpleOutput", "RecTracks");

	ui->lineEdit_SimpleOutputPath->setText(path);
	ui->checkBox_SimpleNoSpace->setChecked(noSpace);
	
	int idx = ui->comboBox_SimpleOutRecQuality->findData(QString(recQual));
	if (idx == -1)
		idx = 0;
	ui->comboBox_SimpleOutRecQuality->setCurrentIndex(idx);

	idx = ui->comboBox_SimpleOutRecFormat->findData(format);
	ui->comboBox_SimpleOutRecFormat->setCurrentIndex(idx);

	idx = ui->comboBox_SimpleOutRecEncoder->findData(QString(recEnc));
	ui->comboBox_SimpleOutRecEncoder->setCurrentIndex(idx);

	idx = ui->comboBox_SimpleOutRecAEncoder->findData(QString(recAudioEnc));
	ui->comboBox_SimpleOutRecAEncoder->setCurrentIndex(idx);

	ui->lineEdit_SimpleOutMuxCustom->setText(muxCustom);

	ui->checkBox_SimpleReplayBuf->setChecked(replayBuf);
	ui->spinBox_SimpleRBSecMax->setValue(rbTime);
	ui->spinBox_SimpleRBMegsMax->setValue(rbSize);

	ui->checkBox_SimpleOutRecTrack1->setChecked(tracks & (1 << 0));
	ui->checkBox_SimpleOutRecTrack2->setChecked(tracks & (1 << 1));
	ui->checkBox_SimpleOutRecTrack3->setChecked(tracks & (1 << 2));
	ui->checkBox_SimpleOutRecTrack4->setChecked(tracks & (1 << 3));
	ui->checkBox_SimpleOutRecTrack5->setChecked(tracks & (1 << 4));
	ui->checkBox_SimpleOutRecTrack6->setChecked(tracks & (1 << 5));
}

void AFQOutputSettingAreaWidget::_EventHookConnect() {
	AFSettingUtilsA::HookWidget(ui->comboBox_SimpleOutRecQuality, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->comboBox_SimpleOutRecFormat, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->comboBox_SimpleOutRecEncoder, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->comboBox_SimpleOutRecAEncoder, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_SimpleNoSpace, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_SimpleOutRecTrack1, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_SimpleOutRecTrack2, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_SimpleOutRecTrack3, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_SimpleOutRecTrack4, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_SimpleOutRecTrack5, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_SimpleOutRecTrack6, this, CHECK_CHANGED, OUTPUTS_CHANGED);

	AFSettingUtilsA::HookWidget(ui->comboBox_AdvOutRecFormat, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->comboBox_AdvOutRecType, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->comboBox_AdvOutRecEncoder, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->comboBox_AdvOutRecAEncoder, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->comboBox_AdvOutRecRescale, this, CBEDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->comboBox_AdvOutSplitFileType, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->comboBox_AdvOutFFType, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->comboBox_AdvOutFFFormat, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->comboBox_AdvOutFFRescale, this, CBEDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->comboBox_AdvOutFFVEncoder, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->comboBox_AdvOutFFAEncoder, this, COMBO_CHANGED, OUTPUTS_CHANGED);

	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutNoSpace, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutRecTrack1, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutRecTrack2, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutRecTrack3, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutRecTrack4, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutRecTrack5, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutRecTrack6, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutFFTrack1, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutFFTrack2, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutFFTrack3, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutFFTrack4, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutFFTrack5, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutFFTrack6, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutSplitFile, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutFFNoSpace, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutFFIgnoreCompat, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutFFUseRescale, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_AdvOutRecUseRescale, this, CHECK_CHANGED, OUTPUTS_CHANGED);

	AFSettingUtilsA::HookWidget(ui->radioButton_FlvTrack1, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->radioButton_FlvTrack2, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->radioButton_FlvTrack3, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->radioButton_FlvTrack4, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->radioButton_FlvTrack5, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->radioButton_FlvTrack6, this, CHECK_CHANGED, OUTPUTS_CHANGED);

	AFSettingUtilsA::HookWidget(ui->lineEdit_SimpleOutputPath, this, EDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->lineEdit_SimpleOutMuxCustom, this, EDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->checkBox_SimpleReplayBuf, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->lineEdit_AdvOutRecPath, this, EDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->lineEdit_AdvOutMuxCustom, this, EDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->lineEdit_AdvOutFFRecPath, this, EDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->lineEdit_AdvOutFFURL, this, EDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->lineEdit_AdvOutFFMCfg, this, EDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->lineEdit_AdvOutFFVCfg, this, EDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->lineEdit_AdvOutFFACfg, this, EDIT_CHANGED, OUTPUTS_CHANGED);

	AFSettingUtilsA::HookWidget(ui->spinBox_SimpleRBSecMax, this, SCROLL_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->spinBox_SimpleRBMegsMax, this, SCROLL_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->spinBox_AdvOutSplitFileTime, this, SCROLL_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->spinBox_AdvOutSplitFileSize, this, SCROLL_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->spinBox_AdvOutFFVBitrate, this, SCROLL_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->spinBox_AdvOutFFVGOPSize, this, SCROLL_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->spinBox_AdvOutFFABitrate, this, SCROLL_CHANGED, OUTPUTS_CHANGED);

	AFSettingUtilsA::HookWidget(ui->checkBox_AdvReplayBuf, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->spinBox_AdvRBSecMax, this, SCROLL_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtilsA::HookWidget(ui->spinBox_AdvRBMegsMax, this, SCROLL_CHANGED, OUTPUTS_CHANGED);

	connect(ui->pushButton_SimpleOutputBrowse, &QPushButton::clicked, this, &AFQOutputSettingAreaWidget::qslotSimpleOutputBrowseClicked);
	connect(ui->comboBox_SimpleOutRecQuality, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotSimpleRecordingQualityChanged);
	connect(ui->comboBox_SimpleOutRecQuality, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotSimpleRecordingQualityLosslessWarning);
	connect(ui->comboBox_SimpleOutRecFormat, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotSimpleRecordingEncoderChanged);
	connect(ui->comboBox_SimpleOutRecEncoder, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotSimpleRecordingEncoderChanged);
	connect(ui->comboBox_SimpleOutRecAEncoder, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotSimpleRecordingEncoderChanged);
	connect(ui->checkBox_SimpleReplayBuf, &QCheckBox::toggled, this, &AFQOutputSettingAreaWidget::qslotSimpleReplayBufferChanged);
	connect(ui->spinBox_SimpleRBSecMax, &QSpinBox::valueChanged, this, &AFQOutputSettingAreaWidget::qslotSimpleReplayBufferChanged);

	connect(ui->pushButton_AdvOutRecPathBrowse, &QPushButton::clicked, this, &AFQOutputSettingAreaWidget::qslotAdvOutRecPathBrowseClicked);
	connect(ui->pushButton_AdvOutFFPathBrowse, &QPushButton::clicked, this, &AFQOutputSettingAreaWidget::qslotAdvOutFFPathBrowseClicked);
	connect(ui->comboBox_AdvOutRecFormat, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotAdvOutRecCheckCodecs);
	connect(ui->comboBox_AdvOutFFFormat, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotAdvOutFFFormatCurrentIndexChanged);
	connect(ui->comboBox_AdvOutFFAEncoder, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotAdvOutFFAEncoderCurrentIndexChanged);
	connect(ui->comboBox_AdvOutFFVEncoder, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotAdvOutFFVEncoderCurrentIndexChanged);
	connect(ui->checkBox_AdvOutFFIgnoreCompat, &QCheckBox::stateChanged, this, &AFQOutputSettingAreaWidget::qslotAdvOutFFIgnoreCompatStateChanged);
	connect(ui->comboBox_AdvOutRecEncoder, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotAdvOutRecEncoderCurrentIndexChanged);
	connect(ui->comboBox_AdvOutRecType, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotOutputTypeChanged);
	connect(ui->comboBox_AdvOutFFType, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotAdvOutFFTypeCurrentIndexChanged);
	
	connect(ui->widget_Simple, &AFQHoverWidget::qsignalMouseClick, this, &AFQOutputSettingAreaWidget::qslotChangeSettingModeToSimple);
	connect(ui->widget_Advanced, &AFQHoverWidget::qsignalMouseClick, this, &AFQOutputSettingAreaWidget::qslotChangeSettingModeToAdvanced);

	connect(ui->checkBox_AdvOutSplitFile, &QCheckBox::stateChanged, this, &AFQOutputSettingAreaWidget::qslotAdvOutSplitFileChanged);
	connect(ui->comboBox_AdvOutSplitFileType, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotAdvOutSplitFileChanged);
	connect(ui->checkBox_AdvOutSplitFile, &QCheckBox::toggled, ui->comboBox_AdvOutSplitFileType, &QComboBox::setEnabled);
	
	connect(ui->checkBox_AdvOutRecTrack1, &QCheckBox::clicked, this, &AFQOutputSettingAreaWidget::qslotAdvOutRecCheckWarnings);
	connect(ui->checkBox_AdvOutRecTrack2, &QCheckBox::clicked, this, &AFQOutputSettingAreaWidget::qslotAdvOutRecCheckWarnings);
	connect(ui->checkBox_AdvOutRecTrack3, &QCheckBox::clicked, this, &AFQOutputSettingAreaWidget::qslotAdvOutRecCheckWarnings);
	connect(ui->checkBox_AdvOutRecTrack4, &QCheckBox::clicked, this, &AFQOutputSettingAreaWidget::qslotAdvOutRecCheckWarnings);
	connect(ui->checkBox_AdvOutRecTrack5, &QCheckBox::clicked, this, &AFQOutputSettingAreaWidget::qslotAdvOutRecCheckWarnings);
	connect(ui->checkBox_AdvOutRecTrack6, &QCheckBox::clicked, this, &AFQOutputSettingAreaWidget::qslotAdvOutRecCheckWarnings);
	connect(ui->comboBox_AdvOutRecFormat, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotAdvOutRecCheckWarnings);
	connect(ui->comboBox_AdvOutRecEncoder, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotAdvOutRecCheckWarnings);
	connect(ui->checkBox_AdvOutRecTrack1, &QCheckBox::toggled, this, &AFQOutputSettingAreaWidget::qslotAdvReplayBufferChanged);
	connect(ui->checkBox_AdvOutRecTrack2, &QCheckBox::toggled, this, &AFQOutputSettingAreaWidget::qslotAdvReplayBufferChanged);
	connect(ui->checkBox_AdvOutRecTrack3, &QCheckBox::toggled, this, &AFQOutputSettingAreaWidget::qslotAdvReplayBufferChanged);
	connect(ui->checkBox_AdvOutRecTrack4, &QCheckBox::toggled, this, &AFQOutputSettingAreaWidget::qslotAdvReplayBufferChanged);
	connect(ui->checkBox_AdvOutRecTrack5, &QCheckBox::toggled, this, &AFQOutputSettingAreaWidget::qslotAdvReplayBufferChanged);
	connect(ui->checkBox_AdvOutRecTrack6, &QCheckBox::toggled, this, &AFQOutputSettingAreaWidget::qslotAdvReplayBufferChanged);
	connect(ui->checkBox_AdvOutFFUseRescale, &QCheckBox::toggled, ui->comboBox_AdvOutFFRescale, &QComboBox::setEnabled);
	connect(ui->checkBox_AdvOutRecUseRescale, &QCheckBox::toggled, ui->comboBox_AdvOutRecRescale, &QComboBox::setEnabled);

	connect(ui->comboBox_AdvOutRecType, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotAdvReplayBufferChanged);
	connect(ui->comboBox_AdvOutRecEncoder, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotAdvReplayBufferChanged);
	connect(ui->checkBox_AdvReplayBuf, &QCheckBox::toggled, this, &AFQOutputSettingAreaWidget::qslotAdvReplayBufferChanged);
	connect(ui->spinBox_AdvRBSecMax, &QSpinBox::valueChanged, this, &AFQOutputSettingAreaWidget::qslotAdvReplayBufferChanged);

	ui->comboBox_AdvOutRecEncoder->setPlaceholderText(QTStr("CodecCompat.CodecPlaceholder"));
	ui->comboBox_AdvOutRecAEncoder->setPlaceholderText(QTStr("CodecCompat.CodecPlaceholder"));
	ui->comboBox_SimpleOutRecEncoder->setPlaceholderText(QTStr("CodecCompat.CodecPlaceholder"));
	ui->comboBox_SimpleOutRecAEncoder->setPlaceholderText(QTStr("CodecCompat.CodecPlaceholder"));
	ui->comboBox_SimpleOutRecFormat->setPlaceholderText(QTStr("CodecCompat.ContainerPlaceholder"));

	ui->spinBox_AdvOutFFVBitrate->setSingleStep(50);
	ui->spinBox_AdvOutFFVBitrate->setSuffix(" Kbps");
	ui->spinBox_AdvOutFFABitrate->setSuffix(" Kbps");

	QRegularExpression rx("\\d{1,5}x\\d{1,5}");
	QValidator* validator = new QRegularExpressionValidator(rx, this);
	ui->comboBox_AdvOutRecRescale->lineEdit()->setValidator(validator);
	ui->comboBox_AdvOutFFRescale->lineEdit()->setValidator(validator);
}

void AFQOutputSettingAreaWidget::_SetSettingModeUi(const bool isAdvanced)
{
	if (isAdvanced)
	{
		ui->widget_Simple->setProperty("modeActive", false);
		ui->widget_Advanced->setProperty("modeActive", true);
		ui->label_Simple->setProperty("modeActive", false);
		ui->label_Advanced->setProperty("modeActive", true);

		ui->widget_SimpleOutputSetting->setVisible(false);
		ui->widget_AdvOutputSetting->setVisible(true);
		
		if (ui->comboBox_AdvOutRecType->currentIndex() == 0)
		{
			ui->widget_AdvRecStandardSetting->setVisible(true);
			ui->widget_AdvRecFFmpegSetting->setVisible(false);
		}
		else
		{
			ui->widget_AdvRecStandardSetting->setVisible(false);
			ui->widget_AdvRecFFmpegSetting->setVisible(true);
		}
	}
	else
	{
		ui->widget_Simple->setProperty("modeActive", true);
		ui->widget_Advanced->setProperty("modeActive", false);
		ui->label_Simple->setProperty("modeActive", true);
		ui->label_Advanced->setProperty("modeActive", false);

		ui->widget_SimpleOutputSetting->setVisible(true);
		ui->widget_AdvOutputSetting->setVisible(false);
	}

	style()->unpolish(ui->widget_Simple);
	style()->unpolish(ui->widget_Advanced);
	style()->unpolish(ui->label_Simple);
	style()->unpolish(ui->label_Advanced);

	style()->polish(ui->widget_Simple);
	style()->polish(ui->widget_Advanced);
	style()->polish(ui->label_Simple);
	style()->polish(ui->label_Advanced);
}

void AFQOutputSettingAreaWidget::qslotChangeSettingModeToSimple() 
{
	ChangeSettingModeToSimple();
	qslotUpdateStreamDelayEstimate();

	emit qsignalSimpleModeClicked();
}

void AFQOutputSettingAreaWidget::qslotChangeSettingModeToAdvanced()
{
	ChangeSettingModeToAdvanced();
	qslotUpdateStreamDelayEstimate();

	emit qsignalAdvancedModeClicked();
}

void AFQOutputSettingAreaWidget::qslotSimpleRecordingEncoderChanged()
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();

	QString qual = ui->comboBox_SimpleOutRecQuality->currentData().toString();
	bool enforceBitrate = !config_get_bool(basicConfig, "Stream1", "IgnoreRecommended");
	QString format = ui->comboBox_SimpleOutRecFormat->currentData().toString();
	QString tooltip =
		QTStr("Basic.Settings.Output.Format.TT." + format.toUtf8());

	if (!tooltip.startsWith("Basic.Settings.Output"))
		ui->comboBox_SimpleOutRecFormat->setToolTip(tooltip);
	else
		ui->comboBox_SimpleOutRecFormat->setToolTip(nullptr);

	if (qual == "Lossless")
	{
	}
	else if (qual != "Stream") {
		QString enc = ui->comboBox_SimpleOutRecEncoder->currentData().toString();
		QString streamEnc = m_strSimpleOutVEncoder;
		bool x264RecEnc = (enc == SIMPLE_ENCODER_X264 ||
			enc == SIMPLE_ENCODER_X264_LOWCPU);

		ui->comboBox_SimpleOutRecEncoder->blockSignals(true);
		ui->comboBox_SimpleOutRecAEncoder->blockSignals(true);
		DisableIncompatibleSimpleCodecs(ui->comboBox_SimpleOutRecEncoder,
			format);
		DisableIncompatibleSimpleCodecs(ui->comboBox_SimpleOutRecAEncoder,
			format);
		ui->comboBox_SimpleOutRecAEncoder->blockSignals(false);
		ui->comboBox_SimpleOutRecEncoder->blockSignals(false);
	}
	else {
		QString streamEnc = m_strSimpleOutVEncoder;
		QString streamAEnc = m_strSimpleOutAEncoder;

		if (streamEnc == "" || streamAEnc == "")
			return;

		ui->comboBox_SimpleOutRecFormat->blockSignals(true);
		DisableIncompatibleSimpleContainer(
			ui->comboBox_SimpleOutRecFormat, format, streamEnc, streamAEnc);
		ui->comboBox_SimpleOutRecFormat->blockSignals(false);
	}

	if (qual != "Lossless" && (format == "mp4" || format == "mov")) {
		emit qsignalSetAutoRemuxText(QTStr("Basic.Settings.Advanced.AutoRemux").arg("mp4") +
			" " + QTStr("Basic.Settings.Advanced.AutoRemux.MP4"));
	}
	else {
		emit qsignalSetAutoRemuxText(QTStr("Basic.Settings.Advanced.AutoRemux").arg("mp4"));
	}

	if (qual == "Stream") {
		ui->stackedWidget_SimpleRecTrack->setCurrentWidget(ui->page_SimpleFlvTracks);
	}
	else if (qual == "Lossless") {
		ui->stackedWidget_SimpleRecTrack->setCurrentWidget(ui->page_SimpleRecTracks);
	}
	else {
		if (format == "flv") {
			ui->stackedWidget_SimpleRecTrack->setCurrentWidget(
				ui->page_SimpleFlvTracks);
		}
		else {
			ui->stackedWidget_SimpleRecTrack->setCurrentWidget(
				ui->page_SimpleRecTracks);
		}
	}

	if (ui->stackedWidget_SimpleRecTrack->currentWidget() == ui->page_SimpleRecTracks)
	{
		ui->stackedWidget_SimpleRecTrack->setVisible(true);
		ui->label_SimpleRecTracks->setVisible(true);
	}
	else
	{
		ui->stackedWidget_SimpleRecTrack->setVisible(false);
		ui->label_SimpleRecTracks->setVisible(false);
	}
}

#define SIMPLE_OUTPUT_WARNING(str) \
	QTStr("Basic.Settings.Output.Simple.Warn." str)

void AFQOutputSettingAreaWidget::qslotSimpleRecordingQualityLosslessWarning(int idx)
{
	if (idx == m_nLastSimpleRecQualityIdx || idx == -1)
		return;

	QString qual = ui->comboBox_SimpleOutRecQuality->itemData(idx).toString();

	if (m_bLoading) {
		m_nLastSimpleRecQualityIdx = idx;
		return;
	}

	if (qual == "Lossless") {
		QString warningString = SIMPLE_OUTPUT_WARNING("Lossless") +
			QString("\n\n") +
			SIMPLE_OUTPUT_WARNING("Lossless.Msg");

		int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
			this, "",
			warningString);

		if (result == QDialog::Rejected) {
			QMetaObject::invokeMethod(
				ui->comboBox_SimpleOutRecQuality, "setCurrentIndex",
				Qt::QueuedConnection,
				Q_ARG(int, m_nLastSimpleRecQualityIdx));
			return;
		}
	}

	m_nLastSimpleRecQualityIdx = idx;
}

void AFQOutputSettingAreaWidget::qslotUpdateStreamDelayEstimate()
{
	qslotUpdateAutomaticReplayBufferCheckboxes();
}

void AFQOutputSettingAreaWidget::qslotUpdateAutomaticReplayBufferCheckboxes()
{
	bool state = false;

	if (!m_bIsAdvancedMode)
	{
		state = ui->checkBox_SimpleReplayBuf->isChecked();
		ui->checkBox_SimpleReplayBuf->setEnabled(
			!obs_frontend_replay_buffer_active());
	}
	else
	{
		state = ui->checkBox_AdvReplayBuf->isChecked();
		bool customFFmpeg = ui->comboBox_AdvOutRecType->currentIndex() == 1;
		bool replayBufEnabled = !obs_frontend_replay_buffer_active() && !customFFmpeg;
		ui->checkBox_AdvReplayBuf->setEnabled(replayBufEnabled);
		ui->label_AdvReplayBufCustomFFmpeg->setVisible(customFFmpeg);
	}

	emit qsignalUpdateReplayBufferStream(state);
}

void AFQOutputSettingAreaWidget::LoadOutputStandardSettings(bool reset)
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();
	
	const char* mode = config_get_string(basicConfig, "Output", "Mode");
	if (astrcmpi(mode, "Advanced") == 0)
		m_bIsAdvancedMode = true;
	else
		m_bIsAdvancedMode = false;
	
	const char* type = config_get_string(basicConfig, "AdvOut", "RecType");
	const char* recencoderType = config_get_string(basicConfig, "AdvOut", "RecAudioEncoder");
	const char* format = config_get_string(basicConfig, "AdvOut", "RecFormat2");
	const char* path = config_get_string(basicConfig, "AdvOut", "RecFilePath");
	bool noSpace = config_get_bool(basicConfig, "AdvOut", "RecFileNameWithoutSpace");
	bool rescale = config_get_bool(basicConfig, "AdvOut", "RecRescale");
	const char* rescaleRes = config_get_string(basicConfig, "AdvOut", "RecRescaleRes");
	const char* muxCustom = config_get_string(basicConfig, "AdvOut", "RecMuxerCustom");
	int tracks = config_get_int(basicConfig, "AdvOut", "RecTracks");
	int flvTrack = config_get_int(basicConfig, "AdvOut", "FLVTrack");
	bool splitFile = config_get_bool(basicConfig, "AdvOut", "RecSplitFile");
	const char* splitFileType = config_get_string(basicConfig, "AdvOut", "RecSplitFileType");
	int splitFileTime = config_get_int(basicConfig, "AdvOut", "RecSplitFileTime");
	int splitFileSize = config_get_int(basicConfig, "AdvOut", "RecSplitFileSize");
	int typeIndex = (astrcmpi(type, "FFmpeg") == 0) ? 1 : 0;
	
	ui->comboBox_AdvOutRecType->setCurrentIndex(typeIndex);
	ui->lineEdit_AdvOutRecPath->setText(path);
	ui->checkBox_AdvOutNoSpace->setChecked(noSpace);

	ui->checkBox_AdvOutRecUseRescale->setChecked(rescale);
	ui->comboBox_AdvOutRecRescale->setEnabled(rescale);
	ui->comboBox_AdvOutRecRescale->setCurrentText(rescaleRes);
	ui->lineEdit_AdvOutMuxCustom->setText(muxCustom);
	
	if (!AFSettingUtilsA::SetComboByValue(ui->comboBox_AdvOutRecAEncoder, recencoderType))
		ui->comboBox_AdvOutRecAEncoder->setCurrentIndex(-1);
	
	int idx = ui->comboBox_AdvOutRecFormat->findData(format);
	ui->comboBox_AdvOutRecFormat->setCurrentIndex(idx);

	ui->checkBox_AdvOutRecTrack1->setChecked(tracks & (1 << 0));
	ui->checkBox_AdvOutRecTrack2->setChecked(tracks & (1 << 1));
	ui->checkBox_AdvOutRecTrack3->setChecked(tracks & (1 << 2));
	ui->checkBox_AdvOutRecTrack4->setChecked(tracks & (1 << 3));
	ui->checkBox_AdvOutRecTrack5->setChecked(tracks & (1 << 4));
	ui->checkBox_AdvOutRecTrack6->setChecked(tracks & (1 << 5));

	if (astrcmpi(splitFileType, "Time") == 0)
		idx = 1;
	else if (astrcmpi(splitFileType, "Size") == 0)
		idx = 2;
	else
		idx = 0;

	ui->checkBox_AdvOutSplitFile->setChecked(splitFile);
	ui->comboBox_AdvOutSplitFileType->setCurrentIndex(idx);
	ui->spinBox_AdvOutSplitFileTime->setValue(splitFileTime);
	ui->spinBox_AdvOutSplitFileSize->setValue(splitFileSize);

	switch (flvTrack) {
	case 1:
		ui->radioButton_FlvTrack1->setChecked(true);
		break;
	case 2:
		ui->radioButton_FlvTrack2->setChecked(true);
		break;
	case 3:
		ui->radioButton_FlvTrack3->setChecked(true);
		break;
	case 4:
		ui->radioButton_FlvTrack4->setChecked(true);
		break;
	case 5:
		ui->radioButton_FlvTrack5->setChecked(true);
		break;
	case 6:
		ui->radioButton_FlvTrack6->setChecked(true);
		break;
	default:
		ui->radioButton_FlvTrack1->setChecked(true);
		break;
	}

	if (reset) {
		if (m_bIsAdvancedMode)
			emit qsignalAdvancedModeClicked();
		else
			emit qsignalSimpleModeClicked();
	}
}

#define AV_FORMAT_DEFAULT_STR AFSettingUtilsA::QTStr("Basic.Settings.Output.Adv.FFmpeg.FormatDefault")
#define AUDIO_STR AFSettingUtilsA::QTStr("Basic.Settings.Output.Adv.FFmpeg.FormatAudio")
#define VIDEO_STR AFSettingUtilsA::QTStr("Basic.Settings.Output.Adv.FFmpeg.FormatVideo")

void AFQOutputSettingAreaWidget::_LoadFormats()
{
#define FORMAT_STR(str) QTStr("Basic.Settings.Output.Format." str)

	ui->comboBox_AdvOutFFFormat->blockSignals(true);

	formats = GetSupportedFormats();

	for (auto& format : formats) {
		bool audio = format.HasAudio();
		bool video = format.HasVideo();

		if (audio || video) {
			QString itemText(format.name);
			if (audio ^ video)
				itemText += QString(" (%1)").arg(
					audio ? AUDIO_STR : VIDEO_STR);

			ui->comboBox_AdvOutFFFormat->addItem(
				itemText, QVariant::fromValue(format));
		}
	}
	ui->comboBox_AdvOutFFFormat->model()->sort(0);

	ui->comboBox_AdvOutFFFormat->insertItem(0, AV_FORMAT_DEFAULT_STR);

	ui->comboBox_AdvOutFFFormat->blockSignals(false);

	ui->comboBox_SimpleOutRecFormat->addItem(FORMAT_STR("FLV"), "flv");
	ui->comboBox_SimpleOutRecFormat->addItem(FORMAT_STR("MKV"), "mkv");
	ui->comboBox_SimpleOutRecFormat->addItem(FORMAT_STR("MP4"), "mp4");
	ui->comboBox_SimpleOutRecFormat->addItem(FORMAT_STR("MOV"), "mov");
	ui->comboBox_SimpleOutRecFormat->addItem(FORMAT_STR("fMP4"), "fragmented_mp4");
	ui->comboBox_SimpleOutRecFormat->addItem(FORMAT_STR("fMOV"), "fragmented_mov");
	ui->comboBox_SimpleOutRecFormat->addItem(FORMAT_STR("TS"), "mpegts");

	ui->comboBox_AdvOutRecFormat->addItem(FORMAT_STR("FLV"), "flv");
	ui->comboBox_AdvOutRecFormat->addItem(FORMAT_STR("MKV"), "mkv");
	ui->comboBox_AdvOutRecFormat->addItem(FORMAT_STR("MP4"), "mp4");
	ui->comboBox_AdvOutRecFormat->addItem(FORMAT_STR("MOV"), "mov");
	ui->comboBox_AdvOutRecFormat->addItem(FORMAT_STR("fMP4"), "fragmented_mp4");
	ui->comboBox_AdvOutRecFormat->addItem(FORMAT_STR("fMOV"), "fragmented_mov");
	ui->comboBox_AdvOutRecFormat->addItem(FORMAT_STR("TS"), "mpegts");
	ui->comboBox_AdvOutRecFormat->addItem(FORMAT_STR("HLS"), "hls");

#undef FORMAT_STR
}



static void AddDefaultCodec(QComboBox* combo, const FFmpegFormat& format, FFmpegCodecType codecType)
{

#define AV_ENCODER_DEFAULT_STR AFSettingUtilsA::QTStr("Basic.Settings.Output.Adv.FFmpeg.AVEncoderDefault")
	FFmpegCodec codec = format.GetDefaultEncoder(codecType);

	int existingIdx = AFSettingUtilsA::FindEncoder(combo, codec.name, codec.id);
	if (existingIdx >= 0)
		combo->removeItem(existingIdx);

	QString itemText;
	if (codec.long_name) {
		itemText = QString("%1 - %2 (%3)").arg(codec.name, codec.long_name, AV_ENCODER_DEFAULT_STR);
	}
	else {
		itemText = QString("%1 (%2)").arg(codec.name, AV_ENCODER_DEFAULT_STR);
	}

	combo->addItem(itemText, QVariant::fromValue(codec));
}


#define AV_ENCODER_DISABLE_STR AFSettingUtilsA::QTStr("Basic.Settings.Output.Adv.FFmpeg.AVEncoderDisable")
void AFQOutputSettingAreaWidget::_ReloadCodecs(const FFmpegFormat& format)
{
	ui->comboBox_AdvOutFFAEncoder->blockSignals(true);
	ui->comboBox_AdvOutFFVEncoder->blockSignals(true);
	ui->comboBox_AdvOutFFAEncoder->clear();
	ui->comboBox_AdvOutFFVEncoder->clear();

	bool ignore_compatibility = ui->checkBox_AdvOutFFIgnoreCompat->isChecked();
	vector<FFmpegCodec> supportedCodecs =
		GetFormatCodecs(format, ignore_compatibility);

	for (auto& codec : supportedCodecs) {
		switch (codec.type) {
		case AUDIO:
			AFSettingUtilsA::AddCodec(ui->comboBox_AdvOutFFAEncoder, codec);
			break;
		case VIDEO:
			AFSettingUtilsA::AddCodec(ui->comboBox_AdvOutFFVEncoder, codec);
			break;
		default:
			break;
		}
	}

	if (format.HasAudio())
		AddDefaultCodec(ui->comboBox_AdvOutFFAEncoder, format,
			FFmpegCodecType::AUDIO);
	if (format.HasVideo())
		AddDefaultCodec(ui->comboBox_AdvOutFFVEncoder, format,
			FFmpegCodecType::VIDEO);

	ui->comboBox_AdvOutFFAEncoder->model()->sort(0);
	ui->comboBox_AdvOutFFVEncoder->model()->sort(0);

	QVariant disable = QVariant::fromValue(FFmpegCodec());

	ui->comboBox_AdvOutFFAEncoder->insertItem(0, AV_ENCODER_DISABLE_STR, disable);
	ui->comboBox_AdvOutFFVEncoder->insertItem(0, AV_ENCODER_DISABLE_STR, disable);

	ui->comboBox_AdvOutFFAEncoder->blockSignals(false);
	ui->comboBox_AdvOutFFVEncoder->blockSignals(false);
}

void AFQOutputSettingAreaWidget::qslotSimpleOutputBrowseClicked()
{
	QString dir = SelectDirectory(
		this,
		AFSettingUtilsA::QTStr("Basic.Settings.Output.SelectDirectory"),
		ui->lineEdit_SimpleOutputPath->text());

	if (dir.isEmpty())
		return;

	ui->lineEdit_SimpleOutputPath->setText(dir);
}

void AFQOutputSettingAreaWidget::qslotAdvOutRecPathBrowseClicked()
{
	QString dir = SelectDirectory(
		this, 
		AFSettingUtilsA::QTStr("Basic.Settings.Output.SelectDirectory"), 
		ui->lineEdit_AdvOutRecPath->text());

	if (dir.isEmpty())
		return;

	ui->lineEdit_AdvOutRecPath->setText(dir);
}

void AFQOutputSettingAreaWidget::qslotAdvOutFFPathBrowseClicked()
{
	QString dir = SelectDirectory(
		this, QTStr("Basic.Settings.Output.SelectDirectory"),
		ui->lineEdit_AdvOutRecPath->text());
	if (dir.isEmpty())
		return;

	ui->lineEdit_AdvOutFFRecPath->setText(dir);
}

void AFQOutputSettingAreaWidget::qslotOutputDataChanged() {
	if (!m_bLoading)
	{
		m_bOutputChanged = true;
		sender()->setProperty("changed", QVariant(true));

		emit qsignalOutputDataChanged();
	}
}

void AFQOutputSettingAreaWidget::qslotOutputTypeChanged(int idx)
{
	if (!m_bIsAdvancedMode)
		return;

	if (idx == 0)
	{
		ui->widget_AdvRecStandardSetting->setVisible(true);
		ui->widget_AdvRecFFmpegSetting->setVisible(false);
	}
	else
	{
		ui->widget_AdvRecStandardSetting->setVisible(false);
		ui->widget_AdvRecFFmpegSetting->setVisible(true);
	}
}

void AFQOutputSettingAreaWidget::_SetAdvOutputFFmpegEnablement(FFmpegCodecType encoderType, bool enabled, bool enableEncoder)
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();
	bool rescale = config_get_bool(basicConfig, "AdvOut", "FFRescale");

	switch (encoderType) {
	case FFmpegCodecType::VIDEO:
		ui->spinBox_AdvOutFFVBitrate->setEnabled(enabled);
		ui->spinBox_AdvOutFFVGOPSize->setEnabled(enabled);
		ui->checkBox_AdvOutFFUseRescale->setEnabled(enabled);
		ui->comboBox_AdvOutFFRescale->setEnabled(enabled && rescale);
		ui->comboBox_AdvOutFFVEncoder->setEnabled(enabled || enableEncoder);
		ui->lineEdit_AdvOutFFVCfg->setEnabled(enabled);
		break;
	case FFmpegCodecType::AUDIO:
		ui->spinBox_AdvOutFFABitrate->setEnabled(enabled);
		ui->comboBox_AdvOutFFAEncoder->setEnabled(enabled || enableEncoder);
		ui->lineEdit_AdvOutFFACfg->setEnabled(enabled);
		ui->checkBox_AdvOutFFTrack1->setEnabled(enabled);
		ui->checkBox_AdvOutFFTrack2->setEnabled(enabled);
		ui->checkBox_AdvOutFFTrack3->setEnabled(enabled);
		ui->checkBox_AdvOutFFTrack4->setEnabled(enabled);
		ui->checkBox_AdvOutFFTrack5->setEnabled(enabled);
		ui->checkBox_AdvOutFFTrack6->setEnabled(enabled);
	default:
		break;
	}
}

void AFQOutputSettingAreaWidget::_LoadAdvancedOutputFFmpegSettings()
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();
	
	bool saveFile = config_get_bool(basicConfig, "AdvOut", "FFOutputToFile");
	const char* path = config_get_string(basicConfig, "AdvOut", "FFFilePath");
	bool noSpace = config_get_bool(basicConfig, "AdvOut", "FFFileNameWithoutSpace");
	const char* url = config_get_string(basicConfig, "AdvOut", "FFURL");
	const char* format = config_get_string(basicConfig, "AdvOut", "FFFormat");
	const char* mimeType = config_get_string(basicConfig, "AdvOut", "FFFormatMimeType");
	const char* muxCustom = config_get_string(basicConfig, "AdvOut", "FFMCustom");
	int videoBitrate = config_get_int(basicConfig, "AdvOut", "FFVBitrate");
	int gopSize = config_get_int(basicConfig, "AdvOut", "FFVGOPSize");
	bool rescale = config_get_bool(basicConfig, "AdvOut", "FFRescale");
	bool codecCompat = config_get_bool(basicConfig, "AdvOut", "FFIgnoreCompat");
	const char* rescaleRes = config_get_string(basicConfig, "AdvOut", "FFRescaleRes");
	const char* vEncoder = config_get_string(basicConfig, "AdvOut", "FFVEncoder");
	int vEncoderId = config_get_int(basicConfig, "AdvOut", "FFVEncoderId");
	const char* vEncCustom = config_get_string(basicConfig, "AdvOut", "FFVCustom");
	const char* aEncoder = config_get_string(basicConfig, "AdvOut", "FFAEncoder");
	int aEncoderId = config_get_int(basicConfig, "AdvOut", "FFAEncoderId");
	const char* aEncCustom = config_get_string(basicConfig, "AdvOut", "FFACustom");
	int audioBitrate = config_get_int(basicConfig, "AdvOut", "FFABitrate");
	int audioMixes = config_get_int(basicConfig, "AdvOut", "FFAudioMixes");

	ui->comboBox_AdvOutFFType->setCurrentIndex(saveFile ? 0 : 1);
	ui->lineEdit_AdvOutFFRecPath->setText(QT_UTF8(path));
	ui->checkBox_AdvOutFFNoSpace->setChecked(noSpace);
	ui->lineEdit_AdvOutFFURL->setText(QT_UTF8(url));
	AFSettingUtilsA::SelectFormat(ui->comboBox_AdvOutFFFormat, format, mimeType);
	ui->lineEdit_AdvOutFFMCfg->setText(muxCustom);          
	ui->spinBox_AdvOutFFVBitrate->setValue(videoBitrate);
	ui->spinBox_AdvOutFFVGOPSize->setValue(gopSize);
	ui->checkBox_AdvOutFFIgnoreCompat->setChecked(codecCompat);
	ui->checkBox_AdvOutFFUseRescale->setChecked(rescale);
	ui->comboBox_AdvOutFFRescale->setEnabled(rescale);
	ui->comboBox_AdvOutFFRescale->setCurrentText(rescaleRes);
	AFSettingUtilsA::SelectEncoder(ui->comboBox_AdvOutFFVEncoder, vEncoder, vEncoderId);
	ui->lineEdit_AdvOutFFVCfg->setText(vEncCustom);
	ui->spinBox_AdvOutFFABitrate->setValue(audioBitrate);
	AFSettingUtilsA::SelectEncoder(ui->comboBox_AdvOutFFAEncoder, aEncoder, aEncoderId);
	ui->lineEdit_AdvOutFFACfg->setText(aEncCustom);

	ui->checkBox_AdvOutFFTrack1->setChecked(audioMixes & (1 << 0));
	ui->checkBox_AdvOutFFTrack2->setChecked(audioMixes & (1 << 1));
	ui->checkBox_AdvOutFFTrack3->setChecked(audioMixes & (1 << 2));
	ui->checkBox_AdvOutFFTrack4->setChecked(audioMixes & (1 << 3));
	ui->checkBox_AdvOutFFTrack5->setChecked(audioMixes & (1 << 4));
	ui->checkBox_AdvOutFFTrack6->setChecked(audioMixes & (1 << 5));
}

#define TEXT_USE_STREAM_ENC \
	QTStr("Basic.Settings.Output.Adv.Recording.UseStreamEncoder")

void AFQOutputSettingAreaWidget::_ResetEncoders(bool streamOnly)
{
	const char* type;
	size_t idx = 0;

	if (!streamOnly) {
		ui->comboBox_AdvOutRecEncoder->clear();
		ui->comboBox_AdvOutRecAEncoder->clear();
	}

	while (obs_enum_encoder_types(idx++, &type)) {
		const char* name = obs_encoder_get_display_name(type);
		const char* codec = obs_get_encoder_codec(type);
		uint32_t caps = obs_get_encoder_caps(type);

		QString qName = QT_UTF8(name);
		QString qType = QT_UTF8(type);

		if (obs_get_encoder_type(type) == OBS_ENCODER_VIDEO) {
			if ((caps & ENCODER_HIDE_FLAGS) != 0)
				continue;

			if (!streamOnly)
				ui->comboBox_AdvOutRecEncoder->addItem(qName, qType);
		}

		if (obs_get_encoder_type(type) == OBS_ENCODER_AUDIO) {
			if (!streamOnly)
				ui->comboBox_AdvOutRecAEncoder->addItem(qName, qType);
		}
	}

	if (!streamOnly) {
		ui->comboBox_AdvOutRecEncoder->model()->sort(0);
		ui->comboBox_AdvOutRecEncoder->insertItem(0, TEXT_USE_STREAM_ENC,
			"none");
		ui->comboBox_AdvOutRecAEncoder->model()->sort(0);
		ui->comboBox_AdvOutRecAEncoder->insertItem(0, TEXT_USE_STREAM_ENC,
			"none");
	}
}

void AFQOutputSettingAreaWidget::_LoadOutputRecordingEncoderProperties()
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();
	const char* type = config_get_string(basicConfig, "AdvOut", "RecEncoder");
	delete recordEncoderProps;
	recordEncoderProps = nullptr;
	if (astrcmpi(type, "none") != 0) {
		recordEncoderProps = _CreateEncoderPropertyView(type, "recordEncoder.json");
		recordEncoderProps->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
		
		ui->verticalLayout_RecEncoder->addWidget(recordEncoderProps);
		connect(recordEncoderProps, &AFQPropertiesView::Changed, this, 
				&AFQOutputSettingAreaWidget::qslotAdvReplayBufferChanged);
	}
	curAdvRecordEncoder = type;

	if (!AFSettingUtilsA::SetComboByValue(ui->comboBox_AdvOutRecEncoder, type)) {
		uint32_t caps = obs_get_encoder_caps(type);
		if ((caps & ENCODER_HIDE_FLAGS) != 0) {
			const char* name = obs_encoder_get_display_name(type);

			ui->comboBox_AdvOutRecEncoder->insertItem(1, QT_UTF8(name), QT_UTF8(type));
			AFSettingUtilsA::SetComboByValue(ui->comboBox_AdvOutRecEncoder, type);
		}
		else {
			ui->comboBox_AdvOutRecEncoder->setCurrentIndex(-1);
		}
	}
}

void AFQOutputSettingAreaWidget::_LoadAdvReplayBuffer()
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();

	bool replayBuf = config_get_bool(basicConfig, "AdvOut", "RecRB");
	int rbTime = config_get_int(basicConfig, "AdvOut", "RecRBTime");
	int rbSize = config_get_int(basicConfig, "AdvOut", "RecRBSize");

	ui->checkBox_AdvReplayBuf->setChecked(replayBuf);
	ui->spinBox_AdvRBSecMax->setValue(rbTime);
	ui->spinBox_AdvRBMegsMax->setValue(rbSize);
}

AFQPropertiesView* AFQOutputSettingAreaWidget::_CreateEncoderPropertyView(const char* encoder, const char* path, bool changed)
{
	OBSDataAutoRelease settings = obs_encoder_defaults(encoder);
	AFQPropertiesView* view;
	if (path) {
		char encoderJsonPath[512];
		int ret = AFConfigManager::GetSingletonInstance().GetProfilePath(encoderJsonPath, sizeof(encoderJsonPath), path);
		if (ret > 0) {
			obs_data_t* data = obs_data_create_from_json_file_safe(encoderJsonPath, "bak");
			obs_data_apply(settings, data);
			obs_data_release(data);
		}
	}



	view = new AFQPropertiesView(settings.Get(), encoder, (PropertiesReloadCallback)obs_get_encoder_properties, ui->label_AdvOutRecEncoder->maximumWidth());

	view->SetScrollDisabled();
	view->SetPropertiesViewWidth(ui->widget_SimpleOutputSetting->width());
	view->SetLayoutMargin(0, 0, 0, 0);
	view->SetFormSpacing(0, 16);
	view->SetLabelFixedSize(ui->label_AdvOutRecEncoder->maximumSize());
	view->SetComboBoxFixedSize(ui->comboBox_AdvOutRecEncoder->maximumSize());
	view->SetLineEditFixedSize(ui->comboBox_AdvOutRecEncoder->maximumSize());
	view->SetSpinBoxFixedSize(ui->comboBox_AdvOutRecEncoder->maximumSize());
	view->SetCheckBoxFixedHeight(ui->checkBox_AdvOutNoSpace->maximumHeight());
	view->setFrameShape(QFrame::NoFrame);
	view->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	view->setProperty("changed", QVariant(changed));
	view->ReloadProperties();
    
	QObject::connect(view, &AFQPropertiesView::Changed, this, &AFQOutputSettingAreaWidget::qslotOutputDataChanged);

	return view;
}

void AFQOutputSettingAreaWidget::_ClearOutputSettingUi()
{
	ui->comboBox_SimpleOutRecFormat->clear();
	ui->comboBox_AdvOutRecFormat->clear();
	ui->comboBox_AdvOutFFFormat->clear();
}

void AFQOutputSettingAreaWidget::_FillSimpleRecordingValues()
{
#define ADD_QUALITY(str)                                                     \
	ui->comboBox_SimpleOutRecQuality->addItem(                                    \
		QTStr("Basic.Settings.Output.Simple.RecordingQuality." str), \
		QString(str));
#define ENCODER_STR(str) QTStr("Basic.Settings.Output.Simple.Encoder." str)

	ADD_QUALITY("Stream");
	ADD_QUALITY("Small");
	ADD_QUALITY("HQ");
	ADD_QUALITY("Lossless");

	ui->comboBox_SimpleOutRecEncoder->addItem(ENCODER_STR("Software"),
		QString(SIMPLE_ENCODER_X264));
	ui->comboBox_SimpleOutRecEncoder->addItem(ENCODER_STR("SoftwareLowCPU"),
		QString(SIMPLE_ENCODER_X264_LOWCPU));
	if (EncoderAvailable("obs_qsv11"))
		ui->comboBox_SimpleOutRecEncoder->addItem(
			ENCODER_STR("Hardware.QSV.H264"),
			QString(SIMPLE_ENCODER_QSV));
	if (EncoderAvailable("obs_qsv11_av1"))
		ui->comboBox_SimpleOutRecEncoder->addItem(
			ENCODER_STR("Hardware.QSV.AV1"),
			QString(SIMPLE_ENCODER_QSV_AV1));
	if (EncoderAvailable("ffmpeg_nvenc"))
		ui->comboBox_SimpleOutRecEncoder->addItem(
			ENCODER_STR("Hardware.NVENC.H264"),
			QString(SIMPLE_ENCODER_NVENC));
	if (EncoderAvailable("jim_av1_nvenc"))
		ui->comboBox_SimpleOutRecEncoder->addItem(
			ENCODER_STR("Hardware.NVENC.AV1"),
			QString(SIMPLE_ENCODER_NVENC_AV1));
#ifdef ENABLE_HEVC
	if (EncoderAvailable("h265_texture_amf"))
		ui->comboBox_SimpleOutRecEncoder->addItem(
			ENCODER_STR("Hardware.AMD.HEVC"),
			QString(SIMPLE_ENCODER_AMD_HEVC));
	if (EncoderAvailable("ffmpeg_hevc_nvenc"))
		ui->comboBox_SimpleOutRecEncoder->addItem(
			ENCODER_STR("Hardware.NVENC.HEVC"),
			QString(SIMPLE_ENCODER_NVENC_HEVC));
#endif
	if (EncoderAvailable("h264_texture_amf"))
		ui->comboBox_SimpleOutRecEncoder->addItem(
			ENCODER_STR("Hardware.AMD.H264"),
			QString(SIMPLE_ENCODER_AMD));
	if (EncoderAvailable("av1_texture_amf"))
		ui->comboBox_SimpleOutRecEncoder->addItem(
			ENCODER_STR("Hardware.AMD.AV1"),
			QString(SIMPLE_ENCODER_AMD_AV1));
	if (EncoderAvailable("com.apple.videotoolbox.videoencoder.ave.avc")
#ifndef __aarch64__
		&& os_get_emulation_status() == true
#endif
		)
		ui->comboBox_SimpleOutRecEncoder->addItem(
			ENCODER_STR("Hardware.Apple.H264"),
			QString(SIMPLE_ENCODER_APPLE_H264));
#ifdef ENABLE_HEVC
	if (EncoderAvailable("com.apple.videotoolbox.videoencoder.ave.hevc")
#ifndef __aarch64__
		&& os_get_emulation_status() == true
#endif
		)
		ui->comboBox_SimpleOutRecEncoder->addItem(
			ENCODER_STR("Hardware.Apple.HEVC"),
			QString(SIMPLE_ENCODER_APPLE_HEVC));
#endif

	if (EncoderAvailable("CoreAudio_AAC") ||
		EncoderAvailable("libfdk_aac") || EncoderAvailable("ffmpeg_aac"))
		ui->comboBox_SimpleOutRecAEncoder->addItem(
			QTStr("Basic.Settings.Output.Simple.Codec.AAC.Default"),
			"aac");
	if (EncoderAvailable("ffmpeg_opus"))
		ui->comboBox_SimpleOutRecAEncoder->addItem(
			QTStr("Basic.Settings.Output.Simple.Codec.Opus"),
			"opus");

#undef ADD_QUALITY
#undef ENCODER_STR
}

void AFQOutputSettingAreaWidget::ResetDownscales(uint32_t cx, uint32_t cy, bool ignoreAllSignals)
{
	QString advRescale;
	QString advRecRescale;
	QString advFFRescale;
	QString oldOutputRes;
	string bestScale;
	int bestPixelDiff = 0x7FFFFFFF;
	uint32_t out_cx = m_nOutputCX;
	uint32_t out_cy = m_nOutputCY;

	advRecRescale = ui->comboBox_AdvOutRecRescale->lineEdit()->text();
	advFFRescale = ui->comboBox_AdvOutFFRescale->lineEdit()->text();

	if (ignoreAllSignals) {
		ui->comboBox_AdvOutRecRescale->blockSignals(true);
		ui->comboBox_AdvOutFFRescale->blockSignals(true);
	}
	ui->comboBox_AdvOutRecRescale->clear();
	ui->comboBox_AdvOutFFRescale->clear();

	if (!out_cx || !out_cy) {
		out_cx = cx;
		out_cy = cy;
	}

	for (size_t idx = 0; idx < numVals; idx++) {
		uint32_t downscaleCX = uint32_t(double(cx) / vals[idx]);
		uint32_t downscaleCY = uint32_t(double(cy) / vals[idx]);
		uint32_t outDownscaleCX = uint32_t(double(out_cx) / vals[idx]);
		uint32_t outDownscaleCY = uint32_t(double(out_cy) / vals[idx]);

		downscaleCX &= 0xFFFFFFFC;
		downscaleCY &= 0xFFFFFFFE;
		outDownscaleCX &= 0xFFFFFFFE;
		outDownscaleCY &= 0xFFFFFFFE;

		string res = AFSettingUtilsA::ResString(downscaleCX, downscaleCY);
		string outRes = AFSettingUtilsA::ResString(outDownscaleCX, outDownscaleCY);

		ui->comboBox_AdvOutRecRescale->addItem(outRes.c_str());
		ui->comboBox_AdvOutFFRescale->addItem(outRes.c_str());

		int newPixelCount = int(downscaleCX * downscaleCY);
		int oldPixelCount = int(out_cx * out_cy);
		int diff = abs(newPixelCount - oldPixelCount);

		if (diff < bestPixelDiff) {
			bestScale = res;
			bestPixelDiff = diff;
		}
	}

	string res = AFSettingUtilsA::ResString(cx, cy);

	if (advRescale.isEmpty())
		advRescale = res.c_str();
	if (advRecRescale.isEmpty())
		advRecRescale = res.c_str();
	if (advFFRescale.isEmpty())
		advFFRescale = res.c_str();

	ui->comboBox_AdvOutRecRescale->lineEdit()->setText(advRecRescale);
	ui->comboBox_AdvOutFFRescale->lineEdit()->setText(advFFRescale);

	if (ignoreAllSignals) {
		ui->comboBox_AdvOutRecRescale->blockSignals(false);
		ui->comboBox_AdvOutFFRescale->blockSignals(false);
	}
}

void AFQOutputSettingAreaWidget::RefreshDownscales(uint32_t cx, uint32_t cy, const QComboBox* comboBox)
{
	if (comboBox == nullptr)
		return;

	QString advRecRescale = ui->comboBox_AdvOutRecRescale->lineEdit()->text();
	QString advFFRescale = ui->comboBox_AdvOutFFRescale->lineEdit()->text();

	ui->comboBox_AdvOutRecRescale->blockSignals(true);
	ui->comboBox_AdvOutFFRescale->blockSignals(true);

	ui->comboBox_AdvOutRecRescale->clear();
	ui->comboBox_AdvOutFFRescale->clear();

	for (int i = 0; i < comboBox->count(); ++i) {
		ui->comboBox_AdvOutRecRescale->addItem(comboBox->itemText(i));
		ui->comboBox_AdvOutFFRescale->addItem(comboBox->itemText(i));
	}

	string res = AFSettingUtilsA::ResString(cx, cy);
	if (advRecRescale.isEmpty())
		advRecRescale = res.c_str();
	if (advFFRescale.isEmpty())
		advFFRescale = res.c_str();

	ui->comboBox_AdvOutRecRescale->lineEdit()->setText(advRecRescale);
	ui->comboBox_AdvOutFFRescale->lineEdit()->setText(advFFRescale);

	ui->comboBox_AdvOutRecRescale->blockSignals(false);
	ui->comboBox_AdvOutFFRescale->blockSignals(false);
}

void AFQOutputSettingAreaWidget::ToggleOnStreaming(bool streaming)
{
	bool useVideo = obs_video_active() ? false : true;
	ui->frame_SettingMode->setEnabled(useVideo);
	ui->widget_SimpleOutputSetting->setEnabled(useVideo);
	ui->widget_SimpleReplayBuffer->setEnabled(useVideo);
	ui->comboBox_AdvOutRecType->setEnabled(useVideo);
	ui->widget_AdvRecFFmpegSetting->setEnabled(useVideo);
	ui->widget_AdvRecStandardSetting->setEnabled(useVideo);
}

QString AFQOutputSettingAreaWidget::GetSimpleAudioRecEncoder()
{
	if (ui->comboBox_SimpleOutRecAEncoder->currentIndex() < 0)
		return "";
	return ui->comboBox_SimpleOutRecAEncoder->currentData().toString();
}

QString AFQOutputSettingAreaWidget::GetSimpleVideoRecEncoder()
{
	if (ui->comboBox_SimpleOutRecEncoder->currentIndex() < 0)
		return "";
	return ui->comboBox_SimpleOutRecEncoder->currentData().toString();
}

QString AFQOutputSettingAreaWidget::GetSimpleRecQuality()
{
	if (ui->comboBox_SimpleOutRecQuality->currentIndex() < 0)
		return "";
	return ui->comboBox_SimpleOutRecQuality->currentData().toString();
}

QString AFQOutputSettingAreaWidget::GetSimpleRecFormat()
{
	if (ui->comboBox_SimpleOutRecFormat->currentIndex() < 0)
		return "";
	return ui->comboBox_SimpleOutRecFormat->currentData().toString();
}

QString AFQOutputSettingAreaWidget::GetAdvAudioRecEncoder()
{
	if (ui->comboBox_AdvOutRecAEncoder->currentIndex() < 0)
		return "";
	return ui->comboBox_AdvOutRecAEncoder->currentData().toString();
}

QString AFQOutputSettingAreaWidget::GetAdvVideoRecEncoder()
{
	if (ui->comboBox_AdvOutRecEncoder->currentIndex() < 0)
		return "";
	return ui->comboBox_AdvOutRecEncoder->currentData().toString();
}

QString AFQOutputSettingAreaWidget::GetAdvRecFormat()
{
	if (ui->comboBox_AdvOutRecFormat->currentIndex() < 0)
		return "";
	return ui->comboBox_AdvOutRecFormat->currentData().toString();
}

void AFQOutputSettingAreaWidget::ChangeSettingModeToSimple()
{
	if (m_bIsAdvancedMode)
	{
		m_bIsAdvancedMode = false;
		_SetSettingModeUi(m_bIsAdvancedMode);

		m_bOutputChanged = true;
		emit qsignalOutputDataChanged();
	}
}

void AFQOutputSettingAreaWidget::ChangeSettingModeToAdvanced()
{
	if (!m_bIsAdvancedMode)
	{
		m_bIsAdvancedMode = true;
		_SetSettingModeUi(m_bIsAdvancedMode);

		m_bOutputChanged = true;
		emit qsignalOutputDataChanged();
	}
}

void AFQOutputSettingAreaWidget::qslotAdvReplayBufferChanged()
{
	obs_data_t* settings;
	QString encoder = ui->comboBox_AdvOutRecEncoder->currentText();
	bool useStream = QString::compare(encoder, TEXT_USE_STREAM_ENC) == 0;

	if (useStream && m_bHasStreamEncoderData) {}
	else if (!useStream && recordEncoderProps) {
		settings = recordEncoderProps->GetSettings();
	}
	else {
		if (useStream)
			encoder = m_strAdvOutVEncoder;
		settings = obs_encoder_defaults(encoder.toUtf8().constData());

		if (!settings)
			return;

		char encoderJsonPath[512];
		int ret = AFConfigManager::GetSingletonInstance().GetProfilePath(encoderJsonPath, 
																		sizeof(encoderJsonPath), 
																		"recordEncoder.json");
		if (ret > 0) {
			OBSDataAutoRelease data =
				obs_data_create_from_json_file_safe(
					encoderJsonPath, "bak");
			obs_data_apply(settings, data);
		}
	}

	int vbitrate; 
	const char* rateControl; 

	if (useStream && m_bHasStreamEncoderData)
	{
		vbitrate = m_nVBitrate;
		rateControl = m_cRateControl;
	}
	else
	{
		if (!settings)
			return;

		vbitrate = (int)obs_data_get_int(settings, "bitrate");
		rateControl = obs_data_get_string(settings, "rate_control");
	}

	if (!rateControl)
		rateControl = "";

	bool lossless = strcmp(rateControl, "lossless") == 0 || ui->comboBox_AdvOutRecType->currentIndex() == 1;
	bool replayBufferEnabled = ui->checkBox_AdvReplayBuf->isChecked();

	int abitrate = 0;
	if (ui->checkBox_AdvOutRecTrack1->isChecked())
		abitrate += m_nSimpleOutABitrate;
	if (ui->checkBox_AdvOutRecTrack2->isChecked())
		abitrate += m_nSimpleOutABitrate;
	if (ui->checkBox_AdvOutRecTrack3->isChecked())
		abitrate += m_nSimpleOutABitrate;
	if (ui->checkBox_AdvOutRecTrack4->isChecked())
		abitrate += m_nSimpleOutABitrate;
	if (ui->checkBox_AdvOutRecTrack5->isChecked())
		abitrate += m_nSimpleOutABitrate;

	int seconds = ui->spinBox_AdvRBSecMax->value();
	uint64_t memTotal = os_get_sys_total_size();
	int64_t memMaxMB = memTotal ? memTotal * 3 / 4 / 1024 / 1024 : 8192;

	int64_t memMB = int64_t(seconds) * int64_t(vbitrate + abitrate)
		* 1000 / 8 / 1024 / 1024;
	if (memMB < 1)
		memMB = 1;

	bool varRateControl = (astrcmpi(rateControl, "CBR") == 0 ||
						   astrcmpi(rateControl, "VBR") == 0 ||
						   astrcmpi(rateControl, "ABR") == 0);
	if (vbitrate == 0)
		varRateControl = false;

	if (varRateControl) {
		ui->spinBox_AdvRBMegsMax->setVisible(false);
		ui->label_AdvRBMegsMax->setVisible(false);

		    if (memMB <= memMaxMB) {
		        ui->label_AdvRBEstimate->setText(
		            QTStr(ESTIMATE_STR)
		            .arg(QString::number(int(memMB))));
		    }
		    else {
		        ui->label_AdvRBEstimate->setText(
		            QTStr(ESTIMATE_TOO_LARGE_STR)
		            .arg(QString::number(int(memMB)),
		                QString::number(int(memMaxMB))));
		        ui->label_AdvRBEstimate->setObjectName("warningLabel");
		    }
	}
	else {
		ui->spinBox_AdvRBMegsMax->setVisible(true);
		ui->label_AdvRBMegsMax->setVisible(true);
		ui->spinBox_AdvRBMegsMax->setMaximum(memMaxMB);
		ui->label_AdvRBEstimate->setText(QTStr(ESTIMATE_UNKNOWN_STR));
	}
	int bitrateSum = vbitrate + abitrate;

	if (lossless) {
		ui->widget_AdvReplayBuffer->setEnabled(false);
		ui->frame_AdvReplayBufferGroup->setVisible(false);
	}
	else {
		ui->widget_AdvReplayBuffer->setEnabled(true);
		ui->frame_AdvReplayBufferGroup->setVisible(replayBufferEnabled);
		ui->label_AdvRBEstimate->style()->polish(ui->label_AdvRBEstimate);
	}

	qslotUpdateAutomaticReplayBufferCheckboxes();
}

void AFQOutputSettingAreaWidget::qslotAdvOutSplitFileChanged()
{
	bool splitFile = ui->checkBox_AdvOutSplitFile->isChecked();
	int splitFileType = splitFile ? ui->comboBox_AdvOutSplitFileType->currentIndex()
								  : -1;
	ui->comboBox_AdvOutSplitFileType->setEnabled(splitFile);
	ui->stackedwidget_AdvOutSplitFileType->setVisible(splitFileType > 0);

	if (splitFileType > 0)
	{
		ui->stackedwidget_AdvOutSplitFileType->setCurrentIndex(splitFileType - 1);
	}
}

static void DisableIncompatibleCodecs(QComboBox* cbox, const QString& format,
	const QString& formatName,
	const QString& streamEncoder)
{
	QString strEncLabel = AFSettingUtilsA::QTStr("Basic.Settings.Output.Adv.Recording.UseStreamEncoder");
	QString recEncoder = cbox->currentData().toString();

	bool currentCompatible = true;
	for (int idx = 0; idx < cbox->count(); idx++) {
		QString encName = cbox->itemData(idx).toString();
		string encoderId = (encName == "none")
			? streamEncoder.toStdString()
			: encName.toStdString();
		QString encDisplayName = (encName == "none")
			? strEncLabel
			: obs_encoder_get_display_name(
				encoderId.c_str());

		if (encoderId.empty())
			continue;

		const char* codec = obs_get_encoder_codec(encoderId.c_str());

		bool is_compatible =
			ContainerSupportsCodec(format.toStdString(), codec);
		if (!is_compatible && !IsBuiltinCodec(codec)) {
			string ext = AFSettingUtilsA::GetFormatExt(QT_TO_UTF8(format));
			is_compatible =
				FFCodecAndFormatCompatible(codec, ext.c_str());
		}

		QStandardItemModel* model =
			dynamic_cast<QStandardItemModel*>(cbox->model());
		QStandardItem* item = model->item(idx);

		if (is_compatible) {
			item->setFlags(Qt::ItemIsSelectable |
				Qt::ItemIsEnabled);
		}
		else {
			if (recEncoder == encName)
				currentCompatible = false;

			item->setFlags(Qt::NoItemFlags);
			encDisplayName += " ";
			encDisplayName += AFSettingUtilsA::QTStr("CodecCompat.Incompatible")
				.arg(formatName);
		}

		item->setText(encDisplayName);
	}
	QString styleSheet = "QComboBox::item::disabled { color: white; }";
	cbox->setStyleSheet(styleSheet);
	if (!currentCompatible)
		cbox->setCurrentIndex(-1);
}

void AFQOutputSettingAreaWidget::qslotAdvOutRecCheckCodecs()
{
	QString recFormat = ui->comboBox_AdvOutRecFormat->currentData().toString();
	QString recFormatName = ui->comboBox_AdvOutRecFormat->currentText();

	QString tooltip = QTStr("Basic.Settings.Output.Format.TT." + recFormat.toUtf8());

	if (!tooltip.startsWith("Basic.Settings.Output"))
		ui->comboBox_AdvOutRecFormat->setToolTip(tooltip);
	else
		ui->comboBox_AdvOutRecFormat->setToolTip(nullptr);

	QString streamEncoder = m_strAdvOutVEncoder;
	QString streamAudioEncoder = m_strAdvOutAEncoder;

	if (streamEncoder == "" || streamAudioEncoder == "")
		return;

	int oldVEncoderIdx = ui->comboBox_AdvOutRecEncoder->currentIndex();
	int oldAEncoderIdx = ui->comboBox_AdvOutRecAEncoder->currentIndex();
	DisableIncompatibleCodecs(ui->comboBox_AdvOutRecEncoder, recFormat,
								recFormatName, streamEncoder);
	DisableIncompatibleCodecs(ui->comboBox_AdvOutRecAEncoder, recFormat,
								recFormatName, streamAudioEncoder);

	if (ui->comboBox_AdvOutRecEncoder->currentIndex() == oldVEncoderIdx &&
		ui->comboBox_AdvOutRecAEncoder->currentIndex() == oldAEncoderIdx)
			qslotAdvOutRecCheckWarnings();
}

void AFQOutputSettingAreaWidget::qslotAdvOutRecEncoderCurrentIndexChanged(int idx) 
{
	if (!m_bLoading) {
		delete recordEncoderProps;
		recordEncoderProps = nullptr;
	}

	if (idx <= 0) {
		ui->widget_RecEncoder->setVisible(false);
		ui->checkBox_AdvOutRecUseRescale->setChecked(false);
		ui->widget_AdvOutRecUseRescale->setVisible(false);
		ui->comboBox_AdvOutRecRescale->setVisible(false);
		return;
	}

	QString encoder = AFSettingUtilsA::GetComboData(ui->comboBox_AdvOutRecEncoder);
	bool loadSettings = encoder == curAdvRecordEncoder;

	if (!m_bLoading) {
		recordEncoderProps = _CreateEncoderPropertyView(
			QT_TO_UTF8(encoder),
			loadSettings ? "recordEncoder.json" : nullptr, true);
		recordEncoderProps->setSizePolicy(QSizePolicy::Preferred,
			QSizePolicy::Minimum);
		ui->verticalLayout_RecEncoder->addWidget(recordEncoderProps);
		connect(recordEncoderProps, &AFQPropertiesView::Changed, this,
			&AFQOutputSettingAreaWidget::qslotAdvReplayBufferChanged);
	}

	ui->widget_RecEncoder->setVisible(true);
	ui->widget_AdvOutRecUseRescale->setVisible(true);
	ui->comboBox_AdvOutRecRescale->setVisible(true);
}

void AFQOutputSettingAreaWidget::qslotAdvOutFFIgnoreCompatStateChanged(int)
{
	qslotAdvOutFFFormatCurrentIndexChanged(
		ui->comboBox_AdvOutFFFormat->currentIndex());
}

#define DEFAULT_CONTAINER_STR \
	QTStr("Basic.Settings.Output.Adv.FFmpeg.FormatDescDef")

void AFQOutputSettingAreaWidget::qslotAdvOutFFFormatCurrentIndexChanged(int idx)
{
	const QVariant itemDataVariant = ui->comboBox_AdvOutFFFormat->itemData(idx);

	if (!itemDataVariant.isNull()) {
		auto format = itemDataVariant.value<FFmpegFormat>();
		_SetAdvOutputFFmpegEnablement(FFmpegCodecType::AUDIO,
									  format.HasAudio(), false);
		_SetAdvOutputFFmpegEnablement(FFmpegCodecType::VIDEO,
									  format.HasVideo(), false);
		_ReloadCodecs(format);

		ui->label_AdvOutFFFormatDesc->setText(format.long_name);

		FFmpegCodec defaultAudioCodecDesc =
			format.GetDefaultEncoder(FFmpegCodecType::AUDIO);
		FFmpegCodec defaultVideoCodecDesc =
			format.GetDefaultEncoder(FFmpegCodecType::VIDEO);
		AFSettingUtilsA::SelectEncoder(ui->comboBox_AdvOutFFAEncoder, defaultAudioCodecDesc.name,
									   defaultAudioCodecDesc.id);
		AFSettingUtilsA::SelectEncoder(ui->comboBox_AdvOutFFVEncoder, defaultVideoCodecDesc.name,
									   defaultVideoCodecDesc.id);
	}
	else {
		ui->comboBox_AdvOutFFAEncoder->blockSignals(true);
		ui->comboBox_AdvOutFFVEncoder->blockSignals(true);
		ui->comboBox_AdvOutFFAEncoder->clear();
		ui->comboBox_AdvOutFFVEncoder->clear();

		ui->label_AdvOutFFFormatDesc->setText(DEFAULT_CONTAINER_STR);
	}
}

void AFQOutputSettingAreaWidget::qslotAdvOutFFAEncoderCurrentIndexChanged(int idx)
{
	const QVariant itemDataVariant = ui->comboBox_AdvOutFFAEncoder->itemData(idx);
	if (!itemDataVariant.isNull()) {
		auto desc = itemDataVariant.value<FFmpegCodec>();
		_SetAdvOutputFFmpegEnablement(
			AUDIO, desc.id != 0 || desc.name != nullptr, true);
	}
}

void AFQOutputSettingAreaWidget::qslotAdvOutFFVEncoderCurrentIndexChanged(int idx)
{
	const QVariant itemDataVariant = ui->comboBox_AdvOutFFVEncoder->itemData(idx);
	if (!itemDataVariant.isNull()) {
		auto desc = itemDataVariant.value<FFmpegCodec>();
		_SetAdvOutputFFmpegEnablement(
			VIDEO, desc.id != 0 || desc.name != nullptr, true);
	}
}
void AFQOutputSettingAreaWidget::qslotAdvOutFFTypeCurrentIndexChanged(int idx)
{
	ui->stackedWidget_AdvOutPath->setCurrentIndex(idx);
	ui->checkBox_AdvOutFFNoSpace->setVisible(idx == 0);
}

#if defined(__APPLE__) && QT_VERSION < QT_VERSION_CHECK(6, 5, 1)
static void ResetInvalidSelection(QComboBox *cbox)
{
    int idx = cbox->currentIndex();
    if (idx < 0)
        return;

    QStandardItemModel *model =
        dynamic_cast<QStandardItemModel *>(cbox->model());
    QStandardItem *item = model->item(idx);

    if (item->isEnabled())
        return;

    cbox->blockSignals(true);
    cbox->setCurrentIndex(-1);
    cbox->blockSignals(false);
}
#endif

void AFQOutputSettingAreaWidget::qslotAdvOutRecCheckWarnings()
{
	auto Checked = [](QCheckBox* box) {
		return box->isChecked() ? 1 : 0;
		};

	QString errorMsg;
	QString warningMsg;
	uint32_t tracks =
			Checked(ui->checkBox_AdvOutRecTrack1) + Checked(ui->checkBox_AdvOutRecTrack2) +
			Checked(ui->checkBox_AdvOutRecTrack3) + Checked(ui->checkBox_AdvOutRecTrack4) +
			Checked(ui->checkBox_AdvOutRecTrack5) + Checked(ui->checkBox_AdvOutRecTrack6);

	bool useStreamEncoder = ui->comboBox_AdvOutRecEncoder->currentIndex() == 0;
	if (useStreamEncoder) {
		if (!warningMsg.isEmpty())
			warningMsg += "\n\n";
		warningMsg += QTStr("OutputWarnings.CannotPause");
	}

	QString recFormat = ui->comboBox_AdvOutRecFormat->currentData().toString();

	if (recFormat == "flv") {
		ui->stackedWidget_RecAudioTrack->setCurrentWidget(ui->widget_FlvTrack);
	}
	else {
		ui->stackedWidget_RecAudioTrack->setCurrentWidget(ui->widget_RecTrack);

		if (tracks == 0)
			errorMsg = AFSettingUtilsA::QTStr("OutputWarnings.NoTracksSelected");
	}

	if (recFormat == "mp4" || recFormat == "mov") {
		if (!warningMsg.isEmpty())
			warningMsg += "\n\n";

		warningMsg += QTStr("OutputWarnings.MP4Recording");
		emit qsignalSetAutoRemuxText(QTStr("Basic.Settings.Advanced.AutoRemux").arg("mp4") +
			" " + QTStr("Basic.Settings.Advanced.AutoRemux.MP4"));
	}
	else {
		emit qsignalSetAutoRemuxText(QTStr("Basic.Settings.Advanced.AutoRemux").arg("mp4"));
	}

#if defined(__APPLE__) && QT_VERSION < QT_VERSION_CHECK(6, 5, 1)
	ResetInvalidSelection(ui->comboBox_AdvOutRecEncoder);
	ResetInvalidSelection(ui->comboBox_AdvOutRecAEncoder);
#endif

	if (ui->comboBox_AdvOutRecEncoder->currentIndex() == -1 ||
		ui->comboBox_AdvOutRecAEncoder->currentIndex() == -1) 
	{
		if (!warningMsg.isEmpty())
			warningMsg += "\n\n";

		warningMsg += QTStr("OutputWarnings.CodecIncompatible");
	}
}

void AFQOutputSettingAreaWidget::qslotSimpleRecordingQualityChanged()
{
	QString qual = ui->comboBox_SimpleOutRecQuality->currentData().toString();
	bool streamQuality = qual == "Stream";
	bool losslessQuality = !streamQuality && qual == "Lossless";

	bool showEncoder = !streamQuality && !losslessQuality;
	ui->comboBox_SimpleOutRecEncoder->setVisible(showEncoder);
	ui->label_SimpleOutRecEncoder->setVisible(showEncoder);
	ui->comboBox_SimpleOutRecAEncoder->setVisible(showEncoder);
	ui->label_SimpleOutRecAEncoder->setVisible(showEncoder);
	ui->comboBox_SimpleOutRecFormat->setVisible(!losslessQuality);
	ui->label_SimpleOutRecFormat->setVisible(!losslessQuality);

	qslotSimpleRecordingEncoderChanged();
	qslotSimpleReplayBufferChanged();
}

void AFQOutputSettingAreaWidget::qslotSimpleReplayBufferChanged()
{
	QString qual = ui->comboBox_SimpleOutRecQuality->currentData().toString();
	bool replayBufferEnabled = ui->checkBox_SimpleReplayBuf->isChecked();
	bool lossless = qual == "Lossless";
	bool streamQuality = qual == "Stream";
	int abitrate = 0;

	ui->spinBox_SimpleRBMegsMax->setVisible(!streamQuality);
	ui->label_SimpleRBMegsMax->setVisible(!streamQuality);

	if (ui->comboBox_SimpleOutRecFormat->currentText().compare("flv") == 0 ||
		streamQuality) {
		abitrate = m_nSimpleOutABitrate;
	}
	else {
		int delta = m_nSimpleOutABitrate;
		if (ui->checkBox_SimpleOutRecTrack1->isChecked())
			abitrate += delta;
		if (ui->checkBox_SimpleOutRecTrack2->isChecked())
			abitrate += delta;
		if (ui->checkBox_SimpleOutRecTrack3->isChecked())
			abitrate += delta;
		if (ui->checkBox_SimpleOutRecTrack4->isChecked())
			abitrate += delta;
		if (ui->checkBox_SimpleOutRecTrack5->isChecked())
			abitrate += delta;
		if (ui->checkBox_SimpleOutRecTrack6->isChecked())
			abitrate += delta;
	}

	int vbitrate = m_nSimpleOutVBitrate;
	int seconds = ui->spinBox_SimpleRBSecMax->value();

	uint64_t memTotal = os_get_sys_total_size();
	int64_t memMaxMB = memTotal ? memTotal * 3 / 4 / 1024 / 1024 : 8192;

	int64_t memMB = int64_t(seconds) * int64_t(vbitrate + abitrate) * 1000 /
		8 / 1024 / 1024;
	if (memMB < 1)
		memMB = 1;

	ui->label_SimpleRBEstimate->setObjectName("");
	if (streamQuality) {
		if (memMB <= memMaxMB) {
			ui->label_SimpleRBEstimate->setText(
				QTStr(ESTIMATE_STR)
				.arg(QString::number(int(memMB))));
		}
		else {
			ui->label_SimpleRBEstimate->setText(
				QTStr(ESTIMATE_TOO_LARGE_STR)
				.arg(QString::number(int(memMB)),
					QString::number(int(memMaxMB))));
			ui->label_SimpleRBEstimate->setObjectName("warningLabel");
		}
	}
	else {
		ui->label_SimpleRBEstimate->setText(QTStr(ESTIMATE_UNKNOWN_STR));
		ui->spinBox_SimpleRBMegsMax->setMaximum(memMaxMB);
	}

	ui->label_SimpleRBEstimate->style()->polish(ui->label_SimpleRBEstimate);
	
	if (lossless) {
		ui->widget_SimpleReplayBuffer->setEnabled(false);
		ui->frame_ReplayBufferGroup->setVisible(false);
	}
	else {
		ui->widget_SimpleReplayBuffer->setEnabled(true);
		ui->frame_ReplayBufferGroup->setVisible(replayBufferEnabled);
	}


	qslotUpdateAutomaticReplayBufferCheckboxes();
}

static void SaveTrackIndex(config_t* config, const char* section,
	const char* name, QAbstractButton* check1,
	QAbstractButton* check2, QAbstractButton* check3,
	QAbstractButton* check4, QAbstractButton* check5,
	QAbstractButton* check6)
{
	if (check1->isChecked())
		config_set_int(config, section, name, 1);
	else if (check2->isChecked())
		config_set_int(config, section, name, 2);
	else if (check3->isChecked())
		config_set_int(config, section, name, 3);
	else if (check4->isChecked())
		config_set_int(config, section, name, 4);
	else if (check5->isChecked())
		config_set_int(config, section, name, 5);
	else if (check6->isChecked())
		config_set_int(config, section, name, 6);
}

void AFQOutputSettingAreaWidget::SaveOutputFormat(QComboBox* combo)
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();
	QVariant v = combo->currentData();
	if (!v.isNull()) {
		auto format = v.value<FFmpegFormat>();
		config_set_string(basicConfig, "AdvOut", "FFFormat",
			format.name);
		config_set_string(basicConfig, "AdvOut", "FFFormatMimeType",
			format.mime_type);

		const char* ext = format.extensions;
		string extStr = ext ? ext : "";

		char* comma = strchr(&extStr[0], ',');
		if (comma)
			*comma = 0;

		config_set_string(basicConfig, "AdvOut", "FFExtension",
			extStr.c_str());
	}
	else {
		config_set_string(basicConfig, "AdvOut", "FFFormat",
			nullptr);
		config_set_string(basicConfig, "AdvOut", "FFFormatMimeType",
			nullptr);

		config_remove_value(basicConfig, "AdvOut", "FFExtension");
	}
}

void AFQOutputSettingAreaWidget::SaveOutputEncoder(QComboBox* combo, const char* section, const char* value)
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();
	QVariant v = combo->currentData();
	FFmpegCodec cd{};
	if (!v.isNull())
		cd = v.value<FFmpegCodec>();

	config_set_int(basicConfig, section,
		QT_TO_UTF8(QString("%1Id").arg(value)), cd.id);
	if (cd.id != 0)
		config_set_string(basicConfig, section, value, cd.name);
	else
		config_set_string(basicConfig, section, value, nullptr);
}

void AFQOutputSettingAreaWidget::SaveOutputSettings()
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();

	if(!m_bIsAdvancedMode)
		config_set_string(basicConfig, "Output", "Mode", "Simple");
	else
		config_set_string(basicConfig, "Output", "Mode", "Advanced");

	AFSettingUtilsA::SaveEdit(ui->lineEdit_SimpleOutputPath, "SimpleOutput", "FilePath");
	AFSettingUtilsA::SaveCheckBox(ui->checkBox_SimpleNoSpace, "SimpleOutput", "FileNameWithoutSpace");
	AFSettingUtilsA::SaveComboData(ui->comboBox_SimpleOutRecFormat, "SimpleOutput", "RecFormat2");
	AFSettingUtilsA::SaveComboData(ui->comboBox_SimpleOutRecQuality, "SimpleOutput", "RecQuality");
	AFSettingUtilsA::SaveComboData(ui->comboBox_SimpleOutRecEncoder, "SimpleOutput", "RecEncoder");
	AFSettingUtilsA::SaveComboData(ui->comboBox_SimpleOutRecAEncoder, "SimpleOutput", "RecAudioEncoder");
	AFSettingUtilsA::SaveEdit(ui->lineEdit_SimpleOutMuxCustom, "SimpleOutput", "MuxerCustom");
	AFSettingUtilsA::SaveCheckBox(ui->checkBox_SimpleReplayBuf, "SimpleOutput", "RecRB");
	AFSettingUtilsA::SaveSpinBox(ui->spinBox_SimpleRBSecMax, "SimpleOutput", "RecRBTime");
	AFSettingUtilsA::SaveSpinBox(ui->spinBox_SimpleRBMegsMax, "SimpleOutput", "RecRBSize");
	config_set_int(basicConfig, "SimpleOutput", "RecTracks", qslotSimpleOutGetSelectedAudioTracks());

	curAdvRecordEncoder = AFSettingUtilsA::GetComboData(ui->comboBox_AdvOutRecType);

	AFSettingUtilsA::SaveEdit(ui->lineEdit_AdvOutRecPath, "AdvOut", "RecFilePath");
	AFSettingUtilsA::SaveCheckBox(ui->checkBox_AdvOutNoSpace, "AdvOut", "RecFileNameWithoutSpace");
	AFSettingUtilsA::SaveComboData(ui->comboBox_AdvOutRecFormat, "AdvOut", "RecFormat2");
	AFSettingUtilsA::SaveComboData(ui->comboBox_AdvOutRecEncoder, "AdvOut", "RecEncoder");
	AFSettingUtilsA::SaveComboData(ui->comboBox_AdvOutRecAEncoder, "AdvOut", "RecAudioEncoder");
	AFSettingUtilsA::SaveCheckBox(ui->checkBox_AdvOutRecUseRescale, "AdvOut", "RecRescale");

	AFSettingUtilsA::SaveCombo(ui->comboBox_AdvOutRecRescale, "AdvOut", "RecRescaleRes");
	AFSettingUtilsA::SaveEdit(ui->lineEdit_AdvOutMuxCustom, "AdvOut", "RecMuxerCustom");
	AFSettingUtilsA::SaveCheckBox(ui->checkBox_AdvOutSplitFile, "AdvOut", "RecSplitFile");
	config_set_string(basicConfig, "AdvOut", "RecSplitFileType", SplitFileTypeFromIdx(ui->comboBox_AdvOutSplitFileType->currentIndex()));
	AFSettingUtilsA::SaveSpinBox(ui->spinBox_AdvOutSplitFileTime, "AdvOut", "RecSplitFileTime");
	AFSettingUtilsA::SaveSpinBox(ui->spinBox_AdvOutSplitFileSize, "AdvOut", "RecSplitFileSize");

	config_set_string(basicConfig, "AdvOut", "RecType", RecTypeFromIdx(ui->comboBox_AdvOutRecType->currentIndex()));
	config_set_int(basicConfig, "AdvOut", "RecTracks", qslotAdvOutGetSelectedAudioTracks());
	config_set_int(basicConfig, "AdvOut", "FLVTrack", _CurrentFLVTrack());

	config_set_bool(basicConfig, "AdvOut", "FFOutputToFile", ui->comboBox_AdvOutFFType->currentIndex() == 0 ? true : false);
	AFSettingUtilsA::SaveEdit(ui->lineEdit_AdvOutFFRecPath, "AdvOut", "FFFilePath");
	AFSettingUtilsA::SaveCheckBox(ui->checkBox_AdvOutFFNoSpace, "AdvOut", "FFFileNameWithoutSpace");
	AFSettingUtilsA::SaveEdit(ui->lineEdit_AdvOutFFURL, "AdvOut", "FFURL");
	_SaveFormat(ui->comboBox_AdvOutFFFormat);
	AFSettingUtilsA::SaveEdit(ui->lineEdit_AdvOutFFMCfg, "AdvOut", "FFMCustom");
	AFSettingUtilsA::SaveSpinBox(ui->spinBox_AdvOutFFVBitrate, "AdvOut", "FFVBitrate");
	AFSettingUtilsA::SaveSpinBox(ui->spinBox_AdvOutFFVGOPSize, "AdvOut", "FFVGOPSize");
	AFSettingUtilsA::SaveCheckBox(ui->checkBox_AdvOutFFIgnoreCompat, "AdvOut", "FFIgnoreCompat");
	AFSettingUtilsA::SaveCheckBox(ui->checkBox_AdvOutFFUseRescale, "AdvOut", "FFRescale");
	AFSettingUtilsA::SaveCombo(ui->comboBox_AdvOutFFRescale, "AdvOut", "FFRescaleRes");
	_SaveEncoder(ui->comboBox_AdvOutFFVEncoder, "AdvOut", "FFVEncoder");
	AFSettingUtilsA::SaveEdit(ui->lineEdit_AdvOutFFVCfg, "AdvOut", "FFVCustom");
	AFSettingUtilsA::SaveSpinBox(ui->spinBox_AdvOutFFABitrate, "AdvOut", "FFABitrate");
	_SaveEncoder(ui->comboBox_AdvOutFFAEncoder, "AdvOut", "FFAEncoder");
	AFSettingUtilsA::SaveEdit(ui->lineEdit_AdvOutFFACfg, "AdvOut", "FFACustom");
	config_set_int(
		basicConfig, "AdvOut", "FFAudioMixes",
		(ui->checkBox_AdvOutFFTrack1->isChecked() ? (1 << 0) : 0) |
		(ui->checkBox_AdvOutFFTrack2->isChecked() ? (1 << 1) : 0) |
		(ui->checkBox_AdvOutFFTrack3->isChecked() ? (1 << 2) : 0) |
		(ui->checkBox_AdvOutFFTrack4->isChecked() ? (1 << 3) : 0) |
		(ui->checkBox_AdvOutFFTrack5->isChecked() ? (1 << 4) : 0) |
		(ui->checkBox_AdvOutFFTrack6->isChecked() ? (1 << 5) : 0));

	AFSettingUtilsA::SaveCheckBox(ui->checkBox_AdvReplayBuf, "AdvOut", "RecRB");

	if (!m_bIsAdvancedMode)
		App()->GetMainView()->GetMainWindow()->EnableReplayBuffer(ui->checkBox_SimpleReplayBuf->isChecked());
	else {
		const char* advRecType = RecTypeFromIdx(ui->comboBox_AdvOutRecType->currentIndex());
		if(astrcmpi(advRecType, "FFmpeg") == 0)
			App()->GetMainView()->GetMainWindow()->EnableReplayBuffer(false);
		else
			App()->GetMainView()->GetMainWindow()->EnableReplayBuffer(ui->checkBox_AdvReplayBuf->isChecked());
	}


	AFSettingUtilsA::SaveSpinBox(ui->spinBox_AdvRBSecMax, "AdvOut", "RecRBTime");
	AFSettingUtilsA::SaveSpinBox(ui->spinBox_AdvRBMegsMax, "AdvOut", "RecRBSize");

	WriteJsonData(recordEncoderProps, "recordEncoder.json");
}

void AFQOutputSettingAreaWidget::ResetOutputSettings()
{
	_ClearOutputSettingUi();
	LoadOutputSettings(true);
}

void AFQOutputSettingAreaWidget::AdvOutEncoderData(QString advOutVEncoder, QString advOutAEncoder)
{
	m_strAdvOutVEncoder = advOutVEncoder;
	m_strAdvOutAEncoder = advOutAEncoder;
	qslotAdvOutRecCheckCodecs();
}

void AFQOutputSettingAreaWidget::SimpleOutVEncoder(QString simpleOutVEncoder)
{
	m_strSimpleOutVEncoder = simpleOutVEncoder;

}
void AFQOutputSettingAreaWidget::SimpleOutAEncoder(QString simpleOutAEncoder)
{
	m_strSimpleOutAEncoder = simpleOutAEncoder;
}

void AFQOutputSettingAreaWidget::SimpleOutVBitrate(int simpleVBitrate)
{
	m_nSimpleOutVBitrate = simpleVBitrate;
}

void AFQOutputSettingAreaWidget::SimpleOutABitrate(int simpleABitrate)
{
	m_nSimpleOutABitrate = simpleABitrate;
}

void AFQOutputSettingAreaWidget::StreamEncoderData(int vbitrate, const char* rateControl)
{
	m_nVBitrate = vbitrate;
	m_cRateControl = rateControl;
}

void AFQOutputSettingAreaWidget::OutputResolution(uint32_t cx, uint32_t cy)
{
	m_nOutputCX = cx;
	m_nOutputCY = cy;
}

void AFQOutputSettingAreaWidget::_SaveFormat(QComboBox* combo)
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();
	QVariant v = combo->currentData();
	if (!v.isNull()) {
		auto format = v.value<FFmpegFormat>();
		config_set_string(basicConfig, "AdvOut", "FFFormat",
			format.name);
		config_set_string(basicConfig, "AdvOut", "FFFormatMimeType",
			format.mime_type);

		const char* ext = format.extensions;
		string extStr = ext ? ext : "";

		char* comma = strchr(&extStr[0], ',');
		if (comma)
			*comma = 0;

		config_set_string(basicConfig, "AdvOut", "FFExtension",
			extStr.c_str());
	}
	else {
		config_set_string(basicConfig, "AdvOut", "FFFormat",
			nullptr);
		config_set_string(basicConfig, "AdvOut", "FFFormatMimeType",
			nullptr);

		config_remove_value(basicConfig, "AdvOut", "FFExtension");
	}
}

void AFQOutputSettingAreaWidget::_SaveEncoder(QComboBox* combo, const char* section, const char* value)
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();
	QVariant v = combo->currentData();
	FFmpegCodec cd{};
	if (!v.isNull())
		cd = v.value<FFmpegCodec>();

	config_set_int(basicConfig, section,
		QT_TO_UTF8(QString("%1Id").arg(value)), cd.id);
	if (cd.id != 0)
		config_set_string(basicConfig, section, value, cd.name);
	else
		config_set_string(basicConfig, section, value, nullptr);
}

int AFQOutputSettingAreaWidget::qslotSimpleOutGetSelectedAudioTracks()
{
	int tracks = (ui->checkBox_SimpleOutRecTrack1->isChecked() ? (1 << 0) : 0) |
		(ui->checkBox_SimpleOutRecTrack2->isChecked() ? (1 << 1) : 0) |
		(ui->checkBox_SimpleOutRecTrack3->isChecked() ? (1 << 2) : 0) |
		(ui->checkBox_SimpleOutRecTrack4->isChecked() ? (1 << 3) : 0) |
		(ui->checkBox_SimpleOutRecTrack5->isChecked() ? (1 << 4) : 0) |
		(ui->checkBox_SimpleOutRecTrack6->isChecked() ? (1 << 5) : 0);
	return tracks;
}

int AFQOutputSettingAreaWidget::qslotAdvOutGetSelectedAudioTracks()
{
	int tracks = (ui->checkBox_AdvOutRecTrack1->isChecked() ? (1 << 0) : 0) |
		(ui->checkBox_AdvOutRecTrack2->isChecked() ? (1 << 1) : 0) |
		(ui->checkBox_AdvOutRecTrack3->isChecked() ? (1 << 2) : 0) |
		(ui->checkBox_AdvOutRecTrack4->isChecked() ? (1 << 3) : 0) |
		(ui->checkBox_AdvOutRecTrack5->isChecked() ? (1 << 4) : 0) |
		(ui->checkBox_AdvOutRecTrack6->isChecked() ? (1 << 5) : 0);
	return tracks;
}

int AFQOutputSettingAreaWidget::_CurrentFLVTrack()
{
	if (ui->radioButton_FlvTrack1->isChecked())
		return 1;
	else if (ui->radioButton_FlvTrack2->isChecked())
		return 2;
	else if (ui->radioButton_FlvTrack3->isChecked())
		return 3;
	else if (ui->radioButton_FlvTrack4->isChecked())
		return 4;
	else if (ui->radioButton_FlvTrack5->isChecked())
		return 5;
	else if (ui->radioButton_FlvTrack6->isChecked())
		return 6;
	return 0;
}

void AFQOutputSettingAreaWidget::_ChangeLanguage()
{
	QList<QLabel*> labelList = findChildren<QLabel*>();
	QList<QCheckBox*> checkboxList = findChildren<QCheckBox*>();
	QList<QComboBox*> comboboxList = findChildren<QComboBox*>();

	foreach(QLabel * label, labelList)
	{
		QString qtranslate = QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str(label->text().toUtf8().constData()));
		label->setText(qtranslate);
	}

	foreach(QCheckBox * checkbox, checkboxList)
	{
		QString qtranslate = QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str(checkbox->text().toUtf8().constData()));
		checkbox->setText(qtranslate);
	}
	foreach(QComboBox* combobox, comboboxList)
	{
		int itemCount = combobox->count();
		for (int i = 0; i < itemCount; ++i)
		{
			QString itemString = combobox->itemText(i);
			QRegularExpression rx("\\d+");

			if (rx.match(itemString).hasMatch())
				continue;
			QString qtranslate = QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str(itemString.toUtf8().constData()));
			combobox->setItemText(i, qtranslate);
		}
	}
}
