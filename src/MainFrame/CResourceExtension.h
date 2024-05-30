#ifndef CRESOURCEEXTENSION_H
#define CRESOURCEEXTENSION_H

#include <QWidget>
#include <QPointer>
#include <QTimer>

namespace Ui {
class AFResourceExtension;
}

class AFStatistics;
enum class PCStatState;

class AFResourceExtension : public QWidget
{
#pragma region QT Field
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

signals:
    void qsignalStatWindowTriggered();
    
#pragma endregion QT Field

#pragma region public func
public:
    void ResourceExtensionInit();
#pragma endregion public func

#pragma region protected func
protected:
#pragma endregion protected func

#pragma region private func
private:
    void _RefreshCPUText();
    void _RefreshDiskText();
    void _RefreshMemoryText();
    void _RefreshNetworkText();
#pragma endregion private func

#pragma region private member var
private:
    Ui::AFResourceExtension* ui;

    AFStatistics* m_Statistics = nullptr;
#pragma endregion private member var
};

#endif // CRESOURCEEXTENSION_H
