#pragma once

#include "obs.hpp"
#include <string>

#include "UIComponent/CTopBaseWindow.h"
#include "CoreModel/OBSOutput/CVirtualCamDef.h"

namespace Ui {
	class AFQVirtualCamDialog;
};

class AFQVirtualCamDialog : public AFTTopBaseDialog
{
	Q_OBJECT

public:
	explicit AFQVirtualCamDialog(const VCamConfig& config, bool active, QWidget* parent = nullptr);
	~AFQVirtualCamDialog();

signals:
	void Accepted(const VCamConfig& config);
	void AcceptedAndRestart(const VCamConfig& config);

private slots:
	void qslotCloseButtonClicked();
	void qslotOutputChanged();
	void qslotUpdateConfig();

private:
	Ui::AFQVirtualCamDialog* ui = nullptr;

	VCamConfig m_config;
	VCamOutputType m_activeType;

	bool m_virtualCamActive = false;
	bool m_requireRestart = false;
};