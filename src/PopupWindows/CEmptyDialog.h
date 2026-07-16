#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

#include <QDialog>

#include "UIComponent/CTopBaseWindow.h"

#include "obs.hpp"
#include "CoreModel/browser/CCefManager.h"


class OBSEventFilter;

namespace Ui {
	class AFQEmptyDialog;
}

class AFQEmptyDialog : public AFTTopBaseDialog
{
	Q_OBJECT

public:
	explicit AFQEmptyDialog(QWidget* parent);
	~AFQEmptyDialog();

public slots:
	void qslotQueryRecieved(const QCefQuery& query);

private slots:
	void qslotCloseButtonClicked();

public:
	void addWidget(QWidget* widget);
	void setTitle(QString title);

private:
	Ui::AFQEmptyDialog* ui;

};

