#pragma once


#include <Windows.h>

#include "Analyzer/ExecutionTrace.hpp"
#include "Analyzer/MemoryAccess.hpp"


class CodeAnalyzer
{
public:
    static CodeAnalyzer& Get();

public:
    void OnProcessAttach();
    void OnProcessDetach();

private:
    void LoadConfig();

private:
    static LONG CALLBACK TopLevelExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo);

private:
    static CodeAnalyzer s_Instance;

private:
    ExecutionTrace m_ExecutionTrace;
    MemoryAccess m_MemoryAccess;

    Analyzer* m_Analyzer = nullptr;
};
