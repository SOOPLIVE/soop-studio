#include "CTransparentMouseEvents.h"

#ifdef _WIN32
#include <Windows.h>
#endif

AFTransparentMouseEvents::AFTransparentMouseEvents(QWidget* parent)
	: QWidget(parent)
{
#ifdef _WIN32
	setAttribute(Qt::WA_NativeWindow);
	setAttribute(Qt::WA_DeleteOnClose);
#endif
}

bool AFTransparentMouseEvents::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
#ifdef _WIN32
	PMSG msg = (PMSG)message;
	switch (msg->message) {
	case WM_NCHITTEST:
		*result = HTTRANSPARENT;
		return true;
		break;
	default:
		break;
	}
#endif
	return false;
}
