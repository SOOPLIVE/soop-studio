#ifndef AFQTIMERLABEL_H
#define AFQTIMERLABEL_H

#include <QObject>
#include <QLabel>
#include <QPointer>
#include <QTimer>

#define CHECK_FIRST_BROAD_TIME 5 * 60
#define CHECK_AFTER_BROAD_TIME 30
#define CHECK_AI_MANAGERBROAD 15 * 60

class AFQTimerLabel : public QLabel
{
#pragma region QT Field
	Q_OBJECT
public:
	explicit AFQTimerLabel(QWidget* parent = nullptr);

public slots:
	void UpdateTime();
	void qslotVodSplitSaved();

signals:
	void qsignalCertainMinuteBroad(bool broad);
	void qsignalAIManagerCheckTime(bool start);

#pragma endregion QT Field

#pragma region public func
public:
	void StartCount();
	void StopCount();
	void ResetCetainTime();
	bool TimerStatus();
	bool IsSplitVodAvailable() { return m_certainMinutecheck; };
	void SetPrefix(QString prefix) { m_prefix = prefix; };
	bool AIManagerAble() { return m_aiManagerAble; };
	QString GetHHMMSS();
#pragma endregion public func

#pragma region protected func
protected:
#pragma endregion protected func

#pragma region private func
private:
	void _CertainTimeBroad(bool broad);
#pragma endregion private func

#pragma region private var
private:
	QPointer<QTimer> m_updateTimer;
	
	int m_broadTimeSeconds = 0;

	int m_certainTimeSeconds = 0;
	int m_certainCheckTime = 0;
	bool m_aiManagerAble = false;
	bool m_certainMinutecheck = false;
	bool m_firstBroadCheck = false;

	QString m_prefix = "";
#pragma endregion private var
};

#endif // AFQTIMERLABEL_H
