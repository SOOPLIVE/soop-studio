#pragma once

#include "CQtDisplay.h"

#include <QPointer>
#include <QTimer>

#include "ViewModel/Preview/CMouseStaterPreview.h"
#include "ViewModel/Preview/CDrawLinePreview.h"
#include "ViewModel/Preview/CModelPreview.h"

class AFGraphicsContext;

class CBasicPreview : public AFQTDisplay
{
	Q_OBJECT

public:
	CBasicPreview(QWidget* parent,
			Qt::WindowFlags flags = Qt::WindowFlags());
	~CBasicPreview();

	static CBasicPreview* Get();

private slots:
	void ShowCustomContextMenu(const QPoint& pos);

public:
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;
    
    virtual void wheelEvent(QWheelEvent *event) override;
    
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void leaveEvent(QEvent* event) override;

	void UpdateCursor(uint32_t& flags);

	void DrawOverflow();
	void DrawSceneEditing(float dpiValue = 1.f);

	void DrawSpacingHelpers(float dpiValue = 1.f);
	void SetSourceBorderColor();

    void InitSetNugeEventAction();

    // move ModelPreview?
	inline void SetLocked(bool newLockedVal) { locked = newLockedVal; }
	inline bool GetLocked() { return locked; }
	inline void	ToggleLocked() { locked = !locked; }

    inline void SetOverflowHidden(bool hidden) { drawlinePreview.SetOverflowHidden(hidden); };
    inline void SetOverflowSelectionHidden(bool hidden) { drawlinePreview.SetOverflowSelectionHidden(hidden); };
    inline void SetOverflowAlwaysVisible(bool visible) { drawlinePreview.SetOverflowAlwaysVisible(visible); };

    inline void SetDrawSpacingHelpers(bool visible) { drawSpacingHelpers = visible; }
    inline bool GetDrawSpacingHelpers() const { return drawSpacingHelpers; }

    inline void SetShowSafeAreas(bool visible) { drawSafeAreas = visible; }
    inline bool GetShowSafeAreas() const { return drawSafeAreas; }

    inline void SetFixedScaling(bool newFixedScalingVal) { fixedScaling = newFixedScalingVal; }
    inline bool IsFixedScaling() const { return fixedScaling; }

    void SetScalingLevel(int32_t newScalingLevelVal);
    void SetScalingAmount(float newScalingAmountVal);
    inline int32_t GetScalingLevel() const { return scalingLevel; }
    inline float GetScalingAmount() const { return scalingAmount; }

    void ResetScrollingOffset();
    inline void SetScrollingOffset(float x, float y) { vec2_set(&scrollingOffset, x, y);  }
    inline float GetScrollX() const { return scrollingOffset.x; }
    inline float GetScrollY() const { return scrollingOffset.y; }
    void ClampScrollingOffsets();
    //

enum class MoveDir { Up, Down, Left, Right };
    
private:
	static vec2 GetMouseEventPos(QMouseEvent* event, float dpiValue = 1.f);
	void ProcessClick(const vec2& pos);
    
    void Nudge(int dist, MoveDir dir);
    
    inline bool CheckHoverAreaShowBlock(const QPointF qtPos);

private:
	CMouseStatePreview mouseState;
	CDrawLinePreview drawlinePreview;
	CModelPreview previewModel;

	vec2 startPos = {0,};
	vec2 mousePos = {0,};
    vec2 lastMoveOffset = {0,};
     
    bool enterHoverAreaShowBlock = false;
    bool signaledShowBlock = false;

    bool drawSpacingHelpers = true;
    bool drawSafeAreas = false;

	bool locked = false;	
	bool scrollMode = false;
	bool fixedScaling = false;
    vec2 scrollingFrom;
	vec2 scrollingOffset = { 0, };
	int32_t scalingLevel = 0;
    float scalingAmount = 1.0f;
   
    QPointer<QTimer> nudge_timer;
    bool recent_nudge = false;

    OBSDataAutoRelease wrapper = nullptr;
    bool changed = false;

    QCursor cachedPaintSourceCursor;
    QString cachedpaintSourceInfo;
};

static const uint16_t HEIGHT_HOVER_AREA = 16;
static const uint16_t BLOCK_WIDTH = 476;

inline bool CBasicPreview::CheckHoverAreaShowBlock(const QPointF qtPos)
{
    return  height() - HEIGHT_HOVER_AREA <= qtPos.y() &&
            qtPos.y() <= height() &&
            (width() /2) - (BLOCK_WIDTH / 2) <= qtPos.x() &&
            qtPos.x() <= (width() /2) + (BLOCK_WIDTH / 2);
}
