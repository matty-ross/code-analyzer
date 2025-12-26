#pragma once


#include <cstdio>
#include <Windows.h>


class Tracer
{
public:
    static Tracer& Get();

public:
    void OnProcessAttach();
    void OnProcessDetach();

private:
    bool OnException(EXCEPTION_POINTERS* exceptionInfo);
    
    bool IsAddressInModule(const void* address) const;
    void LogExecutedInstruction(const EXCEPTION_POINTERS* exceptionInfo);

private:
    static LONG CALLBACK TopLevelExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo);

private:
    static Tracer s_Instance;

private:
    void* m_ModuleBaseAddress = nullptr;
    size_t m_ModuleSize = 0;
    
    FILE* m_TraceFile = nullptr;
};
