#pragma once

#include "CQtDisplay.h"

#include <QPointer>
#include <QTimer>


#include "ViewModel/Preview/CMouseStaterPreview.h"
#include "ViewModel/Preview/CDrawLinePreview.h"
#include "ViewModel/Preview/CModelPreview.h"


class AFGraphicsContext;


class AFBasicPreview : public AFQTDisplay
{
#pragma region QT Field, CTOR/DTOR
	Q_OBJECT

public:
	AFBasicPreview(QWidget* parent,
			Qt::WindowFlags flags = Qt::WindowFlags());
	~AFBasicPreview();

	static AFBasicPreview* Get();

private slots:
	void qSlotShowContextMenu(const QPoint& pos);

#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    virtual void    keyPressEvent(QKeyEvent *event) override;
    virtual void    keyReleaseEvent(QKeyEvent *event) override;
    
    virtual void    wheelEvent(QWheelEvent *event) override;
    
	virtual void	mousePressEvent(QMouseEvent* event) override;
	virtual void	mouseReleaseEvent(QMouseEvent* event) override;
	virtual void	mouseMoveEvent(QMouseEvent* event) override;
	virtual void	leaveEvent(QEvent* event) override;

	void			UpdateCursor(uint32_t& flags);

	void			DrawOverflow();
	void			DrawSceneEditing(float dpiValue = 1.f);

	void			DrawSpacingHelpers(float dpiValue = 1.f);
	void			SetSourceBorderColor();

	void			RegisterShortCutAction(QAction* actionShortcut);
	void			RegisterRemoveSourceAction(QAction* removeSourceAction);
    void            InitSetNugeEventAction();

    // after move ModelPreview
	inline void		SetLocked(bool newLockedVal) { m_isLocked = newLockedVal; }
	inline bool		GetLocked() { return m_isLocked; }
	inline void		ToggleLocked() { m_isLocked = !m_isLocked; }

    inline void     SetOverflowHidden(bool hidden) { m_DrawPreview.SetOverflowHidden(hidden); };
    inline void     SetOverflowSelectionHidden(bool hidden) { m_DrawPreview.SetOverflowSelectionHidden(hidden); };
    inline void     SetOverflowAlwaysVisible(bool visible) { m_DrawPreview.SetOverflowAlwaysVisible(visible); };

    inline void     SetDrawSpacingHelpers(bool visible) { m_bDrawSpacingHelpers = visible; }
    inline bool     GetDrawSpacingHelpers() const { return m_bDrawSpacingHelpers; }

    inline void     SetShowSafeAreas(bool visible) { m_bDrawSafeAreas = visible; }
    inline bool     GetShowSafeAreas() const { return m_bDrawSafeAreas; }

    inline void     SetFixedScaling(bool newFixedScalingVal)
                    {
                        m_isFixedScaling = newFixedScalingVal;
                    }
    inline bool     IsFixedScaling() const { return m_isFixedScaling; }
    void            SetScalingLevel(int32_t newScalingLevelVal);
    void            SetScalingAmount(float newScalingAmountVal);
    inline int32_t  GetScalingLevel() const { return m_scalingLevel; }
    inline float    GetScalingAmount() const { return m_scalingAmount; }
    void            ResetScrollingOffset();
    inline void     SetScrollingOffset(float x, float y)
                    {
                        vec2_set(&m_vec2ScrollingOffset, x, y);
                    }
    inline float    GetScrollX() const { return m_vec2ScrollingOffset.x; }
    inline float    GetScrollY() const { return m_vec2ScrollingOffset.y; }
    void            ClampScrollingOffsets();
    //


#pragma endregion public func

#pragma region private func
enum class MoveDir { Up, Down, Left, Right };
    
private:
	static vec2     _GetMouseEventPos(QMouseEvent* event, float dpiValue = 1.f);
	void		    _ProcessClick(const vec2& pos);
    
    void            _Nudge(int dist, MoveDir dir);
    
    inline bool     _CheckHoverAreaShowBlock(const QPointF qtPos);
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
private:
	// Fast Access Context
	AFGraphicsContext*			m_pInitedContextGraphics = nullptr;
	//

	AFMouseStaterPreview		m_MouseState;
	AFDrawLinePreview			m_DrawPreview;
	AFModelPreview				m_PreviewModel;

	vec2						m_vec2StartPos = {0,};
	vec2						m_vec2MousePos = {0,};
    vec2                        m_vec2LastMoveOffset = {0,};
	vec2						m_vec2ScrollingFrom = {0,};
    
    
    bool                        m_IsEnterHoverAreaShowBlock = false;
    bool                        m_bSignaledShowBlock = false;
    
	// after move ModelPreview
	bool	m_isLocked = false;	
	bool	m_isScrollMode = false;
	bool	m_isFixedScaling = false;

    bool    m_bDrawSpacingHelpers = true;
    bool    m_bDrawSafeAreas = false;

    vec2    m_vec2scrollingFrom;
	vec2	m_vec2ScrollingOffset = { 0, };
	int32_t m_scalingLevel = 0;
    float   m_scalingAmount = 1.0f;

    QPointer<QTimer> nudge_timer;
    bool recent_nudge = false;

    OBSDataAutoRelease m_wrapper = nullptr;
    bool m_changed = false;
#pragma endregion private member var

};

static const uint16_t HEIGHT_HOVER_AREA = 16;
static const uint16_t BLOCK_WIDTH = 476;

inline bool AFBasicPreview::_CheckHoverAreaShowBlock(const QPointF qtPos)
{
    return  height() - HEIGHT_HOVER_AREA <= qtPos.y() &&
            qtPos.y() <= height() &&
            (width() /2) - (BLOCK_WIDTH / 2) <= qtPos.x() &&
            qtPos.x() <= (width() /2) + (BLOCK_WIDTH / 2);
}
