#pragma once

#include <QPlainTextEdit>

class AFQPlainTextEdit : public QPlainTextEdit {
	Q_OBJECT

public:
	explicit AFQPlainTextEdit(QWidget* parent = nullptr,
		bool monospace = true);
};
