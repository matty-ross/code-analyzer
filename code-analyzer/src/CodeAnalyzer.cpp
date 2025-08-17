#include "CodeAnalyzer.hpp"

#include <cstdio>


CodeAnalyzer CodeAnalyzer::s_Instance;


CodeAnalyzer& CodeAnalyzer::Get()
{
    return s_Instance;
}

void CodeAnalyzer::OnProcessAttach()
{
    SetUnhandledExceptionFilter(&CodeAnalyzer::TopLevelExceptionFilter);
    m_Analyzer.OnProcessAttach();
}

void CodeAnalyzer::OnProcessDetach()
{
    m_Analyzer.OnProcessDetach();
    MessageBoxA(NULL, "Finished code analysis.", "Code Analyzer", MB_OK);
}

LONG CALLBACK CodeAnalyzer::TopLevelExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{
    /*printf_s(
        "---------------------------------------\n"
        "ExceptionAddress: %p\n"
        "ExceptionCode:    %08X\n",
        ExceptionInfo->ExceptionRecord->ExceptionAddress,
        ExceptionInfo->ExceptionRecord->ExceptionCode
    );*/
    //system("pause > nul");
    
    switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        s_Instance.m_Analyzer.OnExceptionAccessViolation(ExceptionInfo);
        break;

    case EXCEPTION_SINGLE_STEP:
        s_Instance.m_Analyzer.OnExceptionSingleStep(ExceptionInfo);
        break;

    case EXCEPTION_GUARD_PAGE:
        s_Instance.m_Analyzer.OnExceptionGuardPage(ExceptionInfo);
        break;
    }
    
    return EXCEPTION_CONTINUE_EXECUTION;
}
