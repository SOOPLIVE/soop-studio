#ifndef AFQPROXYSTYLE_H
#define AFQPROXYSTYLE_H

#include <QProxyStyle>

class AFQProxyStyle : public QProxyStyle
{
public:
	int styleHint(StyleHint hint, const QStyleOption* option, 
		const QWidget* widget, QStyleHintReturn* returnData) const override;
};

//Used in OBS ContextBar (Source Control bar)
class AFContextBarProxyStyle : public AFQProxyStyle {
public:
	QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap,
		const QStyleOption* option) const override;
};


#endif // AFQPROXYSTYLE_H
