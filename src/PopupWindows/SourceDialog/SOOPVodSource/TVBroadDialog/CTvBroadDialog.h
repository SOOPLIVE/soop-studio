#pragma once

#include "obs.hpp"

#include <vector>
#include <string>

#include <QTimer>

#include "UIComponent/CTopBaseWindow.h"

class AFQTvBroadItem;

namespace Ui {
	class AFQTvBroadDialog;
}

class AFQTvBroadDialog : public AFTTopBaseDialog
{
	Q_OBJECT
public:
	AFQTvBroadDialog(QWidget* parent, obs_source_t* source);
	~AFQTvBroadDialog();

private slots:
	void _qslotCloseButtonClicked();
	void _qslotTvLiveGeoBlockCheckAPIResponse(const QByteArray& responseData, int cpNo, int categoryNo);
public slots:
	void qslotResponseTvLiveOnetimeUrl(int cpNo);
	void qslotTvLiveItemClicked(int cpNo);
	void qslotRequestStopTvBroad(int cpNo);

protected:
	virtual void showEvent(QShowEvent* event) override;
private:
	static void _SourceRemoved(void* data, calldata_t* params);
private:
	Ui::AFQTvBroadDialog* ui;

	std::vector<QPointer<AFQTvBroadItem>> m_tvBroadItems;
	OBSSignal removeSignal;
};
