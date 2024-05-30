#pragma once

#include <QMainWindow>

//#include <util/config-file.h>

class AFQMainWindow : public QMainWindow {
	Q_OBJECT

public:
	inline AFQMainWindow(QWidget* parent) : QMainWindow(parent)
	{
		this->setMouseTracking(true);
		this->setAttribute(Qt::WA_Hover);
		this->installEventFilter(this);
	}

	//virtual config_t* Config() const = 0;
	virtual bool MainWindowInit() = 0;

	/*virtual int GetProfilePath(char* path, size_t size,
		const char* file) const = 0;*/
};
