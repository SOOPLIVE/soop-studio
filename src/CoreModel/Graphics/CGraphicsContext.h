#pragma once

#include <obs.hpp>
#include <graphics/graphics.h>


class AFGraphicsContext final
{
#pragma region QT Field, CTOR/DTOR
public:
    static AFGraphicsContext& GetSingletonInstance()
    {
        static AFGraphicsContext* instance = nullptr;
        if (instance == nullptr)
            instance = new AFGraphicsContext;
        return *instance;
    };
    ~AFGraphicsContext() {};
    // singleton constructor
    AFGraphicsContext() = default;
    AFGraphicsContext(const AFGraphicsContext&) = delete;
    AFGraphicsContext(/* rValue */AFGraphicsContext&& other) noexcept = delete;
    AFGraphicsContext& operator=(const AFGraphicsContext&) = delete;
    //
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    static inline void                  InitSafeAreas(gs_vertbuffer_t** actionSafeMargin,
                                                      gs_vertbuffer_t** graphicsSafeMargin,
                                                      gs_vertbuffer_t** fourByThreeSafeMargin,
                                                      gs_vertbuffer_t** leftLine,
                                                      gs_vertbuffer_t** topLine,
                                                      gs_vertbuffer_t** rightLine);
    static inline void                  RenderSafeAreas(gs_vertbuffer_t *vb, int cx, int cy);


    void                                InitContext();
    bool                                FinContext();
    void                                ClearContext();

    bool                                IsInited() { return m_bInited; };


    // Value Of Context
    gs_vertbuffer_t*                    GetBoxVB() { return m_vbBox; };

    int32_t                             GetMainPreviewX() { return m_iMainPreviewX; };
    void                                SetMainPreviewX(int32_t value) { m_iMainPreviewX = value; };
    int32_t                             GetMainPreviewY() { return m_iMainPreviewY; };
    void                                SetMainPreviewY(int32_t value) { m_iMainPreviewY = value; };
    int32_t                             GetMainPreviewCX() { return m_iMainPreviewCX; };
    void                                SetMainPreviewCX(int32_t value) { m_iMainPreviewCX = value; };
    int32_t                             GetMainPreviewCY() { return m_iMainPreviewCY; };
    void                                SetMainPreviewCY(int32_t value) { m_iMainPreviewCY = value; };
    float                               GetMainPreviewScale() { return m_iMainPreviewScale; };
    void                                SetMainPreviewScale(float value) { m_iMainPreviewScale = value; };
    
    int32_t                             GetProgramPreviewX() { return m_iProgramPreviewX; };
    void                                SetProgramPreviewX(int32_t value) { m_iProgramPreviewX = value; };
    int32_t                             GetProgramPreviewY() { return m_iProgramPreviewY; };
    void                                SetProgramPreviewY(int32_t value) { m_iProgramPreviewY = value; };
    int32_t                             GetProgramPreviewCX() { return m_iProgramPreviewCX; };
    void                                SetProgramPreviewCX(int32_t value) { m_iProgramPreviewCX = value; };
    int32_t                             GetProgramPreviewCY() { return m_iProgramPreviewCY; };
    void                                SetProgramPreviewCY(int32_t value) { m_iProgramPreviewCY = value; };
    float                               GetProgramPreviewScale() { return m_iProgramPreviewScale; };
    void                                SetProgramPreviewScale(float value) { m_iProgramPreviewScale = value; };
    //

    gs_vertbuffer_t*                    GetActionSafeMargin() { return m_vActionSafeMargin; };
    gs_vertbuffer_t*                    GetGraphicsSafeMargin() { return m_vGraphicsSafeMargin; };
    gs_vertbuffer_t*                    GetFourByThreeSafeMargin() { return m_FourByThreeSafeMargin; };
    gs_vertbuffer_t*                    GetLeftLine() { return m_LeftLine; };
    gs_vertbuffer_t*                    GetTopLine() { return m_TopLine; };
    gs_vertbuffer_t*                    GetRightLine() { return m_RightLine; };


    const char*                         GetRenderModule() const;
#pragma endregion public func

#pragma region private func
    void                                _InitPrimitives();
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
    bool                                m_bInited = false;


    gs_vertbuffer_t*                    m_vbBox = nullptr;
    gs_vertbuffer_t*                    m_vbBoxLeft = nullptr;
    gs_vertbuffer_t*                    m_vbBoxTop = nullptr;
    gs_vertbuffer_t*                    m_vbBoxRight = nullptr;
    gs_vertbuffer_t*                    m_vbBoxBottom = nullptr;
    gs_vertbuffer_t*                    m_vbCircle = nullptr;

    gs_vertbuffer_t*                    m_vActionSafeMargin = nullptr;
    gs_vertbuffer_t*                    m_vGraphicsSafeMargin = nullptr;
    gs_vertbuffer_t*                    m_FourByThreeSafeMargin = nullptr;
    gs_vertbuffer_t*                    m_LeftLine = nullptr;
    gs_vertbuffer_t*                    m_TopLine = nullptr;
    gs_vertbuffer_t*                    m_RightLine = nullptr;

    int32_t                             m_iMainPreviewX = 0;
    int32_t                             m_iMainPreviewY = 0;
    int32_t                             m_iMainPreviewCX = 0;
    int32_t                             m_iMainPreviewCY = 0;
    float                               m_iMainPreviewScale = 0.0f;
    
    
    int32_t                             m_iProgramPreviewX = 0;
    int32_t                             m_iProgramPreviewY = 0;
    int32_t                             m_iProgramPreviewCX = 0;
    int32_t                             m_iProgramPreviewCY = 0;
    float                               m_iProgramPreviewScale = 0.0f;
#pragma endregion private member var
};

#include "CGraphicsContext.inl"