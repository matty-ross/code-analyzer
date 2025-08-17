#pragma once


#include <Windows.h>

#include "Analyzer/ExecutionTrace.hpp"


class CodeAnalyzer
{
public:
    static CodeAnalyzer& Get();

public:
    void OnProcessAttach();
    void OnProcessDetach();

private:
    static LONG CALLBACK TopLevelExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo);

private:
    static CodeAnalyzer s_Instance;

private:
    ExecutionTrace m_ExecutionTrace;

    Analyzer& m_Analyzer = m_ExecutionTrace;
};
