#pragma once

#include <obs.hpp>
#include <graphics/graphics.h>


class AFGraphicsContext final
{
public:
    AFGraphicsContext() = default;
    ~AFGraphicsContext() = default;

public:
    void                InitContext();
    bool                FinContext();

    bool                IsInited() { return m_inited; };


    // Value Of Context
    gs_vertbuffer_t*    GetBoxVB() { return m_pBox; };

    int32_t             GetMainPreviewX() { return m_mainPreviewX; };
    void                SetMainPreviewX(int32_t value) { m_mainPreviewX = value; };
    int32_t             GetMainPreviewY() { return m_mainPreviewY; };
    void                SetMainPreviewY(int32_t value) { m_mainPreviewY = value; };
    int32_t             GetMainPreviewCX() { return m_mainPreviewCX; };
    void                SetMainPreviewCX(int32_t value) { m_mainPreviewCX = value; };
    int32_t             GetMainPreviewCY() { return m_mainPreviewCY; };
    void                SetMainPreviewCY(int32_t value) { m_mainPreviewCY = value; };
    float               GetMainPreviewScale() { return m_mainPreviewScale; };
    void                SetMainPreviewScale(float value) { m_mainPreviewScale = value; };

    int32_t             GetProgramPreviewX() { return m_programPreviewX; };
    void                SetProgramPreviewX(int32_t value) { m_programPreviewX = value; };
    int32_t             GetProgramPreviewY() { return m_programPreviewY; };
    void                SetProgramPreviewY(int32_t value) { m_programPreviewY = value; };
    int32_t             GetProgramPreviewCX() { return m_programPreviewCX; };
    void                SetProgramPreviewCX(int32_t value) { m_programPreviewCX = value; };
    int32_t             GetProgramPreviewCY() { return m_programPreviewCY; };
    void                SetProgramPreviewCY(int32_t value) { m_programPreviewCY = value; };
    float               GetProgramPreviewScale() { return m_programPreviewScale; };
    void                SetProgramPreviewScale(float value) { m_programPreviewScale = value; };
    //

    //Safe Area Function Need Check
    gs_vertbuffer_t*    GetActionSafeMargin() { return m_pActionSafeMargin; };
    gs_vertbuffer_t*    GetGraphicsSafeMargin() { return m_pGraphicsSafeMargin; };
    gs_vertbuffer_t*    GetFourByThreeSafeMargin() { return m_pFourByThreeSafeMargin; };
    gs_vertbuffer_t*    GetLeftLine() { return m_pLeftLine; };
    gs_vertbuffer_t*    GetTopLine() { return m_pTopLine; };
    gs_vertbuffer_t*    GetRightLine() { return m_pRightLine; };

    //Safe Area Function Need Check


    const char*         GetRenderModule() const;

private:
    void                _InitPrimitives();

private:
    bool                m_inited = false;

    gs_vertbuffer_t*    m_pBox = nullptr;
    gs_vertbuffer_t*    m_pBoxLeft = nullptr;
    gs_vertbuffer_t*    m_pBoxTop = nullptr;
    gs_vertbuffer_t*    m_pBoxRight = nullptr;
    gs_vertbuffer_t*    m_pBoxBottom = nullptr;
    gs_vertbuffer_t*    m_pCircle = nullptr;

    //Safe Area Function Need Check
    gs_vertbuffer_t*    m_pActionSafeMargin = nullptr;
    gs_vertbuffer_t*    m_pGraphicsSafeMargin = nullptr;
    gs_vertbuffer_t*    m_pFourByThreeSafeMargin = nullptr;
    gs_vertbuffer_t*    m_pLeftLine = nullptr;
    gs_vertbuffer_t*    m_pTopLine = nullptr;
    gs_vertbuffer_t*    m_pRightLine = nullptr;
    //Safe Area Function Need Check

    int32_t             m_mainPreviewX = 0;
    int32_t             m_mainPreviewY = 0;
    int32_t             m_mainPreviewCX = 0;
    int32_t             m_mainPreviewCY = 0;
    float               m_mainPreviewScale = 0.0f;

    int32_t             m_programPreviewX = 0;
    int32_t             m_programPreviewY = 0;
    int32_t             m_programPreviewCX = 0;
    int32_t             m_programPreviewCY = 0;
    float               m_programPreviewScale = 0.0f;
};