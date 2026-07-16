#pragma once

#include <QWidget>

class AFTransparentMouseEvents : public QWidget {
public:
	explicit AFTransparentMouseEvents(QWidget* parent = nullptr);
	~AFTransparentMouseEvents() {};

protected:
	bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

};
