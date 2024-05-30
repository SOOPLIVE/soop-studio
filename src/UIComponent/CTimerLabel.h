#ifndef AFQTIMERLABEL_H
#define AFQTIMERLABEL_H

#include <QObject>
#include <QLabel>
#include <QPointer>
#include <QTimer>

class AFQTimerLabel : public QLabel
{
#pragma region QT Field
	Q_OBJECT
public:
	explicit AFQTimerLabel(QWidget* parent = nullptr);

public slots:
	void UpdateTime();

signals:

#pragma endregion QT Field

#pragma region public func
public:
	void StartCount();
	void StopCount();
	bool TimerStatus();
#pragma endregion public func

#pragma region protected func
protected:
#pragma endregion protected func

#pragma region private var
private:
	QPointer<QTimer> m_UpdateTimer;
	
	int m_Seconds = 0;

#pragma endregion private var
};

#endif // AFQTIMERLABEL_H
