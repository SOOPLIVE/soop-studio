#pragma once

#include <QColorDialog>
#include "UIComponent/CTopBaseWindow.h"

namespace Ui {
	class AFQCustomColorDialog;
}

class AFQCustomColorDialog : public AFTTopBaseDialog
{
	Q_OBJECT

public:
	explicit AFQCustomColorDialog(const QColor& initial, const QString& title, QWidget* parent = nullptr);
	~AFQCustomColorDialog();

public:
	static QColor getColor(const QColor& initial, QWidget* parent,
						   const QString& title, QColorDialog::ColorDialogOptions options);

	void setOptions(QColorDialog::ColorDialogOptions options);

signals:
	void qsignalCurrentColorChanged(const QColor& color);
	void qsignalColorSelected(const QColor& color);
	void qsignalReject();

private slots:
	void qslotAccept();
	void qslotReject();
	void qslotCurrentColorChanged(const QColor& color);
	void qslotColorSelected(const QColor& color);

private:
	Ui::AFQCustomColorDialog* ui;

	QColorDialog* m_pColorPicker = nullptr;
};
