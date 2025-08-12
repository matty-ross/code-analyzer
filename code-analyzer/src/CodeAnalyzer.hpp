#pragma once


#include <Windows.h>

#include "Analyzer/SingleStep.hpp"


class CodeAnalyzer
{
public:
    static CodeAnalyzer& Get();

public:
    void OnProcessAttach();
    void OnProcessDetach();

private:
    void OnException(EXCEPTION_POINTERS* exceptionInfo);

private:
    static CodeAnalyzer s_Instance;

private:
    Analyzer::SingleStep m_AnalyzerSingleStep;
};
