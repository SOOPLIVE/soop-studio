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

class CMouseStatePreview final
{
public:
	CMouseStatePreview() = default;
	~CMouseStatePreview() = default;

public:
    // Preview Model Context State
    bool GetStateCropping() { return cropping; };
    void SetStateCropping(bool value) { cropping = value; };
    bool GetStateLocked() { return locked; };
    void SetStateLocked(bool value) { locked = value; };
    bool GetStateScrollMode() { return scrollMode; };
    void SetStateScrollMode(bool value) { scrollMode = value; };
    bool GetStateFixedScaling() { return fixedScaling; };
    void SetStateFixedScaling(bool value) { fixedScaling = value; };
    bool GetStateSelectionBox() { return selectionBox; };
    void SetStateSelectionBox(bool value) { selectionBox = value; };
      
    // Mouse State
	bool IsMouseDown() { return stateMouseDown; };
	void SetMouseDown() { stateMouseDown = true; };
	void ResetMouseDown() { stateMouseDown = false; };

	bool IsMouseMoved() { return stateMmouseMoved; };
	void SetMouseMoved() { stateMmouseMoved = true; };
	void ResetMouseMoved() { stateMmouseMoved = false; };

	bool IsMouseOverItems() { return stateMmouseOverItems; };
	void SetMouseOverItems() { stateMmouseOverItems = true; };
	void ResetMouseOverItems() { stateMmouseOverItems = false; };
 
    // Item(OBSource)
	ItemHandle GetCurrStateHandle () { return currItemsHandle; };
	void SetCurrStateHandle(ItemHandle value) { currItemsHandle = value; };

private:
    bool cropping = false;
    bool locked = false;
    bool scrollMode = false;
    bool fixedScaling = false;
    bool selectionBox = false;
      
	bool stateMouseDown = false;
	bool stateMmouseMoved = false;
	bool stateMmouseOverItems = false;

	ItemHandle currItemsHandle = ItemHandle::None;
};
