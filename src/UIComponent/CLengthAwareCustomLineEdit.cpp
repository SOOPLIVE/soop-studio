#include "CLengthAwareCustomLineEdit.h"
#include <QStyle>

#include <QRegularExpression>

#pragma region FocusAwareLineEdit
AFQFocusAwareLineEdit::AFQFocusAwareLineEdit(QWidget* parent) 
	: AFQBasicLineEdit(parent) {}

AFQFocusAwareLineEdit::~AFQFocusAwareLineEdit() {}

void AFQFocusAwareLineEdit::focusInEvent(QFocusEvent* event)
{
	emit qsignalFocusChanged(true);
	AFQBasicLineEdit::focusInEvent(event);
}

void AFQFocusAwareLineEdit::focusOutEvent(QFocusEvent* event)
{
	emit qsignalFocusChanged(false);
	AFQBasicLineEdit::focusOutEvent(event);
}
#pragma endregion FocusAwareLineEdit



AFQLengthAwareLineEdit::AFQLengthAwareLineEdit(QWidget* parent)
	: QWidget(parent)
{
	setAttribute(Qt::WA_StyledBackground, true);

	_Init();
}

AFQLengthAwareLineEdit::~AFQLengthAwareLineEdit()
{
	delete m_hLayout;
	delete m_lineEdit;
	delete m_currentLengthLabel;
	delete m_maxLengthLabel;
}

void AFQLengthAwareLineEdit::_Init()
{
	// Layout
	m_hLayout = new QHBoxLayout();
	m_hLayout->setContentsMargins(QMargins(12, 2, 14, 2));
	m_hLayout->setSpacing(8);

	// LineEdit
	m_lineEdit = new AFQFocusAwareLineEdit(this);
	m_lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	QHBoxLayout* lengthLabelLayout = new QHBoxLayout();
	lengthLabelLayout->setContentsMargins(QMargins(0, 0, 0, 0));
	lengthLabelLayout->setSpacing(0);

	// Current Length Label
	m_currentLengthLabel = new QLabel(this);
	m_currentLengthLabel->setObjectName("label_CurrentLength");
	m_currentLengthLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

	// Max Length Label
	m_maxLengthLabel = new QLabel(this);
	m_maxLengthLabel->setObjectName("label_MaxLength");
	m_maxLengthLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

	lengthLabelLayout->addWidget(m_currentLengthLabel);
	lengthLabelLayout->addWidget(m_maxLengthLabel);

	// Add Widget
	m_hLayout->addWidget(m_lineEdit);
	m_hLayout->addLayout(lengthLabelLayout);
	this->setLayout(m_hLayout);

	connect(m_lineEdit, &AFQFocusAwareLineEdit::textChanged, this, &AFQLengthAwareLineEdit::qsignalTextChanged);
	connect(m_lineEdit, &AFQFocusAwareLineEdit::textChanged, this, &AFQLengthAwareLineEdit::_qslotUpdateCurrentTextLength);
	connect(m_lineEdit, &AFQFocusAwareLineEdit::editingFinished, this, &AFQLengthAwareLineEdit::qsignalEditFinished);
	connect(m_lineEdit, &AFQFocusAwareLineEdit::qsignalFocusChanged, this, &AFQLengthAwareLineEdit::_qslotLineEditFocusChanged);
	
	SetMaxLength(m_maxLength);
}

void AFQLengthAwareLineEdit::SetMaxLength(int maxLength)
{
	m_maxLength = maxLength;
	m_lineEdit->setMaxLength(m_maxLength);

	// Set Length Label Width
	QFontMetrics fontMetrics(m_maxLengthLabel->font());
	QString maxText = QString("/%1").arg(m_maxLength);
	
	int maxWidth = fontMetrics.horizontalAdvance(maxText);
	m_maxLengthLabel->setMinimumWidth(maxWidth);
	m_currentLengthLabel->setMinimumWidth(maxWidth);

	// Set Max Length Label Text
	m_maxLengthLabel->setText(maxText);

	// Init Current Length Label Text
	_qslotUpdateCurrentTextLength();
}

void AFQLengthAwareLineEdit::SetText(const QString& text)
{
	m_lineEdit->setText(text);
}

