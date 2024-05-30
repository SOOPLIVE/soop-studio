#pragma once

#include <QTimer>
#include <QScrollArea>

class AFQListViewScrollArea : public QScrollArea
{
	Q_OBJECT

public:
	explicit AFQListViewScrollArea(QWidget* parent = nullptr);
	~AFQListViewScrollArea();

	void ShowScrollBar();

private slots:
	void qSlotHideScrollBar();

protected:
	void resizeEvent(QResizeEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	QTimer* m_timerScrollVisible = nullptr;
};