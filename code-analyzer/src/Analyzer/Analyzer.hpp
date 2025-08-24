#pragma once


#include <map>
#include <Windows.h>


class Analyzer
{
public:
    struct Config
    {
        void* StartAddress = nullptr;
        void* EndAddress = nullptr;
    };

public:
    virtual void OnProcessAttach() {}
    virtual void OnProcessDetach() {}

    virtual void OnExceptionAccessViolation(EXCEPTION_POINTERS* exceptionInfo) {}
    virtual void OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo) {}
    virtual void OnExceptionBreakpoint(EXCEPTION_POINTERS* exceptionInfo) {}

    Config& GetConfig();

protected:
    void SetTrapFlag(CONTEXT* context);
    void ClearTrapFlag(CONTEXT* context);
    
    void AddBreakpoint(void* address);
    void RemoveBreakpoint(void* address);

private:
    Config m_Config;
    
    std::map<void*, BYTE> m_Breakpoints;
};
