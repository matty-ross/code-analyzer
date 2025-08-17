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
}

void CodeAnalyzer::OnProcessDetach()
{
    MessageBoxA(NULL, "Finished code analysis.", "Code Analyzer", MB_OK);
}

void CodeAnalyzer::OnExceptionAccessViolation(EXCEPTION_POINTERS* exceptionInfo)
{
}

void CodeAnalyzer::OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo)
{
}

void CodeAnalyzer::OnExceptionGuardPage(EXCEPTION_POINTERS* exceptionInfo)
{
}

LONG CALLBACK CodeAnalyzer::TopLevelExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{
    printf_s(
        "---------------------------------------\n"
        "ExceptionAddress: %p\n"
        "ExceptionCode:    %08X\n",
        ExceptionInfo->ExceptionRecord->ExceptionAddress,
        ExceptionInfo->ExceptionRecord->ExceptionCode
    );
    
    switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        s_Instance.OnExceptionAccessViolation(ExceptionInfo);
        break;

    case EXCEPTION_SINGLE_STEP:
        s_Instance.OnExceptionSingleStep(ExceptionInfo);
        break;

    case EXCEPTION_GUARD_PAGE:
        s_Instance.OnExceptionGuardPage(ExceptionInfo);
        break;
    }
    
    return EXCEPTION_CONTINUE_EXECUTION;
}
