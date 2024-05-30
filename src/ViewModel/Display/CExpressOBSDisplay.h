#pragma once

#include <obs.hpp>

class QWidget;

class AFExpressOBSDisplay final
{
#pragma region QT Field, CTOR/DTOR
public:
	AFExpressOBSDisplay(QWidget* pView);
	~AFExpressOBSDisplay();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
	inline obs_display_t*		GetOBSDisplay() const { return m_obsDisplay; }

	bool						CreateDisplay(uint32_t bgColor);
	bool						ResizeDisplay(uint32_t bgColor, uint32_t cx, uint32_t cy);
	void						ResetDisplay();
	void						DestroyDisplay();

	void						UpdateDisplayBackgroundColor(uint32_t bgColor);
	void						UpdateDisplayColorSpace();

#pragma endregion public func

#pragma region private func
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
private:
	bool						m_bDestroying = false;
	OBSDisplay					m_obsDisplay;


	QWidget*					m_pConnectedView = nullptr;

#pragma endregion private member var
};
