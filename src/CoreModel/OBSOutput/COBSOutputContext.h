#pragma once


#include <memory>



#include "SBasicOutputHandler.h"


typedef std::unique_ptr<AFBasicOutputHandler> tUPTR_BASIC_OUTPUT_HANDLER;


class AFOBSOutputContext final
{
#pragma region QT Field, CTOR/DTOR
public:

    static AFOBSOutputContext& GetSingletonInstance()
    {
        static AFOBSOutputContext* instance = nullptr;
        if (instance == nullptr)
            instance = new AFOBSOutputContext;
        return *instance;
    };
    ~AFOBSOutputContext() {};
private:
    // singleton constructor
    AFOBSOutputContext() = default;
    AFOBSOutputContext(const AFOBSOutputContext&) = delete;
    AFOBSOutputContext(/* rValue */AFOBSOutputContext&& other) noexcept = delete;
    AFOBSOutputContext& operator=(const AFOBSOutputContext&) = delete;
    //
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void                                InitContext();
    bool                                FinContext();
    void                                ClearContext();

    const tUPTR_BASIC_OUTPUT_HANDLER &   GetMainOuputHandler() const;
#pragma endregion public func

#pragma region private func
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
private:
    tUPTR_BASIC_OUTPUT_HANDLER                  m_MainOutputHandler;
    bool                                        m_bStreamingStopping = false;
    bool                                        m_bRecordingStopping = false;
    bool                                        m_bReplayBufferStopping = false;
#pragma endregion private member var
};