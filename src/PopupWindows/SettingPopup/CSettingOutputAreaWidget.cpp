#include "CSettingOutputAreaWidget.h"

#include <util/dstr.hpp>
#include <sstream> // stringstream
#include <string>
#include <QCompleter>

#include "qt-wrappers.hpp"

#include "UIComponent/CMessageBox.h"
#include "Common/SettingsMiscDef.h"
#include "Common/StringMiscUtils.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/OBSOutput/SBasicOutputHandler.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Encoder/CEncoder.h"

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

static void WriteJsonData(OBSPropertiesView* view, const char* path)
{
	char full_path[512];

	if (!view || !AFSettingUtils::WidgetChanged(view))
		return;

	int ret = GetProfilePath(full_path, sizeof(full_path), path);
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
	/* Similar to above, but works in reverse to disable incompatible formats
	 * based on the encoder selection. */
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
	/* Unlike in advanced mode the available simple mode encoders are
	 * hardcoded, so this check is also a simpler, hardcoded one. */
	QString encoder = cbox->currentData().toString();

	bool currentCompatible = true;
	for (int idx = 0; idx < cbox->count(); idx++) {
		QString encName = cbox->itemData(idx).toString();
		QString codec;

		/* Simple mode does not expose audio encoder variants directly,
		 * so we have to simply set the codec to the internal name. */
		if (encName == "opus" || encName == "aac") {
			codec = encName;
		}
		else {
			const char* encoder_id = get_simple_output_encoder(QT_TO_UTF8(encName));
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
	QWidget(parent),
	ui(new Ui::AFQOutputSettingAreaWidget)
{
	ui->setupUi(this);

	_FillSimpleRecordingValues();
	_EventHookConnect();

	if (MAINFRAME->IsSmallResolution())
	{
		ui->scrollArea->setFixedWidth(796);
		ui->scrollAreaWidgetContents->setFixedWidth(796);
	}
}

AFQOutputSettingAreaWidget::~AFQOutputSettingAreaWidget()
{
	delete ui;
}

void AFQOutputSettingAreaWidget::LoadOutputSettings(bool reset) {
	// `streaming
	m_loading = true;
	if (!m_service) {
		m_service = obs_service_create("rtmp_common", NULL, NULL, nullptr);
		obs_service_release(m_service);
	}
	// `streaming
	m_protocol = QT_UTF8(obs_service_get_protocol(m_service));

	_ResetEncoders(false);
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

	auto activeConfig = ACTIVECONFIG;
	//
	uint32_t cx = config_get_uint(activeConfig, "Video", "BaseCX");
	uint32_t cy = config_get_uint(activeConfig, "Video", "BaseCY");
	uint32_t out_cx = config_get_uint(activeConfig, "Video", "OutputCX");
	uint32_t out_cy = config_get_uint(activeConfig, "Video", "OutputCY");
	string outputResString = AFSettingUtils::ResString(out_cx, out_cy);
	ResetDownscales(cx, cy);

	qslotOutputTypeChanged(ui->comboBox_AdvOutRecType->currentIndex());
	qslotAdvOutSplitFileChanged();
	qslotAdvOutFFTypeCurrentIndexChanged(ui->comboBox_AdvOutFFType->currentIndex());

	ToggleOnStreaming(false);
	_SetSettingModeUi(m_isAdvancedMode);

	//if (modeIdx) {
	//	ui->widget_Advanced->setVisible(false);
	//	ui->widget_OutputType->setVisible(false);
	//}
	//else {
	//	ui->widget_Advanced->setVisible(true);
	//	ui->widget_OutputType->setVisible(true);
	//}

	m_loading = false;
}

void AFQOutputSettingAreaWidget::LoadSimpleOutputSettings()
{
	auto config = ACTIVECONFIG;
	//
	const char* path = config_get_string(config, "SimpleOutput", "FilePath");
	bool noSpace = config_get_bool(config, "SimpleOutput", "FileNameWithoutSpace");
	const char* recQual = config_get_string(config, "SimpleOutput", "RecQuality");
	const char* format = config_get_string(config, "SimpleOutput", "RecFormat2");
	const char* recEnc = config_get_string(config, "SimpleOutput", "RecEncoder");
	const char* recAudioEnc = config_get_string(config, "SimpleOutput", "RecAudioEncoder");
	const char* muxCustom = config_get_string(config, "SimpleOutput", "MuxerCustom");
	bool replayBuf = config_get_bool(config, "SimpleOutput", "RecRB");
	int rbTime = config_get_int(config, "SimpleOutput", "RecRBTime");
	int rbSize = config_get_int(config, "SimpleOutput", "RecRBSize");
	int tracks = config_get_int(config, "SimpleOutput", "RecTracks");

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

void AFQOutputSettingAreaWidget::_EventHookConnect()
{
	AFSettingUtils::HookWidget(ui->comboBox_SimpleOutRecQuality, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_SimpleOutRecFormat, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_SimpleOutRecEncoder, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_SimpleOutRecAEncoder, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_SimpleNoSpace, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_SimpleOutRecTrack1, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_SimpleOutRecTrack2, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_SimpleOutRecTrack3, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_SimpleOutRecTrack4, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_SimpleOutRecTrack5, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_SimpleOutRecTrack6, this, CHECK_CHANGED, OUTPUTS_CHANGED);

	AFSettingUtils::HookWidget(ui->comboBox_AdvOutRecFormat, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_AdvOutRecType, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_AdvOutRecEncoder, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_AdvOutRecAEncoder, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_AdvOutRecRescale, this, CBEDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_AdvOutSplitFileType, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_AdvOutFFType, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_AdvOutFFFormat, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_AdvOutFFRescale, this, CBEDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_AdvOutFFVEncoder, this, COMBO_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_AdvOutFFAEncoder, this, COMBO_CHANGED, OUTPUTS_CHANGED);

	AFSettingUtils::HookWidget(ui->checkBox_AdvOutNoSpace, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutRecTrack1, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutRecTrack2, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutRecTrack3, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutRecTrack4, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutRecTrack5, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutRecTrack6, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutFFTrack1, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutFFTrack2, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutFFTrack3, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutFFTrack4, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutFFTrack5, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutFFTrack6, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutSplitFile, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutFFNoSpace, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutFFIgnoreCompat, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutFFUseRescale, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutRecUseRescale, this, CHECK_CHANGED, OUTPUTS_CHANGED);

	AFSettingUtils::HookWidget(ui->radioButton_FlvTrack1, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->radioButton_FlvTrack2, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->radioButton_FlvTrack3, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->radioButton_FlvTrack4, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->radioButton_FlvTrack5, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->radioButton_FlvTrack6, this, CHECK_CHANGED, OUTPUTS_CHANGED);

	AFSettingUtils::HookWidget(ui->lineEdit_SimpleOutputPath, this, EDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->lineEdit_SimpleOutMuxCustom, this, EDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_SimpleReplayBuf, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->lineEdit_AdvOutRecPath, this, EDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->lineEdit_AdvOutMuxCustom, this, EDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->lineEdit_AdvOutFFRecPath, this, EDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->lineEdit_AdvOutFFURL, this, EDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->lineEdit_AdvOutFFMCfg, this, EDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->lineEdit_AdvOutFFVCfg, this, EDIT_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->lineEdit_AdvOutFFACfg, this, EDIT_CHANGED, OUTPUTS_CHANGED);

	AFSettingUtils::HookWidget(ui->spinBox_SimpleRBSecMax, this, SCROLL_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->spinBox_SimpleRBMegsMax, this, SCROLL_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->spinBox_AdvOutSplitFileTime, this, SCROLL_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->spinBox_AdvOutSplitFileSize, this, SCROLL_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->spinBox_AdvOutFFVBitrate, this, SCROLL_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->spinBox_AdvOutFFVGOPSize, this, SCROLL_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->spinBox_AdvOutFFABitrate, this, SCROLL_CHANGED, OUTPUTS_CHANGED);

	AFSettingUtils::HookWidget(ui->checkBox_AdvReplayBuf, this, CHECK_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->spinBox_AdvRBSecMax, this, SCROLL_CHANGED, OUTPUTS_CHANGED);
	AFSettingUtils::HookWidget(ui->spinBox_AdvRBMegsMax, this, SCROLL_CHANGED, OUTPUTS_CHANGED);

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
	
	connect(ui->comboBox_SettingMode, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotSettingModeCurrentIndexChanged);

	connect(ui->checkBox_AdvOutSplitFile, &QCheckBox::stateChanged, this, &AFQOutputSettingAreaWidget::qslotAdvOutSplitFileChanged);
	connect(ui->comboBox_AdvOutSplitFileType, &QComboBox::currentIndexChanged, this, &AFQOutputSettingAreaWidget::qslotAdvOutSplitFileChanged);
	connect(ui->checkBox_AdvOutSplitFile, &QCheckBox::toggled, ui->comboBox_AdvOutSplitFileType, &QComboBox::setEnabled);
	
	// Add warning checks to advanced output recording section controls
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
	// Advanced
	if (isAdvanced)
	{
		ui->comboBox_SettingMode->setCurrentIndex(1);

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
	// Simple
	else
	{
		ui->comboBox_SettingMode->setCurrentIndex(0);

		ui->widget_SimpleOutputSetting->setVisible(true);
		ui->widget_AdvOutputSetting->setVisible(false);
	}
}

void AFQOutputSettingAreaWidget::qslotSimpleRecordingEncoderChanged()
{
	QString qual = ui->comboBox_SimpleOutRecQuality->currentData().toString();
	//QString warning;
	
	//bool enforceBitrate = !ui->ignoreRecommended->isChecked();
	bool enforceBitrate = !config_get_bool(ACTIVECONFIG, "Stream1", "IgnoreRecommended");
	//OBSService service = GetStream1Service();

	//delete simpleOutRecWarning;

	//if (enforceBitrate && service) {
	//	OBSDataAutoRelease videoSettings = obs_data_create();
	//	OBSDataAutoRelease audioSettings = obs_data_create();
	//	int oldVBitrate = ui->simpleOutputVBitrate->value();
	//	int oldABitrate =
	//		ui->simpleOutputABitrate->currentText().toInt();
	//	obs_data_set_int(videoSettings, "bitrate", oldVBitrate);
	//	obs_data_set_int(audioSettings, "bitrate", oldABitrate);

	//	obs_service_apply_encoder_settings(service, videoSettings,
	//		audioSettings);

	//	int newVBitrate = obs_data_get_int(videoSettings, "bitrate");
	//	int newABitrate = obs_data_get_int(audioSettings, "bitrate");

	//	if (newVBitrate < oldVBitrate)
	//		warning = SIMPLE_OUTPUT_WARNING("VideoBitrate")
	//		.arg(newVBitrate);
	//	if (newABitrate < oldABitrate) {
	//		if (!warning.isEmpty())
	//			warning += "\n\n";
	//		warning += SIMPLE_OUTPUT_WARNING("AudioBitrate")
	//			.arg(newABitrate);
	//	}
	//}

	QString format = ui->comboBox_SimpleOutRecFormat->currentData().toString();
	/* Set tooltip if available */
	QString tooltip =
		QTStr("Basic.Settings.Output.Format.TT." + format.toUtf8());

	if (!tooltip.startsWith("Basic.Settings.Output"))
		ui->comboBox_SimpleOutRecFormat->setToolTip(tooltip);
	else
		ui->comboBox_SimpleOutRecFormat->setToolTip(nullptr);

	if (qual == "Lossless")
	{
		//if (!warning.isEmpty())
		//	warning += "\n\n";
		//warning += SIMPLE_OUTPUT_WARNING("Lossless");
		//warning += "\n\n";
		//warning += SIMPLE_OUTPUT_WARNING("Encoder");
	}
	else if (qual != "Stream") {
		QString enc = ui->comboBox_SimpleOutRecEncoder->currentData().toString();
		QString streamEnc = m_simpleOutVEncoder;
		bool x264RecEnc = (enc == SIMPLE_ENCODER_X264 ||
			enc == SIMPLE_ENCODER_X264_LOWCPU);

		//if (streamEnc == SIMPLE_ENCODER_X264 && x264RecEnc) {
		//	if (!warning.isEmpty())
		//		warning += "\n\n";
		//	warning += SIMPLE_OUTPUT_WARNING("Encoder");
		//}

		/* Prevent function being called recursively if changes happen. */
		ui->comboBox_SimpleOutRecEncoder->blockSignals(true);
		ui->comboBox_SimpleOutRecAEncoder->blockSignals(true);
		DisableIncompatibleSimpleCodecs(ui->comboBox_SimpleOutRecEncoder,
			format);
		DisableIncompatibleSimpleCodecs(ui->comboBox_SimpleOutRecAEncoder,
			format);
		ui->comboBox_SimpleOutRecAEncoder->blockSignals(false);
		ui->comboBox_SimpleOutRecEncoder->blockSignals(false);
		
		//if (ui->simpleOutRecEncoder->currentIndex() == -1 ||
		//	ui->simpleOutRecAEncoder->currentIndex() == -1) {
		//	if (!warning.isEmpty())
		//		warning += "\n\n";
		//	warning += QTStr("OutputWarnings.CodecIncompatible");
		//}
	}
	else {
		/* When using stream encoders do the reverse; Disable containers that are incompatible. */
		QString streamEnc = m_simpleOutVEncoder;
		QString streamAEnc = m_simpleOutAEncoder;

		if (streamEnc == "" || streamAEnc == "")
			return;

		ui->comboBox_SimpleOutRecFormat->blockSignals(true);
		DisableIncompatibleSimpleContainer(ui->comboBox_SimpleOutRecFormat, format, streamEnc, streamAEnc);
		ui->comboBox_SimpleOutRecFormat->blockSignals(false);

		//if (ui->simpleOutRecFormat->currentIndex() == -1) {
		//	if (!warning.isEmpty())
		//		warning += "\n\n";
		//	warning +=
		//		SIMPLE_OUTPUT_WARNING("IncompatibleContainer");
		//}

		//if (!warning.isEmpty())
		//	warning += "\n\n";
		//warning += SIMPLE_OUTPUT_WARNING("CannotPause");
	}

	if (qual != "Lossless" && (format == "mp4" || format == "mov")) {
		//ui->checkBox_AutoRemux->setText(
		//	QTStr("Basic.Settings.Advanced.AutoRemux").arg("mp4") +
		//	" " + QTStr("Basic.Settings.Advanced.AutoRemux.MP4"));
		emit qsignalSetAutoRemuxText(QTStr("Basic.Settings.Advanced.AutoRemux").arg("mp4") +
			" " + QTStr("Basic.Settings.Advanced.AutoRemux.MP4"));
	}
	else {
		//ui->checkBox_AutoRemux->setText(
		//	QTStr("Basic.Settings.Advanced.AutoRemux").arg("mp4"));
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

	//if (warning.isEmpty())
	//	return;

	//simpleOutRecWarning = new QLabel(warning, this);
	//simpleOutRecWarning->setObjectName("warningLabel");
	//simpleOutRecWarning->setWordWrap(true);
	//ui->simpleOutInfoLayout->addWidget(simpleOutRecWarning);
}

#define SIMPLE_OUTPUT_WARNING(str) \
	QTStr("Basic.Settings.Output.Simple.Warn." str)

void AFQOutputSettingAreaWidget::qslotSimpleRecordingQualityLosslessWarning(int idx)
{
	if (idx == m_lastSimpleRecQualityIdx || idx == -1)
		return;

	QString qual = ui->comboBox_SimpleOutRecQuality->itemData(idx).toString();

	if (m_loading) {
		m_lastSimpleRecQualityIdx = idx;
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
				Q_ARG(int, m_lastSimpleRecQualityIdx));
			return;
		}
	}

	m_lastSimpleRecQualityIdx = idx;
}

void AFQOutputSettingAreaWidget::qslotUpdateStreamDelayEstimate()
{
	qslotUpdateAutomaticReplayBufferCheckboxes();
}

void AFQOutputSettingAreaWidget::qslotUpdateAutomaticReplayBufferCheckboxes()
{
	bool state = false;

	if (!m_isAdvancedMode)
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
	auto activeConfig = ACTIVECONFIG;
	//
	const char* mode = config_get_string(activeConfig, "Output", "Mode");
	if (astrcmpi(mode, "Advanced") == 0)
		m_isAdvancedMode = true;
	else
		m_isAdvancedMode = false;
	
	const char* type = config_get_string(activeConfig, "AdvOut", "RecType");
	const char* recencoderType = config_get_string(activeConfig, "AdvOut", "RecAudioEncoder");
	const char* format = config_get_string(activeConfig, "AdvOut", "RecFormat2");
	const char* path = config_get_string(activeConfig, "AdvOut", "RecFilePath");
	bool noSpace = config_get_bool(activeConfig, "AdvOut", "RecFileNameWithoutSpace");
	bool rescale = config_get_bool(activeConfig, "AdvOut", "RecRescale");
	const char* rescaleRes = config_get_string(activeConfig, "AdvOut", "RecRescaleRes");
	const char* muxCustom = config_get_string(activeConfig, "AdvOut", "RecMuxerCustom");
	int tracks = config_get_int(activeConfig, "AdvOut", "RecTracks");
	int flvTrack = config_get_int(activeConfig, "AdvOut", "FLVTrack");
	bool splitFile = config_get_bool(activeConfig, "AdvOut", "RecSplitFile");
	const char* splitFileType = config_get_string(activeConfig, "AdvOut", "RecSplitFileType");
	int splitFileTime = config_get_int(activeConfig, "AdvOut", "RecSplitFileTime");
	int splitFileSize = config_get_int(activeConfig, "AdvOut", "RecSplitFileSize");
	int typeIndex = (astrcmpi(type, "FFmpeg") == 0) ? 1 : 0;
	
	ui->comboBox_AdvOutRecType->setCurrentIndex(typeIndex);
	ui->lineEdit_AdvOutRecPath->setText(path);
	ui->checkBox_AdvOutNoSpace->setChecked(noSpace);

	ui->checkBox_AdvOutRecUseRescale->setChecked(rescale);
	ui->comboBox_AdvOutRecRescale->setEnabled(rescale);
	ui->comboBox_AdvOutRecRescale->setCurrentText(rescaleRes);
	ui->lineEdit_AdvOutMuxCustom->setText(muxCustom);
	
	if (!AFSettingUtils::SetComboByValue(ui->comboBox_AdvOutRecAEncoder, recencoderType))
		ui->comboBox_AdvOutRecAEncoder->setCurrentIndex(-1);
	
	int idx = ui->comboBox_AdvOutRecFormat->findData(format);
	ui->comboBox_AdvOutRecFormat->setCurrentIndex(idx);

	ui->checkBox_AdvOutRecTrack1->setChecked(tracks & (1 << 0));
	ui->checkBox_AdvOutRecTrack2->setChecked(tracks & (1 << 1));
	ui->checkBox_AdvOutRecTrack3->setChecked(tracks & (1 << 2));
	ui->checkBox_AdvOutRecTrack4->setChecked(tracks & (1 << 3));
	ui->checkBox_AdvOutRecTrack5->setChecked(tracks & (1 << 4));
	ui->checkBox_AdvOutRecTrack6->setChecked(tracks & (1 << 5));

	// File split
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
		if (m_isAdvancedMode)
			emit qsignalAdvancedModeClicked();
		else
			emit qsignalSimpleModeClicked();
	}
}

#define AV_FORMAT_DEFAULT_STR QTStr("Basic.Settings.Output.Adv.FFmpeg.FormatDefault")
#define AUDIO_STR QTStr("Basic.Settings.Output.Adv.FFmpeg.FormatAudio")
#define VIDEO_STR QTStr("Basic.Settings.Output.Adv.FFmpeg.FormatVideo")

void AFQOutputSettingAreaWidget::_LoadFormats()
{
#define FORMAT_STR(str) QTStr("Basic.Settings.Output.Format." str)

	ui->comboBox_AdvOutFFFormat->blockSignals(true);

	m_formats = GetSupportedFormats();

	for (auto& format : m_formats) {
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
	ui->comboBox_SimpleOutRecFormat->addItem(FORMAT_STR("hMP4"), "hybrid_mp4");
	ui->comboBox_SimpleOutRecFormat->addItem(FORMAT_STR("fMP4"), "fragmented_mp4");
	ui->comboBox_SimpleOutRecFormat->addItem(FORMAT_STR("fMOV"), "fragmented_mov");
	ui->comboBox_SimpleOutRecFormat->addItem(FORMAT_STR("TS"), "mpegts");

	ui->comboBox_AdvOutRecFormat->addItem(FORMAT_STR("FLV"), "flv");
	ui->comboBox_AdvOutRecFormat->addItem(FORMAT_STR("MKV"), "mkv");
	ui->comboBox_AdvOutRecFormat->addItem(FORMAT_STR("MP4"), "mp4");
	ui->comboBox_AdvOutRecFormat->addItem(FORMAT_STR("MOV"), "mov");
	ui->comboBox_AdvOutRecFormat->addItem(FORMAT_STR("hMP4"), "hybrid_mp4");
	ui->comboBox_AdvOutRecFormat->addItem(FORMAT_STR("fMP4"), "fragmented_mp4");
	ui->comboBox_AdvOutRecFormat->addItem(FORMAT_STR("fMOV"), "fragmented_mov");
	ui->comboBox_AdvOutRecFormat->addItem(FORMAT_STR("TS"), "mpegts");
	ui->comboBox_AdvOutRecFormat->addItem(FORMAT_STR("HLS"), "hls");

#undef FORMAT_STR
}



static void AddDefaultCodec(QComboBox* combo, const FFmpegFormat& format, FFmpegCodecType codecType)
{

#define AV_ENCODER_DEFAULT_STR QTStr("Basic.Settings.Output.Adv.FFmpeg.AVEncoderDefault")
	FFmpegCodec codec = format.GetDefaultEncoder(codecType);

	int existingIdx = AFSettingUtils::FindEncoder(combo, codec.name, codec.id);
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


#define AV_ENCODER_DISABLE_STR QTStr("Basic.Settings.Output.Adv.FFmpeg.AVEncoderDisable")
void AFQOutputSettingAreaWidget::_ReloadCodecs(const FFmpegFormat& format)
{
	ui->comboBox_AdvOutFFAEncoder->blockSignals(true);
	ui->comboBox_AdvOutFFVEncoder->blockSignals(true);
	ui->comboBox_AdvOutFFAEncoder->clear();
	ui->comboBox_AdvOutFFVEncoder->clear();

	bool ignore_compatibility = ui->checkBox_AdvOutFFIgnoreCompat->isChecked();
	std::vector<FFmpegCodec> supportedCodecs = GetFormatCodecs(format, ignore_compatibility);
	for (auto& codec : supportedCodecs) {
		switch (codec.type) {
		case AUDIO:
			AFSettingUtils::AddCodec(ui->comboBox_AdvOutFFAEncoder, codec);
			break;
		case VIDEO:
			AFSettingUtils::AddCodec(ui->comboBox_AdvOutFFVEncoder, codec);
			break;
		default:
			break;
		}
	}

	if (format.HasAudio())
		AddDefaultCodec(ui->comboBox_AdvOutFFAEncoder, format, FFmpegCodecType::AUDIO);
	if (format.HasVideo())
		AddDefaultCodec(ui->comboBox_AdvOutFFVEncoder, format, FFmpegCodecType::VIDEO);

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
	QString dir = SelectDirectory(this, QTStr("Basic.Settings.Output.SelectDirectory"), ui->lineEdit_SimpleOutputPath->text());

	if (dir.isEmpty())
		return;

	ui->lineEdit_SimpleOutputPath->setText(dir);
}

void AFQOutputSettingAreaWidget::qslotAdvOutRecPathBrowseClicked()
{
	QString dir = SelectDirectory(
		this, 
		QTStr("Basic.Settings.Output.SelectDirectory"), 
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
	if (!m_loading)
	{
		m_outputChanged = true;
		sender()->setProperty("changed", QVariant(true));

		emit qsignalOutputDataChanged();
	}
}

void AFQOutputSettingAreaWidget::qslotOutputTypeChanged(int idx)
{
	if (!m_isAdvancedMode)
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
	bool rescale = config_get_bool(ACTIVECONFIG, "AdvOut", "FFRescale");

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
	auto activeConfig = ACTIVECONFIG;
	//
	bool saveFile = config_get_bool(activeConfig, "AdvOut", "FFOutputToFile");
	const char* path = config_get_string(activeConfig, "AdvOut", "FFFilePath");
	bool noSpace = config_get_bool(activeConfig, "AdvOut", "FFFileNameWithoutSpace");
	const char* url = config_get_string(activeConfig, "AdvOut", "FFURL");
	const char* format = config_get_string(activeConfig, "AdvOut", "FFFormat");
	const char* mimeType = config_get_string(activeConfig, "AdvOut", "FFFormatMimeType");
	const char* muxCustom = config_get_string(activeConfig, "AdvOut", "FFMCustom");
	int videoBitrate = config_get_int(activeConfig, "AdvOut", "FFVBitrate");
	int gopSize = config_get_int(activeConfig, "AdvOut", "FFVGOPSize");
	bool rescale = config_get_bool(activeConfig, "AdvOut", "FFRescale");
	bool codecCompat = config_get_bool(activeConfig, "AdvOut", "FFIgnoreCompat");
	const char* rescaleRes = config_get_string(activeConfig, "AdvOut", "FFRescaleRes");
	const char* vEncoder = config_get_string(activeConfig, "AdvOut", "FFVEncoder");
	int vEncoderId = config_get_int(activeConfig, "AdvOut", "FFVEncoderId");
	const char* vEncCustom = config_get_string(activeConfig, "AdvOut", "FFVCustom");
	const char* aEncoder = config_get_string(activeConfig, "AdvOut", "FFAEncoder");
	int aEncoderId = config_get_int(activeConfig, "AdvOut", "FFAEncoderId");
	const char* aEncCustom = config_get_string(activeConfig, "AdvOut", "FFACustom");
	int audioBitrate = config_get_int(activeConfig, "AdvOut", "FFABitrate");
	int audioMixes = config_get_int(activeConfig, "AdvOut", "FFAudioMixes");

	ui->comboBox_AdvOutFFType->setCurrentIndex(saveFile ? 0 : 1);
	ui->lineEdit_AdvOutFFRecPath->setText(QT_UTF8(path));
	ui->checkBox_AdvOutFFNoSpace->setChecked(noSpace);
	ui->lineEdit_AdvOutFFURL->setText(QT_UTF8(url));
	AFSettingUtils::SelectFormat(ui->comboBox_AdvOutFFFormat, format, mimeType);
	
	ui->lineEdit_AdvOutFFMCfg->setText(muxCustom);          
	ui->spinBox_AdvOutFFVBitrate->setValue(videoBitrate);
	ui->spinBox_AdvOutFFVGOPSize->setValue(gopSize);
	ui->checkBox_AdvOutFFIgnoreCompat->setChecked(codecCompat);
	ui->checkBox_AdvOutFFUseRescale->setChecked(rescale);
	ui->comboBox_AdvOutFFRescale->setEnabled(rescale);
	ui->comboBox_AdvOutFFRescale->setCurrentText(rescaleRes);
	AFSettingUtils::SelectEncoder(ui->comboBox_AdvOutFFVEncoder, vEncoder, vEncoderId);
	
	ui->lineEdit_AdvOutFFVCfg->setText(vEncCustom);
	ui->spinBox_AdvOutFFABitrate->setValue(audioBitrate);
	AFSettingUtils::SelectEncoder(ui->comboBox_AdvOutFFAEncoder, aEncoder, aEncoderId);
	
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

	/* ------------------------------------------------- */
	/* clear encoder lists                               */
	if (!streamOnly) {
		ui->comboBox_AdvOutRecEncoder->clear();
		ui->comboBox_AdvOutRecAEncoder->clear();
	}

	/* ------------------------------------------------- */
	/* load advanced stream/recording encoders           */
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
		ui->comboBox_AdvOutRecEncoder->insertItem(0, TEXT_USE_STREAM_ENC, "none");
		ui->comboBox_AdvOutRecAEncoder->model()->sort(0);
		ui->comboBox_AdvOutRecAEncoder->insertItem(0, TEXT_USE_STREAM_ENC, "none");
	}
}

void AFQOutputSettingAreaWidget::_LoadOutputRecordingEncoderProperties()
{
	const char* type = config_get_string(ACTIVECONFIG, "AdvOut", "RecEncoder");
	delete m_pRecordEncoderProps;
	m_pRecordEncoderProps = nullptr;
	if (astrcmpi(type, "none") != 0) {
		m_pRecordEncoderProps = _CreateEncoderPropertyView(type, "recordEncoder.json");
		m_pRecordEncoderProps->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
		
		//ui->advOutRecEncoderProps->layout()->addWidget(m_pRecordEncoderProps);
		ui->verticalLayout_RecEncoder->addWidget(m_pRecordEncoderProps);
		connect(m_pRecordEncoderProps, &OBSPropertiesView::Changed, this, &AFQOutputSettingAreaWidget::qslotAdvReplayBufferChanged);
	}
	m_curAdvRecordEncoder = type;

	if (!AFSettingUtils::SetComboByValue(ui->comboBox_AdvOutRecEncoder, type)) {
		uint32_t caps = obs_get_encoder_caps(type);
		if ((caps & ENCODER_HIDE_FLAGS) != 0) {
			const char* name = obs_encoder_get_display_name(type);

			ui->comboBox_AdvOutRecEncoder->insertItem(1, QT_UTF8(name), QT_UTF8(type));
			AFSettingUtils::SetComboByValue(ui->comboBox_AdvOutRecEncoder, type);
		}
		else {
			ui->comboBox_AdvOutRecEncoder->setCurrentIndex(-1);
		}
	}
}

void AFQOutputSettingAreaWidget::_LoadAdvReplayBuffer()
{
	auto activeConfig = ACTIVECONFIG;
	//
	bool replayBuf = config_get_bool(activeConfig, "AdvOut", "RecRB");
	int rbTime = config_get_int(activeConfig, "AdvOut", "RecRBTime");
	int rbSize = config_get_int(activeConfig, "AdvOut", "RecRBSize");

	ui->checkBox_AdvReplayBuf->setChecked(replayBuf);
	ui->spinBox_AdvRBSecMax->setValue(rbTime);
	ui->spinBox_AdvRBMegsMax->setValue(rbSize);
}

OBSPropertiesView* AFQOutputSettingAreaWidget::_CreateEncoderPropertyView(const char* encoder, const char* path, bool changed)
{
	OBSDataAutoRelease settings = obs_encoder_defaults(encoder);
	OBSPropertiesView* view;
	if (path) {
		char encoderJsonPath[512];
		int ret = GetProfilePath(encoderJsonPath, sizeof(encoderJsonPath), path);
		if (ret > 0) {
			obs_data_t* data = obs_data_create_from_json_file_safe(encoderJsonPath, "bak");
			obs_data_apply(settings, data);
			obs_data_release(data);
		}
	}

	view = new OBSPropertiesView(settings.Get(), encoder, (PropertiesReloadCallback)obs_get_encoder_properties, ui->label_AdvOutRecEncoder->maximumWidth());
	view->setLayoutMargin(0, 0, 0, 0);
	view->setFormSpacing(0, 16);
	view->setFrameShape(QFrame::NoFrame);
	view->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	view->setProperty("changed", QVariant(changed));
	view->ReloadProperties();
    
	QObject::connect(view, &OBSPropertiesView::Changed, this, &AFQOutputSettingAreaWidget::qslotOutputDataChanged);

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

	ui->comboBox_SimpleOutRecEncoder->addItem(ENCODER_STR("Software"), QString(SIMPLE_ENCODER_X264));
	ui->comboBox_SimpleOutRecEncoder->addItem(ENCODER_STR("SoftwareLowCPU"), QString(SIMPLE_ENCODER_X264_LOWCPU));
	if (AFEncoderUtil::EncoderAvailable("obs_qsv11"))
		ui->comboBox_SimpleOutRecEncoder->addItem(ENCODER_STR("Hardware.QSV.H264"), QString(SIMPLE_ENCODER_QSV));
	if (AFEncoderUtil::EncoderAvailable("obs_qsv11_av1"))
		ui->comboBox_SimpleOutRecEncoder->addItem(ENCODER_STR("Hardware.QSV.AV1"), QString(SIMPLE_ENCODER_QSV_AV1));
	if (AFEncoderUtil::EncoderAvailable("ffmpeg_nvenc"))
		ui->comboBox_SimpleOutRecEncoder->addItem(ENCODER_STR("Hardware.NVENC.H264"), QString(SIMPLE_ENCODER_NVENC));
	if (AFEncoderUtil::EncoderAvailable("obs_nvenc_av1_tex"))
		ui->comboBox_SimpleOutRecEncoder->addItem(ENCODER_STR("Hardware.NVENC.AV1"), QString(SIMPLE_ENCODER_NVENC_AV1));
#ifdef ENABLE_HEVC
	if (AFEncoderUtil::EncoderAvailable("h265_texture_amf"))
		ui->comboBox_SimpleOutRecEncoder->addItem(ENCODER_STR("Hardware.AMD.HEVC"), QString(SIMPLE_ENCODER_AMD_HEVC));
	if (AFEncoderUtil::EncoderAvailable("ffmpeg_hevc_nvenc"))
		ui->comboBox_SimpleOutRecEncoder->addItem(ENCODER_STR("Hardware.NVENC.HEVC"), QString(SIMPLE_ENCODER_NVENC_HEVC));
#endif // ENABLE_HEVC
	if (AFEncoderUtil::EncoderAvailable("h264_texture_amf"))
		ui->comboBox_SimpleOutRecEncoder->addItem(ENCODER_STR("Hardware.AMD.H264"), QString(SIMPLE_ENCODER_AMD));
	if (AFEncoderUtil::EncoderAvailable("av1_texture_amf"))
		ui->comboBox_SimpleOutRecEncoder->addItem(ENCODER_STR("Hardware.AMD.AV1"), QString(SIMPLE_ENCODER_AMD_AV1));
#ifdef __APPLE__
	if (AFEncoderUtil::EncoderAvailable("com.apple.videotoolbox.videoencoder.ave.avc")
#ifndef __aarch64__
		&& os_get_emulation_status() == true
#endif // __aarch64__
		)
		ui->comboBox_SimpleOutRecEncoder->addItem(ENCODER_STR("Hardware.Apple.H264"), QString(SIMPLE_ENCODER_APPLE_H264));
#ifdef ENABLE_HEVC
	if (AFEncoderUtil::EncoderAvailable("com.apple.videotoolbox.videoencoder.ave.hevc")
#ifndef __aarch64__
		&& os_get_emulation_status() == true
#endif // __aarch64__
		)
		ui->comboBox_SimpleOutRecEncoder->addItem(ENCODER_STR("Hardware.Apple.HEVC"), QString(SIMPLE_ENCODER_APPLE_HEVC));
#endif // ENABLE_HEVC
#endif // __APPLE__

	if (AFEncoderUtil::EncoderAvailable("CoreAudio_AAC") ||
		AFEncoderUtil::EncoderAvailable("libfdk_aac") || AFEncoderUtil::EncoderAvailable("ffmpeg_aac"))
		ui->comboBox_SimpleOutRecAEncoder->addItem(QTStr("Basic.Settings.Output.Simple.Codec.AAC.Default"), "aac");
	if (AFEncoderUtil::EncoderAvailable("ffmpeg_opus"))
		ui->comboBox_SimpleOutRecAEncoder->addItem(QTStr("Basic.Settings.Output.Simple.Codec.Opus"), "opus");

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
	uint32_t out_cx = m_outputCX;
	uint32_t out_cy = m_outputCY;

	advRecRescale = ui->comboBox_AdvOutRecRescale->lineEdit()->text();
	advFFRescale = ui->comboBox_AdvOutFFRescale->lineEdit()->text();

	//bool lockedOutputRes = !ui->outputResolution->isEditable();

	if (ignoreAllSignals) {
		//ui->advOutRescale->blockSignals(true);
		ui->comboBox_AdvOutRecRescale->blockSignals(true);
		ui->comboBox_AdvOutFFRescale->blockSignals(true);
	}
	//ui->advOutRescale->clear();
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

		string res = AFSettingUtils::ResString(downscaleCX, downscaleCY);
		string outRes = AFSettingUtils::ResString(outDownscaleCX, outDownscaleCY);
		//if (!lockedOutputRes)
		//	ui->outputResolution->addItem(res.c_str());
		
		//ui->advOutRecRescale->addItem(outRes.c_str());
		ui->comboBox_AdvOutRecRescale->addItem(outRes.c_str());
		ui->comboBox_AdvOutFFRescale->addItem(outRes.c_str());

		/* always try to find the closest output resolution to the
		 * previously set output resolution */
		int newPixelCount = int(downscaleCX * downscaleCY);
		int oldPixelCount = int(out_cx * out_cy);
		int diff = abs(newPixelCount - oldPixelCount);

		if (diff < bestPixelDiff) {
			bestScale = res;
			bestPixelDiff = diff;
		}
	}

	string res = AFSettingUtils::ResString(cx, cy);

	if (advRescale.isEmpty())
		advRescale = res.c_str();
	if (advRecRescale.isEmpty())
		advRecRescale = res.c_str();
	if (advFFRescale.isEmpty())
		advFFRescale = res.c_str();

	//ui->advOutRescale->lineEdit()->setText(advRescale);
	ui->comboBox_AdvOutRecRescale->lineEdit()->setText(advRecRescale);
	ui->comboBox_AdvOutFFRescale->lineEdit()->setText(advFFRescale);

	if (ignoreAllSignals) {
		//ui->advOutRescale->blockSignals(false);
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

	string res = AFSettingUtils::ResString(cx, cy);
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
	ui->widget_AdvReplayBuffer->setEnabled(useVideo);
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
	if (m_isAdvancedMode)
	{
		m_isAdvancedMode = false;
		_SetSettingModeUi(m_isAdvancedMode);

		m_outputChanged = true;
		emit qsignalOutputDataChanged();
	}
}

void AFQOutputSettingAreaWidget::ChangeSettingModeToAdvanced()
{
	if (!m_isAdvancedMode)
	{
		m_isAdvancedMode = true;
		_SetSettingModeUi(m_isAdvancedMode);

		m_outputChanged = true;
		emit qsignalOutputDataChanged();
	}
}

void AFQOutputSettingAreaWidget::qslotAdvReplayBufferChanged()
{
	obs_data_t* settings;
	QString encoder = ui->comboBox_AdvOutRecEncoder->currentText();
	bool useStream = QString::compare(encoder, TEXT_USE_STREAM_ENC) == 0;

	if (useStream && m_hasStreamEncoderData) {}
	else if (!useStream && m_pRecordEncoderProps) {
		settings = m_pRecordEncoderProps->GetSettings();
	}
	else {
		if (useStream)
			encoder = m_advOutVEncoder;
		settings = obs_encoder_defaults(encoder.toUtf8().constData());

		if (!settings)
			return;

		char encoderJsonPath[512];
		int ret = GetProfilePath(encoderJsonPath, sizeof(encoderJsonPath), "recordEncoder.json");
		if (ret > 0) {
			OBSDataAutoRelease data = obs_data_create_from_json_file_safe(encoderJsonPath, "bak");
			obs_data_apply(settings, data);
		}
	}

	int vbitrate; 
	const char* rateControl; 

	if (useStream && m_hasStreamEncoderData)
	{
		vbitrate = m_vbitrate;
		rateControl = m_rateControl;
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
		//abitrate += ui->advOutTrack1Bitrate->currentText().toInt();
		abitrate += m_simpleOutABitrate;
	if (ui->checkBox_AdvOutRecTrack2->isChecked())
		//abitrate += ui->advOutTrack2Bitrate->currentText().toInt();
		abitrate += m_simpleOutABitrate;
	if (ui->checkBox_AdvOutRecTrack3->isChecked())
		//abitrate += ui->advOutTrack3Bitrate->currentText().toInt();
		abitrate += m_simpleOutABitrate;
	if (ui->checkBox_AdvOutRecTrack4->isChecked())
		//abitrate += ui->advOutTrack4Bitrate->currentText().toInt();
		abitrate += m_simpleOutABitrate;
	if (ui->checkBox_AdvOutRecTrack5->isChecked())
		//abitrate += ui->advOutTrack5Bitrate->currentText().toInt();
		abitrate += m_simpleOutABitrate;

	// Set maximum to 75% of installed memory
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

	//ui->advRBEstimate->setObjectName("");
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
		ui->checkBox_AdvReplayBuf->setEnabled(false);
		ui->frame_AdvReplayBufferGroup->setEnabled(false);
		ui->frame_AdvReplayBufferGroup->setVisible(false);
	}
	else {
		ui->checkBox_AdvReplayBuf->setEnabled(true);
		ui->frame_AdvReplayBufferGroup->setEnabled(true);
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
									  const QString& formatName, const QString& streamEncoder)
{
	QString strEncLabel = QTStr("Basic.Settings.Output.Adv.Recording.UseStreamEncoder");
	QString recEncoder = cbox->currentData().toString();

	/* Check if selected encoders and output format are compatible, disable incompatible items. */
	bool currentCompatible = true;
	for (int idx = 0; idx < cbox->count(); idx++) {
		QString encName = cbox->itemData(idx).toString();
		string encoderId = (encName == "none") ? streamEncoder.toStdString() : encName.toStdString();
		QString encDisplayName = (encName == "none") ? strEncLabel
			: obs_encoder_get_display_name(encoderId.c_str());

		/* Something has gone horribly wrong and there's no encoder */
		if (encoderId.empty())
			continue;

		if(obs_get_encoder_caps(encoderId.c_str()) & OBS_ENCODER_CAP_DEPRECATED) {
			encDisplayName += " (" + QTStr("Deprecated") + ")";
		}

		const char* codec = obs_get_encoder_codec(encoderId.c_str());

		bool is_compatible = ContainerSupportsCodec(format.toStdString(), codec);
		/* Fall back to FFmpeg check if codec not one of the built-in ones. */
		if (!is_compatible && !IsBuiltinCodec(codec)) {
			string ext = GetFormatExt(QT_TO_UTF8(format));
			is_compatible = FFCodecAndFormatCompatible(codec, ext.c_str());
		}

		QStandardItemModel* model = dynamic_cast<QStandardItemModel*>(cbox->model());
		QStandardItem* item = model->item(idx);

		if (is_compatible) {
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		}
		else {
			if (recEncoder == encName)
				currentCompatible = false;

			item->setFlags(Qt::NoItemFlags);
			encDisplayName += " ";
			encDisplayName += QTStr("CodecCompat.Incompatible").arg(formatName);
		}

		item->setText(encDisplayName);
	}
	// Set to invalid entry if encoder was incompatible
	if (!currentCompatible)
		cbox->setCurrentIndex(-1);
}

void AFQOutputSettingAreaWidget::qslotAdvOutRecCheckCodecs()
{
	QString recFormat = ui->comboBox_AdvOutRecFormat->currentData().toString();
	QString recFormatName = ui->comboBox_AdvOutRecFormat->currentText();

	/* Set tooltip if available */
	QString tooltip = QTStr("Basic.Settings.Output.Format.TT." + recFormat.toUtf8());

	if (!tooltip.startsWith("Basic.Settings.Output"))
		ui->comboBox_AdvOutRecFormat->setToolTip(tooltip);
	else
		ui->comboBox_AdvOutRecFormat->setToolTip(nullptr);

	QString streamEncoder = m_advOutVEncoder;
	QString streamAudioEncoder = m_advOutAEncoder;

	if (streamEncoder == "" || streamAudioEncoder == "")
		return;

	int oldVEncoderIdx = ui->comboBox_AdvOutRecEncoder->currentIndex();
	int oldAEncoderIdx = ui->comboBox_AdvOutRecAEncoder->currentIndex();
	DisableIncompatibleCodecs(ui->comboBox_AdvOutRecEncoder, recFormat, recFormatName, streamEncoder);
	DisableIncompatibleCodecs(ui->comboBox_AdvOutRecAEncoder, recFormat, recFormatName, streamAudioEncoder);

	/* Only invoke AdvOutRecCheckWarnings() if it wouldn't already have
	 * been triggered by one of the encoder selections being reset. */
	if (ui->comboBox_AdvOutRecEncoder->currentIndex() == oldVEncoderIdx &&
		ui->comboBox_AdvOutRecAEncoder->currentIndex() == oldAEncoderIdx)
			qslotAdvOutRecCheckWarnings();
}

void AFQOutputSettingAreaWidget::qslotAdvOutRecEncoderCurrentIndexChanged(int idx) 
{
	if (!m_loading) {
		delete m_pRecordEncoderProps;
		m_pRecordEncoderProps = nullptr;
	}

	if (idx <= 0) {
		ui->widget_RecEncoder->setVisible(false);
		ui->checkBox_AdvOutRecUseRescale->setChecked(false);
		ui->widget_AdvOutRecUseRescale->setVisible(false);
		ui->comboBox_AdvOutRecRescale->setVisible(false);
		return;
	}

	QString encoder = AFSettingUtils::GetComboData(ui->comboBox_AdvOutRecEncoder);
	bool loadSettings = encoder == m_curAdvRecordEncoder;

	if (!m_loading) {
		m_pRecordEncoderProps = _CreateEncoderPropertyView(QT_TO_UTF8(encoder),
														   loadSettings ? "recordEncoder.json" : nullptr, true);
		m_pRecordEncoderProps->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
		ui->verticalLayout_RecEncoder->addWidget(m_pRecordEncoderProps);
		connect(m_pRecordEncoderProps, &OBSPropertiesView::Changed, this, &AFQOutputSettingAreaWidget::qslotAdvReplayBufferChanged);
	}

	ui->widget_RecEncoder->setVisible(true);
	ui->widget_AdvOutRecUseRescale->setVisible(true);
	ui->comboBox_AdvOutRecRescale->setVisible(true);
}

void AFQOutputSettingAreaWidget::qslotAdvOutFFIgnoreCompatStateChanged(int)
{
	/* Little hack to reload codecs when checked */
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
		AFSettingUtils::SelectEncoder(ui->comboBox_AdvOutFFAEncoder, defaultAudioCodecDesc.name,
									   defaultAudioCodecDesc.id);
		AFSettingUtils::SelectEncoder(ui->comboBox_AdvOutFFVEncoder, defaultVideoCodecDesc.name,
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

void AFQOutputSettingAreaWidget::qslotSettingModeCurrentIndexChanged(int idx)
{
	if (0 == idx)		// Simple
	{
		ChangeSettingModeToSimple();
		qslotUpdateStreamDelayEstimate();

		emit qsignalSimpleModeClicked();
	}
	else if (1 == idx)	// Advanced
	{
		ChangeSettingModeToAdvanced();
		qslotUpdateStreamDelayEstimate();

		emit qsignalAdvancedModeClicked();
	}
}

#if defined(__APPLE__) && QT_VERSION < QT_VERSION_CHECK(6, 5, 1)
// Workaround for QTBUG-56064 on macOS
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

    // Reset to "invalid" state if item was disabled
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
			errorMsg = QTStr("OutputWarnings.NoTracksSelected");
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
	// Workaround for QTBUG-56064 on macOS
	ResetInvalidSelection(ui->comboBox_AdvOutRecEncoder);
	ResetInvalidSelection(ui->comboBox_AdvOutRecAEncoder);
#endif

	// Show warning if codec selection was reset to an invalid state
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
		abitrate = m_simpleOutABitrate;
	}
	else {
		int delta = m_simpleOutABitrate;
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

	int vbitrate = m_simpleOutVBitrate;
	int seconds = ui->spinBox_SimpleRBSecMax->value();

	// Set maximum to 75% of installed memory
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

static void SaveTrackIndex(config_t* config, const char* section, const char* name,
						   QAbstractButton* check1, QAbstractButton* check2, QAbstractButton* check3,
						   QAbstractButton* check4, QAbstractButton* check5, QAbstractButton* check6)
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
	auto activeConfig = ACTIVECONFIG;
	//
	QVariant v = combo->currentData();
	if (!v.isNull()) {
		auto format = v.value<FFmpegFormat>();
		config_set_string(activeConfig, "AdvOut", "FFFormat", format.name);
		config_set_string(activeConfig, "AdvOut", "FFFormatMimeType", format.mime_type);

		const char* ext = format.extensions;
		string extStr = ext ? ext : "";

		char* comma = strchr(&extStr[0], ',');
		if (comma)
			*comma = 0;

		config_set_string(activeConfig, "AdvOut", "FFExtension", extStr.c_str());
	}
	else {
		config_set_string(activeConfig, "AdvOut", "FFFormat", nullptr);
		config_set_string(activeConfig, "AdvOut", "FFFormatMimeType", nullptr);
		config_remove_value(activeConfig, "AdvOut", "FFExtension");
	}
}

void AFQOutputSettingAreaWidget::SaveOutputEncoder(QComboBox* combo, const char* section, const char* value)
{
	QVariant v = combo->currentData();
	FFmpegCodec cd{};
	if (!v.isNull())
		cd = v.value<FFmpegCodec>();

	config_set_int(ACTIVECONFIG, section, QT_TO_UTF8(QString("%1Id").arg(value)), cd.id);
	if (cd.id != 0)
		config_set_string(ACTIVECONFIG, section, value, cd.name);
	else
		config_set_string(ACTIVECONFIG, section, value, nullptr);
}

void AFQOutputSettingAreaWidget::SaveOutputSettings()
{
	if (!m_outputChanged)
		return;

//	QString encoder = ui->simpleOutStrEncoder->currentData().toString();
//	const char* presetType;
//
//	if (encoder == SIMPLE_ENCODER_QSV)
//		presetType = "QSVPreset";
//	else if (encoder == SIMPLE_ENCODER_QSV_AV1)
//		presetType = "QSVPreset";
//	else if (encoder == SIMPLE_ENCODER_NVENC)
//		presetType = "NVENCPreset2";
//	else if (encoder == SIMPLE_ENCODER_NVENC_AV1)
//		presetType = "NVENCPreset2";
//#ifdef ENABLE_HEVC
//	else if (encoder == SIMPLE_ENCODER_AMD_HEVC)
//		presetType = "AMDPreset";
//	else if (encoder == SIMPLE_ENCODER_NVENC_HEVC)
//		presetType = "NVENCPreset2";
//#endif
//	else if (encoder == SIMPLE_ENCODER_AMD)
//		presetType = "AMDPreset";
//	else if (encoder == SIMPLE_ENCODER_AMD_AV1)
//		presetType = "AMDAV1Preset";
//	else if (encoder == SIMPLE_ENCODER_APPLE_H264
//#ifdef ENABLE_HEVC
//		|| encoder == SIMPLE_ENCODER_APPLE_HEVC
//#endif
//		)
//		/* The Apple encoders don't have presets like the other encoders
//		 do. This only exists to make sure that the x264 preset doesn't
//		 get overwritten with empty data. */
//		presetType = "ApplePreset";
//	else
//		presetType = "Preset";


	// [`OBS Output Streaming Settings]
	//curAdvStreamEncoder = GetComboData(ui->advOutEncoder);
	//AFSettingUtilsA::SaveComboData(ui->advOutEncoder, "AdvOut", "Encoder");
	//AFSettingUtilsA::SaveComboData(ui->advOutAEncoder, "AdvOut", "AudioEncoder");
	//AFSettingUtilsA::SaveCheckBox(ui->advOutUseRescale, "AdvOut", "Rescale");
	//AFSettingUtilsA::SaveCombo(ui->advOutRescale, "AdvOut", "RescaleRes");
	//SaveTrackIndex(basicConfig, "AdvOut", "TrackIndex", ui->advOutTrack1,
	//	ui->advOutTrack2, ui->advOutTrack3, ui->advOutTrack4,
	//	ui->advOutTrack5, ui->advOutTrack6);

	auto activeConfig = ACTIVECONFIG;
	//
	if(!m_isAdvancedMode)
		config_set_string(activeConfig, "Output", "Mode", "Simple");
	else
		config_set_string(activeConfig, "Output", "Mode", "Advanced");

	AFSettingUtils::SaveEdit(ui->lineEdit_SimpleOutputPath, "SimpleOutput", "FilePath");
	AFSettingUtils::SaveCheckBox(ui->checkBox_SimpleNoSpace, "SimpleOutput", "FileNameWithoutSpace");
	AFSettingUtils::SaveComboData(ui->comboBox_SimpleOutRecFormat, "SimpleOutput", "RecFormat2");
	AFSettingUtils::SaveComboData(ui->comboBox_SimpleOutRecQuality, "SimpleOutput", "RecQuality");
	AFSettingUtils::SaveComboData(ui->comboBox_SimpleOutRecEncoder, "SimpleOutput", "RecEncoder");
	AFSettingUtils::SaveComboData(ui->comboBox_SimpleOutRecAEncoder, "SimpleOutput", "RecAudioEncoder");
	AFSettingUtils::SaveEdit(ui->lineEdit_SimpleOutMuxCustom, "SimpleOutput", "MuxerCustom");
	AFSettingUtils::SaveCheckBox(ui->checkBox_SimpleReplayBuf, "SimpleOutput", "RecRB");
	AFSettingUtils::SaveSpinBox(ui->spinBox_SimpleRBSecMax, "SimpleOutput", "RecRBTime");
	AFSettingUtils::SaveSpinBox(ui->spinBox_SimpleRBMegsMax, "SimpleOutput", "RecRBSize");
	config_set_int(activeConfig, "SimpleOutput", "RecTracks", qslotSimpleOutGetSelectedAudioTracks());

	m_curAdvRecordEncoder = AFSettingUtils::GetComboData(ui->comboBox_AdvOutRecType);

	AFSettingUtils::SaveEdit(ui->lineEdit_AdvOutRecPath, "AdvOut", "RecFilePath");
	AFSettingUtils::SaveCheckBox(ui->checkBox_AdvOutNoSpace, "AdvOut", "RecFileNameWithoutSpace");
	AFSettingUtils::SaveComboData(ui->comboBox_AdvOutRecFormat, "AdvOut", "RecFormat2");
	AFSettingUtils::SaveComboData(ui->comboBox_AdvOutRecEncoder, "AdvOut", "RecEncoder");
	AFSettingUtils::SaveComboData(ui->comboBox_AdvOutRecAEncoder, "AdvOut", "RecAudioEncoder");
	AFSettingUtils::SaveCheckBox(ui->checkBox_AdvOutRecUseRescale, "AdvOut", "RecRescale");

	AFSettingUtils::SaveCombo(ui->comboBox_AdvOutRecRescale, "AdvOut", "RecRescaleRes");
	AFSettingUtils::SaveEdit(ui->lineEdit_AdvOutMuxCustom, "AdvOut", "RecMuxerCustom");
	AFSettingUtils::SaveCheckBox(ui->checkBox_AdvOutSplitFile, "AdvOut", "RecSplitFile");
	config_set_string(activeConfig, "AdvOut", "RecSplitFileType", SplitFileTypeFromIdx(ui->comboBox_AdvOutSplitFileType->currentIndex()));
	AFSettingUtils::SaveSpinBox(ui->spinBox_AdvOutSplitFileTime, "AdvOut", "RecSplitFileTime");
	AFSettingUtils::SaveSpinBox(ui->spinBox_AdvOutSplitFileSize, "AdvOut", "RecSplitFileSize");

	config_set_string(activeConfig, "AdvOut", "RecType", RecTypeFromIdx(ui->comboBox_AdvOutRecType->currentIndex()));
	config_set_int(activeConfig, "AdvOut", "RecTracks", qslotAdvOutGetSelectedAudioTracks()); // checkbox
	config_set_int(activeConfig, "AdvOut", "FLVTrack", _CurrentFLVTrack()); // radio

	// [`OBS Output Advanced FFmpeg Rec Settings]
	config_set_bool(activeConfig, "AdvOut", "FFOutputToFile", ui->comboBox_AdvOutFFType->currentIndex() == 0 ? true : false);
	AFSettingUtils::SaveEdit(ui->lineEdit_AdvOutFFRecPath, "AdvOut", "FFFilePath");
	AFSettingUtils::SaveCheckBox(ui->checkBox_AdvOutFFNoSpace, "AdvOut", "FFFileNameWithoutSpace");
	AFSettingUtils::SaveEdit(ui->lineEdit_AdvOutFFURL, "AdvOut", "FFURL");
	_SaveFormat(ui->comboBox_AdvOutFFFormat);
	AFSettingUtils::SaveEdit(ui->lineEdit_AdvOutFFMCfg, "AdvOut", "FFMCustom");
	AFSettingUtils::SaveSpinBox(ui->spinBox_AdvOutFFVBitrate, "AdvOut", "FFVBitrate");
	AFSettingUtils::SaveSpinBox(ui->spinBox_AdvOutFFVGOPSize, "AdvOut", "FFVGOPSize");
	AFSettingUtils::SaveCheckBox(ui->checkBox_AdvOutFFIgnoreCompat, "AdvOut", "FFIgnoreCompat");
	AFSettingUtils::SaveCheckBox(ui->checkBox_AdvOutFFUseRescale, "AdvOut", "FFRescale");
	AFSettingUtils::SaveCombo(ui->comboBox_AdvOutFFRescale, "AdvOut", "FFRescaleRes");
	_SaveEncoder(ui->comboBox_AdvOutFFVEncoder, "AdvOut", "FFVEncoder");
	AFSettingUtils::SaveEdit(ui->lineEdit_AdvOutFFVCfg, "AdvOut", "FFVCustom");
	AFSettingUtils::SaveSpinBox(ui->spinBox_AdvOutFFABitrate, "AdvOut", "FFABitrate");
	_SaveEncoder(ui->comboBox_AdvOutFFAEncoder, "AdvOut", "FFAEncoder");
	AFSettingUtils::SaveEdit(ui->lineEdit_AdvOutFFACfg, "AdvOut", "FFACustom");
	config_set_int(activeConfig, "AdvOut", "FFAudioMixes",
				   (ui->checkBox_AdvOutFFTrack1->isChecked() ? (1 << 0) : 0) |
				   (ui->checkBox_AdvOutFFTrack2->isChecked() ? (1 << 1) : 0) |
				   (ui->checkBox_AdvOutFFTrack3->isChecked() ? (1 << 2) : 0) |
				   (ui->checkBox_AdvOutFFTrack4->isChecked() ? (1 << 3) : 0) |
				   (ui->checkBox_AdvOutFFTrack5->isChecked() ? (1 << 4) : 0) |
				   (ui->checkBox_AdvOutFFTrack6->isChecked() ? (1 << 5) : 0));

	// [`Output Audio Settings Tab]
	//AFSettingUtilsA::SaveCombo(ui->advOutTrack1Bitrate, "AdvOut", "Track1Bitrate");
	//AFSettingUtilsA::SaveCombo(ui->advOutTrack2Bitrate, "AdvOut", "Track2Bitrate");
	//AFSettingUtilsA::SaveCombo(ui->advOutTrack3Bitrate, "AdvOut", "Track3Bitrate");
	//AFSettingUtilsA::SaveCombo(ui->advOutTrack4Bitrate, "AdvOut", "Track4Bitrate");
	//AFSettingUtilsA::SaveCombo(ui->advOutTrack5Bitrate, "AdvOut", "Track5Bitrate");
	//AFSettingUtilsA::SaveCombo(ui->advOutTrack6Bitrate, "AdvOut", "Track6Bitrate");
	//AFSettingUtilsA::SaveEdit(ui->advOutTrack1Name, "AdvOut", "Track1Name");
	//AFSettingUtilsA::SaveEdit(ui->advOutTrack2Name, "AdvOut", "Track2Name");
	//AFSettingUtilsA::SaveEdit(ui->advOutTrack3Name, "AdvOut", "Track3Name");
	//AFSettingUtilsA::SaveEdit(ui->advOutTrack4Name, "AdvOut", "Track4Name");
	//AFSettingUtilsA::SaveEdit(ui->advOutTrack5Name, "AdvOut", "Track5Name");
	//AFSettingUtilsA::SaveEdit(ui->advOutTrack6Name, "AdvOut", "Track6Name");

	//if (vodTrackCheckbox) {
	//	AFSettingUtilsA::SaveCheckBox(simpleVodTrack, "SimpleOutput", "VodTrackEnabled");
	//	AFSettingUtilsA::SaveCheckBox(vodTrackCheckbox, "AdvOut", "VodTrackEnabled");
	//	SaveTrackIndex(basicConfig, "AdvOut", "VodTrackIndex",
	//		vodTrack[0], vodTrack[1], vodTrack[2],
	//		vodTrack[3], vodTrack[4], vodTrack[5]);
	//}

	AFSettingUtils::SaveCheckBox(ui->checkBox_AdvReplayBuf, "AdvOut", "RecRB");

	if (!m_isAdvancedMode)
		MAINFRAME->EnableReplayBuffer(ui->checkBox_SimpleReplayBuf->isChecked());
	else {
		const char* advRecType = RecTypeFromIdx(ui->comboBox_AdvOutRecType->currentIndex());
		if(astrcmpi(advRecType, "FFmpeg") == 0)
			MAINFRAME->EnableReplayBuffer(false);
		else
			MAINFRAME->EnableReplayBuffer(ui->checkBox_AdvReplayBuf->isChecked());
	}


	AFSettingUtils::SaveSpinBox(ui->spinBox_AdvRBSecMax, "AdvOut", "RecRBTime");
	AFSettingUtils::SaveSpinBox(ui->spinBox_AdvRBMegsMax, "AdvOut", "RecRBSize");

	// WriteJsonData(streamEncoderProps, "streamEncoder.json");
	WriteJsonData(m_pRecordEncoderProps, "recordEncoder.json");
	// main->ResetOutputs();
}

void AFQOutputSettingAreaWidget::ResetOutputSettings()
{
	_ClearOutputSettingUi();
	LoadOutputSettings(true);
}

void AFQOutputSettingAreaWidget::AdvOutEncoderData(QString advOutVEncoder, QString advOutAEncoder)
{
	m_advOutVEncoder = advOutVEncoder;
	m_advOutAEncoder = advOutAEncoder;
	qslotAdvOutRecCheckCodecs();
}

void AFQOutputSettingAreaWidget::SimpleOutVEncoder(QString simpleOutVEncoder)
{
	m_simpleOutVEncoder = simpleOutVEncoder;

}
void AFQOutputSettingAreaWidget::SimpleOutAEncoder(QString simpleOutAEncoder)
{
	m_simpleOutAEncoder = simpleOutAEncoder;
}

void AFQOutputSettingAreaWidget::SimpleOutVBitrate(int simpleVBitrate)
{
	m_simpleOutVBitrate = simpleVBitrate;
	m_vbitrate = simpleVBitrate;	
}

void AFQOutputSettingAreaWidget::SimpleOutABitrate(int simpleABitrate)
{
	m_simpleOutABitrate = simpleABitrate;
}

void AFQOutputSettingAreaWidget::StreamEncoderData(int vbitrate, const char* rateControl)
{
	m_vbitrate = vbitrate;
	m_simpleOutVBitrate = vbitrate;
	m_rateControl = rateControl;
}

void AFQOutputSettingAreaWidget::OutputResolution(uint32_t cx, uint32_t cy)
{
	m_outputCX = cx;
	m_outputCY = cy;
}

void AFQOutputSettingAreaWidget::_SaveFormat(QComboBox* combo)
{
	auto activeConfig = ACTIVECONFIG;
	//
	QVariant v = combo->currentData();
	if (!v.isNull()) {
		auto format = v.value<FFmpegFormat>();
		config_set_string(activeConfig, "AdvOut", "FFFormat", format.name);
		config_set_string(activeConfig, "AdvOut", "FFFormatMimeType", format.mime_type);

		const char* ext = format.extensions;
		string extStr = ext ? ext : "";

		char* comma = strchr(&extStr[0], ',');
		if (comma)
			*comma = 0;

		config_set_string(activeConfig, "AdvOut", "FFExtension", extStr.c_str());
	}
	else {
		config_set_string(activeConfig, "AdvOut", "FFFormat", nullptr);
		config_set_string(activeConfig, "AdvOut", "FFFormatMimeType", nullptr);
		config_remove_value(activeConfig, "AdvOut", "FFExtension");
	}
}

void AFQOutputSettingAreaWidget::_SaveEncoder(QComboBox* combo, const char* section, const char* value)
{
	QVariant v = combo->currentData();
	FFmpegCodec cd{};
	if (!v.isNull())
		cd = v.value<FFmpegCodec>();

	config_set_int(ACTIVECONFIG, section, QT_TO_UTF8(QString("%1Id").arg(value)), cd.id);
	if (cd.id != 0)
		config_set_string(ACTIVECONFIG, section, value, cd.name);
	else
		config_set_string(ACTIVECONFIG, section, value, nullptr);
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
		QString qtranslate = QTStr(label->text().toUtf8().constData());
		label->setText(qtranslate);
	}

	foreach(QCheckBox * checkbox, checkboxList)
	{
		QString qtranslate = QTStr(checkbox->text().toUtf8().constData());
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
			QString qtranslate = QTStr(itemString.toUtf8().constData());
			combobox->setItemText(i, qtranslate);
		}
	}
}
