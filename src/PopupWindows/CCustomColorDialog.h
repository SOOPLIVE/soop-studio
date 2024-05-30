#pragma once

#include <QColorDialog>
#include "UIComponent/CRoundedDialogBase.h"

namespace Ui {
	class AFQCustomColorDialog;
}

class AFQCustomColorDialog : public AFQRoundedDialogBase
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
	void qSignalCurrentColorChanged(const QColor& color);
	void qSignalColorSelected(const QColor& color);
	void qSignalReject();

private slots:
	void qSlotAccept();
	void qSlotReject();
	void qSlotCurrentColorChanged(const QColor& color);
	void qSlotColorSelected(const QColor& color);

private:
	Ui::AFQCustomColorDialog* ui;

	QColorDialog* m_colorPicker = nullptr;
};