void AFQLengthAwareLineEdit::SetPlaceholderText(const QString& text)
{
	m_lineEdit->setPlaceholderText(text);
}

void AFQLengthAwareLineEdit::SetMaxLengthVisible(bool visible)
{
	m_maxLengthLabel->setVisible(visible);
	m_currentLengthLabel->setVisible(visible);
}

void AFQLengthAwareLineEdit::SelectAll()
{
	m_lineEdit->selectAll();
}

void AFQLengthAwareLineEdit::SetFocus()
{
	m_lineEdit->setFocus();
}

void AFQLengthAwareLineEdit::AllowOnlyAscii()
{
	m_lineEdit->AllowOnlyAscii();
}

QString AFQLengthAwareLineEdit::GetText()
{
	return m_lineEdit->text();
}

void AFQLengthAwareLineEdit::SetLineEditProperty(const char* propertyName, const QVariant& propertyValue)
{
	m_lineEdit->setProperty(propertyName, propertyValue);
}

void AFQLengthAwareLineEdit::PolishLineEditStyle()
{
	m_lineEdit->style()->unpolish(m_lineEdit);
	m_lineEdit->style()->polish(m_lineEdit);
}

void AFQLengthAwareLineEdit::SetUseBroadInfoDock(bool useBroadInfoDock)
{
	if (useBroadInfoDock)
	{
		if (m_hLayout)
		{
			m_hLayout->setContentsMargins(QMargins(0, 0, 0, 0));
			m_hLayout->setSpacing(0);
		}
	}
}

void AFQLengthAwareLineEdit::_qslotUpdateCurrentTextLength() 
{
	int currentLength = m_lineEdit->text().size();

	QString currentLengthText = QString::number(currentLength);
	m_currentLengthLabel->setText(currentLengthText);
}

void AFQLengthAwareLineEdit::_qslotLineEditFocusChanged(bool hasFocus) 
{
	// Update Style
	this->setProperty("focused", hasFocus);
	this->style()->unpolish(this);
	this->style()->polish(this);
}


#pragma region PrefixLengthAwareLineEdit
AFQPrefixLengthAwareLineEdit::AFQPrefixLengthAwareLineEdit(QWidget* parent) 
	: AFQLengthAwareLineEdit(parent)
{
	_InitPrefix();
}

AFQPrefixLengthAwareLineEdit::~AFQPrefixLengthAwareLineEdit()
{
	delete m_prefixLabel;
}

void AFQPrefixLengthAwareLineEdit::_InitPrefix()
{
	if (!m_hLayout)
		return;

	m_prefixLabel = new QLabel(this);
	m_prefixLabel->setObjectName("label_PrefixLineEdit");
	m_prefixLabel->setText("");
	m_prefixLabel->hide();

	// Add Label At The Beginning
	m_hLayout->insertWidget(0, m_prefixLabel);
}

void AFQPrefixLengthAwareLineEdit::qslotSetPrefixVisible(bool visible)
{
	if (!m_prefixLabel)
		return;

	m_prefixLabel->setVisible(visible);
}

void AFQPrefixLengthAwareLineEdit::SetPrefixText(const QString& text)
{
	if (!m_prefixLabel)
		return;

	m_prefixLabel->setText(text);
}

#pragma endregion PrefixLengthAwareLineEdit


#pragma region SecureLengthAwareLineEdit
AFQSecureLengthAwareLineEdit::AFQSecureLengthAwareLineEdit(QWidget* parent) 
{
	_InitHideText();
}

AFQSecureLengthAwareLineEdit::~AFQSecureLengthAwareLineEdit() 
{
	delete m_hideTextButton;
}

void AFQSecureLengthAwareLineEdit::qslotHideTextToggled()
{
	if (!m_lineEdit)
		return;

	if (m_hideTextButton->isChecked())
		m_lineEdit->setEchoMode(QLineEdit::Normal);
	else
		m_lineEdit->setEchoMode(QLineEdit::Password);
}

void AFQSecureLengthAwareLineEdit::SetHideTextButtonVisible(bool visible)
{
	if (!m_hideTextButton)
		return;

	m_hideTextButton->setVisible(visible);
}

