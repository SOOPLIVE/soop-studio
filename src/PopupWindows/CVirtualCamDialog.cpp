
#include "CVirtualCamDialog.h"
#include "ui_virtualcam-dialog.h"

#include <util/util.hpp>
#include <util/platform.h>

#include <QStandardItem>

#include "Application/CApplication.h"


AFQVirtualCamDialog::AFQVirtualCamDialog(const VCamConfig& config, bool active, QWidget* parent)
	:AFTTopBaseDialog(parent),
	ui(new Ui::AFQVirtualCamDialog),

	m_config(config),
	m_virtualCamActive(active),
	m_activeType(config.type)
{
	ui->setupUi(this);

#ifdef __APPLE__
	setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
	setWindowTitle(QTStr("Basic.MainMenu.Tools.VirtualCamera"));
	ui->titleFrame->hide();
#endif

	SetWidthResizeEnabled(false);
	SetHeightResizeEnabled(false);

	ui->outputType->addItem(QTStr("Basic.VCam.OutputType.Program"), (int)VCamOutputType::ProgramView);
	ui->outputType->addItem(QTStr("Basic.VCam.OutputType.StudioMode"), (int)VCamOutputType::PreviewOutput);
	ui->outputType->addItem(QTStr("Basic.Scene"), (int)VCamOutputType::SceneOutput);
	ui->outputType->addItem(QTStr("Basic.Main.Source"), (int)VCamOutputType::SourceOutput);

	ui->outputType->setCurrentIndex(ui->outputType->findData((int)config.type));
	ui->warningLabel->setVisible(false);

    ui->pushButton_Ok->setProperty("buttonType", "coloredButton");

	qslotOutputChanged();

	connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFQVirtualCamDialog::qslotCloseButtonClicked);
	connect(ui->outputType, &QComboBox::currentIndexChanged, this, &AFQVirtualCamDialog::qslotOutputChanged);

	connect(ui->pushButton_Ok, &QPushButton::clicked, this, &AFQVirtualCamDialog::qslotUpdateConfig);
	connect(ui->pushButton_Cancel, &QPushButton::clicked, this, &AFQVirtualCamDialog::qslotCloseButtonClicked);
}
AFQVirtualCamDialog::~AFQVirtualCamDialog()
{
	delete ui;
}
// slots
void AFQVirtualCamDialog::qslotCloseButtonClicked()
{
	close();
}
void AFQVirtualCamDialog::qslotOutputChanged()
{
	VCamOutputType type = (VCamOutputType)ui->outputType->currentData().toInt();
	ui->outputSelection->setDisabled(false);

	auto list = ui->outputSelection;
	list->clear();

	switch(type) {
		case VCamOutputType::Invalid:
		case VCamOutputType::ProgramView:
		case VCamOutputType::PreviewOutput:
			ui->outputSelection->setDisabled(true);
			list->addItem(QTStr("Basic.VCam.OutputSelection.NoSelection"));
			break;
		case VCamOutputType::SceneOutput: {
			// Scenes in default order
			BPtr<char*> scenes = obs_frontend_get_scene_names();
			for(char** temp = scenes; *temp; temp++) {
				list->addItem(*temp);

				if(m_config.scene.compare(*temp) == 0)
					list->setCurrentIndex(list->count() - 1);
			}
			break;
		}
		case VCamOutputType::SourceOutput: {
			// Sources in alphabetical order
			std::vector<std::string> sources;
			auto AddSource = [&](obs_source_t* source) {
				auto name = obs_source_get_name(source);

				if(!(obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO))
					return;

				sources.push_back(name);
			};
			using AddSource_t = decltype(AddSource);

			obs_enum_sources([](void* data, obs_source_t* source) {
				auto& AddSource = *static_cast<AddSource_t*>(data);
				if(!obs_source_removed(source))
					AddSource(source);
				return true;
			}, static_cast<void*>(&AddSource));

			// Sort and select current item
			sort(sources.begin(), sources.end());
			for(auto&& source : sources) {
				list->addItem(source.c_str());

				if(m_config.source == source)
					list->setCurrentIndex(list->count() - 1);
			}
			break;
		}
	}

	if(!m_virtualCamActive)
		return;

    m_requireRestart = (m_activeType == VCamOutputType::ProgramView &&
						type != VCamOutputType::ProgramView) ||
					   (m_activeType != VCamOutputType::ProgramView &&
						type == VCamOutputType::ProgramView);

	ui->warningLabel->setVisible(m_requireRestart);
}
void AFQVirtualCamDialog::qslotUpdateConfig()
{
	VCamOutputType type = (VCamOutputType)ui->outputType->currentData().toInt();
	switch(type) {
		case VCamOutputType::ProgramView:
		case VCamOutputType::PreviewOutput:
			break;
		case VCamOutputType::SceneOutput:
			m_config.scene = ui->outputSelection->currentText().toStdString();
			break;
		case VCamOutputType::SourceOutput:
			m_config.source = ui->outputSelection->currentText().toStdString();
			break;
		default:
			// unknown value, don't save type
			return;
	}

	m_config.type = type;
	if(m_requireRestart) {
		emit AcceptedAndRestart(m_config);
	} else {
		emit Accepted(m_config);
	}
	close();
}
//