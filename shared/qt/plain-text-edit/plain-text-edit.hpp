#pragma once

#include <QPlainTextEdit>

class OBSPlainTextEdit : public QPlainTextEdit {
	Q_OBJECT

public:
	explicit OBSPlainTextEdit(QWidget *parent = nullptr, bool monospace = false);

	void setMaxLength(int maxLen);

private slots:
	void qslotTextChanged();

private:
	// set Max Length
	int  maxLength = -1;
	bool inInternalChange = false;
};
