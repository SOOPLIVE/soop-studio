#ifndef CRESOURCEEXTENSION_H
#define CRESOURCEEXTENSION_H

#include <QWidget>
#include <QPointer>
#include <QTimer>

//#include "CoreModel/Statistics/CStatistics.h"

enum class PCStatState : int;

namespace Ui {
    class AFResourceExtension;
}

class AFResourceExtension : public QWidget
{
    Q_OBJECT

public:
    explicit AFResourceExtension(QWidget* parent = nullptr);
    ~AFResourceExtension();

signals:
    void qsignalCheckDiskSpaceRemaining(uint64_t num_bytes);

public slots:
    void qslotResourceUpdateTimerTick();
    void qslotCPUState(PCStatState state);
    void qslotDiskState(PCStatState state);
    void qslotMemoryState(PCStatState state);
    void qslotNetworkState(PCStatState state);
    void qslotFPSState(PCStatState state);

signals:
    void qsignalStatWindowTriggered();

public:
    void ResourceExtensionInit();

private:
    void _RefreshCPUText();
    void _RefreshDiskText();
    void _RefreshMemoryText();
    void _RefreshNetworkText();
    void _RefreshFPSText();

private:
    Ui::AFResourceExtension* ui = nullptr;
};

#endif // CRESOURCEEXTENSION_H
