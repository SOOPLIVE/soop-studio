#pragma once


#include <string>
#include <fstream>


#include <util/base.h>


#include "CoreModel/OBSData/SBaseLexer.h"


//Forward
class AFOBSProfiler;

class AFLogManager final
{
#pragma region QT Field, CTOR/DTOR
public:
    static AFLogManager& GetSingletonInstance()
    {
        static AFLogManager* instance = nullptr;
        if (instance == nullptr)
            instance = new AFLogManager;
        return *instance;
    };
    ~AFLogManager() = default;
private:
    // singleton constructor
    AFLogManager() = default;
    AFLogManager(const AFLogManager&) = delete;
    AFLogManager(/* rValue */AFLogManager&& other) noexcept = delete;
    AFLogManager& operator=(const AFLogManager&) = delete;
    //
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void                    DeleteOldestFile(bool has_prefix, const char* location);
    log_handler_t*          GetDefLogHandler() { return &m_LogHandler; };

    void                    CreateLogFile();

    void                    OBSBaseLog(int log_level, const char* format, ...);

    std::string&            GetStrCurrentLogFile() { return m_strCurrentLogFile; };

    void                    CreateMainOBSProfiler();
    void                    DeleteMainOBSProfiler();
    AFOBSProfiler*          GetMainOBSProfile() { return m_pMainOBSProfiler; };
#pragma endregion public func

#pragma region private func
private:
    // callback for libobs
    static void		        _DoLog(int log_level, const char* msg, va_list args, void* param);
    //

    static inline bool      _TooManyRepeatedEntries(std::fstream& logFile, const char* msg,
                                                    const char* output_str);
    static inline void      _LogStringChunk(std::fstream& logFile, char* str, int log_level);

    static void             _LogString(std::fstream& logFile, const char* timeString, char* str,
                                       int log_level);


    uint64_t                _ConvertLogName(bool has_prefix, const char* name);
    bool                    _GetToken(lexer* lex, std::string& str, base_token_type type);
    bool                    _ExpectToken(lexer* lex, const char* str, base_token_type type);

    void                    _GetLastLog(bool has_prefix, const char* subdir_to_use, std::string& last);


#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
private:
    std::string             m_strCurrentLogFile;
    std::string             m_strLastLogFile;
    std::string             m_strLastCrashLogFile;


    bool                    m_bCreatedLogFile = false;
    std::fstream            m_LogFile;
    
    log_handler_t           m_LogHandler = nullptr;

    AFOBSProfiler*          m_pMainOBSProfiler = nullptr;
#pragma endregion private member var
};


#include "CLogManager.inl"