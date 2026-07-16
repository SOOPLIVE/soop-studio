//#include "plain-text-edit.hpp"
#include "moc_plain-text-edit.cpp"

#include <QFontDatabase>

OBSPlainTextEdit::OBSPlainTextEdit(QWidget *parent, bool monospace) : QPlainTextEdit(parent)
{
	// Fix display of tabs & multiple spaces
	document()->setDefaultStyleSheet("font { white-space: pre; }");

	if (monospace) {
		const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

		setStyleSheet(QString("font-family: %1; font-size: %2pt;")
				      .arg(fixedFont.family(), QString::number(fixedFont.pointSize())));
	}
}

void OBSPlainTextEdit::setMaxLength(int maxLen)
{
	maxLength = maxLen;
	connect(this, &QPlainTextEdit::textChanged, this, &OBSPlainTextEdit::qslotTextChanged);
}

void OBSPlainTextEdit::qslotTextChanged()
{
    if(inInternalChange)
        return;

    if(maxLength <= 0)
        return;

    QString text = toPlainText();
    int len = text.length();

    if(len > maxLength)
    {
        inInternalChange = true;

        text.truncate(maxLength);
        setPlainText(text);

        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::End);
        setTextCursor(cursor);

        inInternalChange = false;
    }
}