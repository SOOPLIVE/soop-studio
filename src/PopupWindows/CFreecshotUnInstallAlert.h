#ifndef CFREECSHOTUNINSTALLALERT_H
#define CFREECSHOTUNINSTALLALERT_H

#include <QDialog>
#include <QLabel>

#include "UIComponent/CTopBaseWindow.h"

namespace Ui {
class AFQFreecshotUninstallAlert;
}

class AFQFreecshotUninstallAlert : public AFTTopBaseDialog
{
	Q_OBJECT

public:
	explicit AFQFreecshotUninstallAlert(QWidget* parent = nullptr);
	~AFQFreecshotUninstallAlert();

signals:
	void qsignalninstallFreecshotAccept();

private slots:
	void qslotCancelClickedButton();
	void qslotAcceptClickedButton();

private:
	Ui::AFQFreecshotUninstallAlert* ui;
};

#endif // CFREECSHOTUNINSTALLALERT_H
