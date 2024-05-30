#ifndef CSTATFRAME_H
#define CSTATFRAME_H

#include <QPointer>
#include <QLabel>
#include "MainFrame/CMainBaseWidget.h"

namespace Ui {
class AFQStatWidget;
}

class AFStatistics;
enum class PCStatState;

class AFQStatWidget : public AFCQMainBaseWidget
{
#pragma region QT Field
	Q_OBJECT

signals:
	void qsignalStatMouseLeave();

public slots:
	void qslotReset();
	void qslotNetworkState(PCStatState state);
	void qslotCPUState(PCStatState state);
	void qslotDiskState(PCStatState state);
	void qslotMemoryState(PCStatState state);
	void qslotSkippedFrameState(PCStatState state);
	void qslotLaggedFrameState(PCStatState state);
#pragma endregion QT Field

#pragma region class initializer, destructor
public:
	explicit AFQStatWidget(QWidget* parent = nullptr, Qt::WindowFlags flag = Qt::FramelessWindowHint);
	~AFQStatWidget();
#pragma endregion class initializer, destructor

#pragma region public func
public:
	void UpdateStateIcon();
	void UpdateState();
#pragma endregion public func

#pragma region private func
private:
	void _ChangeLanguage();

	void _RefreshNetworkText();
	void _RefreshFPSText();
	void _RefreshCPUText();
	void _RefreshDiskText();
	void _RefreshMemoryText();
	void _RefreshRenderingAvgTimeText();
	void _RefreshSkippedFrameText();
	void _RefreshLaggedFrameText();
	void _RefreshStreamResourceText();
	void _RefreshRecResourceText();
#pragma endregion private func

#pragma region private var
private:
	Ui::AFQStatWidget* ui;
	AFStatistics* m_Statistics = nullptr;

	uint64_t m_streamLastBytesSent = 0;
	uint64_t m_streamLastBytesSentTime = 0;
	uint64_t m_recLastBytesSent = 0;
	uint64_t m_recLastBytesSentTime = 0;
#pragma endregion private var
};

#endif // CSTATFRAME_H
