#pragma once

#include <QFontDialog>
#include "UIComponent/CTopBaseWindow.h"

namespace Ui {
	class AFQCustomFontDialog;
}

class AFQCustomFontDialog : public AFTTopBaseDialog
{
	Q_OBJECT

public:
	explicit AFQCustomFontDialog(bool* ok, const QFont& initial, const QString& title, QWidget* parent = nullptr,
								QFontDialog::FontDialogOptions options = QFontDialog::FontDialogOptions());
	~AFQCustomFontDialog();

public:
	static QFont getFont(bool* ok, const QFont& initial, QWidget* parent = nullptr, const QString& title = QString(),
						  QFontDialog::FontDialogOptions options = QFontDialog::FontDialogOptions());

private slots:
	void qslotAccept();
	void qslotReject();

private:
	Ui::AFQCustomFontDialog* ui;

	QFontDialog* m_pFontFrame = nullptr;
};
