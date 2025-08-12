#include "CodeAnalyzer.hpp"


CodeAnalyzer CodeAnalyzer::s_Instance;


CodeAnalyzer& CodeAnalyzer::Get()
{
    return s_Instance;
}

void CodeAnalyzer::OnProcessAttach()
{
    SetUnhandledExceptionFilter(
        [](EXCEPTION_POINTERS* ExceptionInfo) -> LONG
        {
            s_Instance.OnException(ExceptionInfo);
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    );
}

void CodeAnalyzer::OnProcessDetach()
{
    MessageBoxA(NULL, "Finished code analysis.", "Code Analyzer", MB_OK);
}

void CodeAnalyzer::OnException(EXCEPTION_POINTERS* exceptionInfo)
{
    DWORD exceptionCode = exceptionInfo->ExceptionRecord->ExceptionCode;
    
    switch (exceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        m_AnalyzerSingleStep.OnExceptionAccessViolation(exceptionInfo);
        break;
    
    case EXCEPTION_SINGLE_STEP:
        m_AnalyzerSingleStep.OnExceptionSingleStep(exceptionInfo);
        break;
    }
}
