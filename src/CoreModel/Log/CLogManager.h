#pragma once


#include <string>
#include <fstream>

#include <util/base.h>

#include "CoreModel/OBSData/SBaseLexer.h"

class AFOBSProfiler;
//

class AFLogManager final
{
#pragma region QT Field, CTOR/DTOR
public:
    AFLogManager();
    ~AFLogManager();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void                    DeleteOldestFile(bool has_prefix, const char* location);
    log_handler_t*          GetDefLogHandler() { return &m_logHandler; };
    log_handler_t*          GetDefAPILogHandler() { return &m_logAPIHandler; };

    void                    CreateLogFile();

    std::string&            GetStrCurrentLogFile() { return m_currentLogFile; };

    inline AFOBSProfiler&   GetProfiler() const { return *m_profiler; }
#pragma endregion public func

#pragma region private func
private:
    uint64_t                _ConvertLogName(bool has_prefix, const char* name);
    bool                    _GetToken(lexer* lex, std::string& str, base_token_type type);
    bool                    _ExpectToken(lexer* lex, const char* str, base_token_type type);

    void                    _GetLastLog(bool has_prefix, const char* subdir_to_use, std::string& last);
#pragma endregion private func

#pragma region private member var
private:
    std::unique_ptr<AFOBSProfiler> m_profiler;

    std::string             m_currentLogFile;
    std::string             m_lastLogFile;

    std::string             m_currentAPILogFile;
    std::string             m_lastAPILogFile;
    std::string             m_lastCrashLogFile;

    bool                    m_createdLogFile = false;
    std::fstream            m_logFile;
    std::fstream            m_logAPIFile;
    
    log_handler_t           m_logHandler = nullptr;
    log_handler_t           m_logAPIHandler = nullptr;
#pragma endregion private member var
};

namespace AFLogUtil {
    // callback for libobs
    void		DoLog(int log_level, const char* msg, va_list args, void* param);
    //

    inline bool TooManyRepeatedEntries(std::fstream& logFile, const char* msg, const char* output_str);
    inline void LogStringChunk(std::fstream& logFile, char* str, int log_level);

    void        LogString(std::fstream& logFile, const char* timeString, char* str, int log_level);
};