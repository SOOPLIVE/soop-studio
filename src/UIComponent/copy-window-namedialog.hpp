// [copy-obs]



#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <string>
#include <memory>

class NameDialog : public QDialog {
	Q_OBJECT

public:
	NameDialog(QWidget *parent);

	// Returns true if user clicks OK, false otherwise
	// userTextInput returns string that user typed into dialog
	static bool AskForName(QWidget *parent, const QString &title,
			       const QString &text, std::string &userTextInput,
			       const QString &placeHolder = QString(""),
			       int maxSize = 170);

	// Returns true if user clicks OK, false otherwise
	// userTextInput returns string that user typed into dialog
	// userOptionReturn the checkbox was ticked user accepted
	static bool
	AskForNameWithOption(QWidget *parent, const QString &title,
			     const QString &text, std::string &userTextInput,
			     const QString &optionLabel, bool &optionChecked,
			     const QString &placeHolder = QString(""));

private:
	QLabel *label;
	QLineEdit *userText;
	QCheckBox *checkbox;
};