void AFQSecureLengthAwareLineEdit::_InitHideText()
{
	if (!m_hLayout)
		return;

	m_hideTextButton = new QPushButton(this);
	m_hideTextButton->setObjectName("pushButton_HideText");
	m_hideTextButton->setCheckable(true);
	m_hideTextButton->setChecked(false);
	qslotHideTextToggled();

	connect(m_hideTextButton, &QPushButton::toggled, 
			this, &AFQSecureLengthAwareLineEdit::qslotHideTextToggled);
	
	m_hLayout->insertWidget(1, m_hideTextButton);
}
#pragma endregion SecureLengthAwareLineEdit

AFQBasicLineEdit::AFQBasicLineEdit(QWidget* parent)
{
	setAttribute(Qt::WA_InputMethodEnabled);
}

void AFQBasicLineEdit::_qslotFilterText(const QString& text)
{
	QString filteredText;
	for (QChar ch : text) {
		// ASCII Check
		if (ch.unicode() >= 32 && ch.unicode() <= 126) {
			filteredText.append(ch);
		}
	}

	if (text != filteredText) {
		setText(filteredText);
		emit qsignalAsciiFailed();
	}
}

void AFQBasicLineEdit::AllowOnlyAscii()
{
	connect(this, &QLineEdit::textChanged, this, &AFQBasicLineEdit::_qslotFilterText);
}

void AFQBasicLineEdit::RestorePreviousText(bool enable)
{
	m_restorePreviousText = enable;
}

void AFQBasicLineEdit::focusInEvent(QFocusEvent* event)
{
	previousText = this->text();
	QLineEdit::focusInEvent(event);
}

void AFQBasicLineEdit::focusOutEvent(QFocusEvent* event)
{
	QRegularExpression onlySpaces("^\\s*$");
	if (m_restorePreviousText && onlySpaces.match(this->text()).hasMatch() && !previousText.isEmpty())
		this->setText(previousText);

	QLineEdit::focusOutEvent(event);
}

void AFQBasicLineEdit::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) 
	{
		clearFocus();
		return;
	}
	else if (event->key() == Qt::Key_Escape)
	{
		setText(previousText);
		clearFocus();
		return;
	}
	QLineEdit::keyPressEvent(event);
}

#include "obs.hpp"
#include <QTextBoundaryFinder>

bool isEmojiGrapheme(const QString& s)
{
	if (s.isEmpty())
		return false;

	int i = 0;
	while (i < s.size()) {
		QChar ch = s.at(i);

		uint codepoint = ch.unicode();
		if (ch.isHighSurrogate() && i + 1 < s.size() && s.at(i + 1).isLowSurrogate()) {
			codepoint = QChar::surrogateToUcs4(ch, s.at(i + 1));
			i += 2;
		}
		else {
			i += 1;
		}

		// Check representative Unicode ranges for emojis
		if ((codepoint >= 0x1F300 && codepoint <= 0x1FAFF) ||   // 🧩 Misc Symbols and Pictographs, Supplemental Symbols, Emoji
			(codepoint >= 0x1F600 && codepoint <= 0x1F64F) ||   // 😀 Emoticons
			(codepoint >= 0x1F680 && codepoint <= 0x1F6FF) ||   // 🚗 Transport and Map
			(codepoint >= 0x2600 && codepoint <= 0x26FF) ||		// ☀ Misc symbols
			(codepoint >= 0x2700 && codepoint <= 0x27BF)) {		// ✂ Dingbats
			return true;
		}
	}
	return false;
}

void AFQBasicLineEdit::inputMethodEvent(QInputMethodEvent* event)
{
	QString preedit = event->preeditString();
	QString commit = event->commitString();

	// default input
	QLineEdit::inputMethodEvent(event);

	if (!commit.isEmpty()) {
		m_lastPreedit.clear();
		return;
	}

	if (!preedit.isEmpty()) {
		m_lastPreedit = preedit;
		return;
	}

	// preedit="", commit="" 
	if (!m_lastPreedit.isEmpty() && isEmojiGrapheme(m_lastPreedit)) {
		QLineEdit::insert(m_lastPreedit);
	}
}
