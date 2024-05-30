#include "CAudioAdvSettingWidget.h"
#include "Application/CApplication.h"
#include "UIComponent/CItemWidgetHelper.h"
#include "CAudioAdvControl.h"
#include "qt-wrapper.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "ui_audio-adv-setting-area.h"

#pragma region class initializer, destructor
AFQAudioAdvSettingDialog::AFQAudioAdvSettingDialog(QWidget* parent)
	: AFQRoundedDialogBase(parent),
	ui(new Ui::AFQAudioAdvSettingDialog),
	m_sourceAddedSignal(obs_get_signal_handler(), "source_activate",
	_OBSSourceAdded, this),
	m_sourceRemovedSignal(obs_get_signal_handler(), "source_deactivate",
	_OBSSourceRemoved, this),
	m_bShowInactive(false)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);
	
	_ChangeLanguage();

	VolumeType volType = (VolumeType)config_get_int(
		AFConfigManager::GetSingletonInstance().GetGlobal(), "BasicWindow", "AdvAudioVolumeType");

	if (volType == VolumeType::Percent)
		ui->checkBox_UsePercent->setChecked(true);

	connect(ui->checkBox_UsePercent, &QCheckBox::clicked, this, &AFQAudioAdvSettingDialog::qslotOnUsePercentToggled);
	connect(ui->checkBox_ShowActiveOnly, &QCheckBox::clicked, this, &AFQAudioAdvSettingDialog::qslotOnActiveOnlyToggled);
	connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFQAudioAdvSettingDialog::qslotCloseButtonClicked);

	installEventFilter(CreateShortcutFilter());

	/* enum user scene/sources */
	obs_enum_sources(_EnumSources, this);
}

AFQAudioAdvSettingDialog::~AFQAudioAdvSettingDialog()
{
	for (size_t i = 0; i < m_vControls.size(); ++i)
		delete m_vControls[i];

	App()->GetMainView()->qslotSaveProject();
}
#pragma endregion class initializer, destructor

#pragma region private func
void AFQAudioAdvSettingDialog::_ChangeLanguage()
{
	QList<QLabel*> labelList = findChildren<QLabel*>();
	QList<QCheckBox*> checkboxList = findChildren<QCheckBox*>();

	foreach(QLabel * label, labelList)
	{
		label->setText(QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str(label->text().toUtf8().constData())));
	}

	foreach(QCheckBox * checkbox, checkboxList)
	{
		checkbox->setText(QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str(checkbox->text().toUtf8().constData())));
	}
}

inline void AFQAudioAdvSettingDialog::_AddAudioSource(obs_source_t* source)
{
	for (size_t i = 0; i < m_vControls.size(); i++) {
		if (m_vControls[i]->GetSource() == source)
			return;
	}

	AFQAdvAudioCtrl* control = new AFQAdvAudioCtrl(ui->gridLayout_AdvSettingArea, source);

	InsertQObjectByName(m_vControls, control);

	for (auto control : m_vControls) {
		control->ShowAudioControl(ui->gridLayout_AdvSettingArea);
	}
}

bool AFQAudioAdvSettingDialog::_EnumSources(void* param, obs_source_t* source)
{
	AFQAudioAdvSettingDialog* dialog = reinterpret_cast<AFQAudioAdvSettingDialog*>(param);
	uint32_t flags = obs_source_get_output_flags(source);

	if ((flags & OBS_SOURCE_AUDIO) != 0 &&
		(dialog->m_bShowInactive || obs_source_active(source)))
		dialog->_AddAudioSource(source);

	return true;
}

void AFQAudioAdvSettingDialog::_OBSSourceAdded(void* param, calldata_t* calldata)
{
	OBSSource source((obs_source_t*)calldata_ptr(calldata, "source"));

	QMetaObject::invokeMethod(reinterpret_cast<AFQAudioAdvSettingDialog*>(param),
		"SourceAdded", Q_ARG(OBSSource, source));
}

void AFQAudioAdvSettingDialog::_OBSSourceRemoved(void* param, calldata_t* calldata)
{
	OBSSource source((obs_source_t*)calldata_ptr(calldata, "source"));

	QMetaObject::invokeMethod(reinterpret_cast<AFQAudioAdvSettingDialog*>(param),
		"SourceRemoved", Q_ARG(OBSSource, source));
}
#pragma endregion private func

#pragma region QT Field
void AFQAudioAdvSettingDialog::qslotSourceAdded(OBSSource source)
{
	uint32_t flags = obs_source_get_output_flags(source);

	if ((flags & OBS_SOURCE_AUDIO) == 0)
		return;

	_AddAudioSource(source);
}

void AFQAudioAdvSettingDialog::qslotSourceRemoved(OBSSource source)
{
	uint32_t flags = obs_source_get_output_flags(source);

	if ((flags & OBS_SOURCE_AUDIO) == 0)
		return;

	for (size_t i = 0; i < m_vControls.size(); i++) {
		if (m_vControls[i]->GetSource() == source) {
			delete m_vControls[i];
			m_vControls.erase(m_vControls.begin() + i);
			break;
		}
	}
}

void AFQAudioAdvSettingDialog::qslotOnUsePercentToggled(bool checked)
{
	VolumeType type;

	if (checked)
		type = VolumeType::Percent;
	else
		type = VolumeType::dB;

	for (size_t i = 0; i < m_vControls.size(); i++)
		m_vControls[i]->SetVolumeWidget(type);

	config_set_int(AFConfigManager::GetSingletonInstance().GetGlobal(), "BasicWindow", "AdvAudioVolumeType",
		(int)type);
}

void AFQAudioAdvSettingDialog::qslotOnActiveOnlyToggled(bool checked)
{
	SetShowInactive(!checked);
}

void AFQAudioAdvSettingDialog::qslotCloseButtonClicked()
{
	close();
}
#pragma endregion QT Field

#pragma region public func
void AFQAudioAdvSettingDialog::SetShowInactive(bool show)
{
	if (m_bShowInactive == show)
		return;

	m_bShowInactive = show;

	m_sourceAddedSignal.Disconnect();
	m_sourceRemovedSignal.Disconnect();

	if (m_bShowInactive) {
		m_sourceAddedSignal.Connect(obs_get_signal_handler(),
			"source_create", _OBSSourceAdded,
			this);
		m_sourceRemovedSignal.Connect(obs_get_signal_handler(),
			"source_remove", _OBSSourceRemoved,
			this);

		obs_enum_sources(_EnumSources, this);
	}
	else {
		m_sourceAddedSignal.Connect(obs_get_signal_handler(),
			"source_activate", _OBSSourceAdded,
			this);
		m_sourceRemovedSignal.Connect(obs_get_signal_handler(),
			"source_deactivate",
			_OBSSourceRemoved, this);

		for (size_t i = 0; i < m_vControls.size(); i++) {
			const auto source = m_vControls[i]->GetSource();
			if (!obs_source_active(source)) {
				delete m_vControls[i];
				m_vControls.erase(m_vControls.begin() + i);
				i--;
			}
		}
	}
}
#pragma endregion public func
