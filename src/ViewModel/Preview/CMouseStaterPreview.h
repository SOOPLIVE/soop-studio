#pragma once


#include <stdint.h>

#define ITEM_LEFT (1 << 0)
#define ITEM_RIGHT (1 << 1)
#define ITEM_TOP (1 << 2)
#define ITEM_BOTTOM (1 << 3)
#define ITEM_ROT (1 << 4)

enum class ItemHandle : uint32_t {
	None = 0,
	TopLeft = ITEM_TOP | ITEM_LEFT,
	TopCenter = ITEM_TOP,
	TopRight = ITEM_TOP | ITEM_RIGHT,
	CenterLeft = ITEM_LEFT,
	CenterRight = ITEM_RIGHT,
	BottomLeft = ITEM_BOTTOM | ITEM_LEFT,
	BottomCenter = ITEM_BOTTOM,
	BottomRight = ITEM_BOTTOM | ITEM_RIGHT,
	Rot = ITEM_ROT
};


class AFMouseStaterPreview final
{
#pragma region QT Field, CTOR/DTOR
public:
	AFMouseStaterPreview() = default;
	~AFMouseStaterPreview() = default;
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    // Preview Model Context State
    bool            GetStateCropping() { return m_bCropping; };
    void            SetStateCropping(bool value) { m_bCropping = value; };
    bool            GetStateLocked() { return m_bLocked; };
    void            SetStateLocked(bool value) { m_bLocked = value; };
    bool            GetStateScrollMode() { return m_bScrollMode; };
    void            SetStateScrollMode(bool value) { m_bScrollMode = value; };
    bool            GetStateFixedScaling() { return m_bFixedScaling; };
    void            SetStateFixedScaling(bool value) { m_bFixedScaling = value; };
    bool            GetStateSelectionBox() { return m_bSelectionBox; };
    void            SetStateSelectionBox(bool value) { m_bSelectionBox = value; };
    
    
    // Mouse State
	bool			IsMouseDown() { return m_stateMouseDown; };
	void			SetMouseDown() { m_stateMouseDown = true; };
	void			ResetMouseDown() { m_stateMouseDown = false; };

	bool			IsMouseMoved() { return m_stateMmouseMoved; };
	void			SetMouseMoved() { m_stateMmouseMoved = true; };
	void			ResetMouseMoved() { m_stateMmouseMoved = false; };

	bool			IsMouseOverItems() { return m_stateMmouseOverItems; };
	void			SetMouseOverItems() { m_stateMmouseOverItems = true; };
	void			ResetMouseOverItems() { m_stateMmouseOverItems = false; };

    
    // Item(OBSource)
	ItemHandle		GetCurrStateHandle () { return m_currItemsHandle; };
	void			SetCurrStateHandle(ItemHandle value) { m_currItemsHandle = value; };
#pragma endregion public func

#pragma region private func
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
private:
    bool                m_bCropping = false;
    bool                m_bLocked = false;
    bool                m_bScrollMode = false;
    bool                m_bFixedScaling = false;
    bool                m_bSelectionBox = false;
    
    
	bool				m_stateMouseDown = false;
	bool				m_stateMmouseMoved = false;
	bool				m_stateMmouseOverItems = false;

	ItemHandle			m_currItemsHandle = ItemHandle::None;   // obs -> stretchHandle
#pragma endregion private member var
};
