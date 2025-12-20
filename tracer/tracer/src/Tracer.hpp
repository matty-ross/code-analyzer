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
    bool OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo);
    bool OnExceptionAccessViolation(EXCEPTION_POINTERS* exceptionInfo);

    void LogExecutedInstruction(const EXCEPTION_POINTERS* exceptionInfo);

    void SetTrapFlag(CONTEXT* context) const;
    void ClearTrapFlag(CONTEXT* context) const;
    void EnableModuleExecutable() const;
    void DisableModuleExecutable() const;

    bool IsAddressInModule(const void* address) const;

private:
    static LONG CALLBACK VectoredExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo);
    static LONG CALLBACK TopLevelExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo);

private:
    static Tracer s_Instance;

private:
    void* m_ModuleBaseAddress = nullptr;
    size_t m_ModuleSize = 0;

    FILE* m_TraceFile = nullptr;
};
