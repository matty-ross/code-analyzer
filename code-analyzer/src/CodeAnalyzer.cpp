#include "CodeAnalyzer.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>


CodeAnalyzer CodeAnalyzer::s_Instance;


CodeAnalyzer& CodeAnalyzer::Get()
{
    return s_Instance;
}

void CodeAnalyzer::OnProcessAttach()
{
    AddVectoredExceptionHandler(1, &CodeAnalyzer::VectoredExceptionHandler);
    LoadConfig();
    m_Analyzer->OnProcessAttach();
}

void CodeAnalyzer::OnProcessDetach()
{
    m_Analyzer->OnProcessDetach();
    MessageBoxA(NULL, "Finished code analysis.", "Code Analyzer", MB_OK);
}

void CodeAnalyzer::LoadConfig()
{
    static constexpr char fileName[] = ".\\config.ini";
    static constexpr char appName[] = "Config";
    char buffer[32] = {};

    GetPrivateProfileStringA(appName, "Analyzer", "", buffer, sizeof(buffer), fileName);
    m_Analyzer = [&]() -> Analyzer*
    {
        if (strcmp(buffer, "ExecutionTrace") == 0) return &m_ExecutionTrace;
        if (strcmp(buffer, "MemoryAccess") == 0)   return &m_MemoryAccess;
        return nullptr;
    }();

    m_PrintExceptions = GetPrivateProfileIntA(appName, "PrintExceptions", 0, fileName);
    m_PauseAfterExceptions = GetPrivateProfileIntA(appName, "PauseAfterExceptions", 0, fileName);

    GetPrivateProfileStringA(appName, "StartAddress", "", buffer, sizeof(buffer), fileName);
    sscanf_s(buffer, "%p", &m_Analyzer->GetConfig().StartAddress);

    GetPrivateProfileStringA(appName, "EndAddress", "", buffer, sizeof(buffer), fileName);
    sscanf_s(buffer, "%p", &m_Analyzer->GetConfig().EndAddress);
}

LONG CALLBACK CodeAnalyzer::VectoredExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo)
{
    if (s_Instance.m_PrintExceptions)
    {
        printf_s(
            "---------------------------------------\n"
            "ExceptionAddress: %p\n"
            "ExceptionCode:    %08X\n",
            ExceptionInfo->ExceptionRecord->ExceptionAddress,
            ExceptionInfo->ExceptionRecord->ExceptionCode
        );
    }
    if (s_Instance.m_PauseAfterExceptions)
    {
        system("pause > nul");
    }

    switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        s_Instance.m_Analyzer->OnExceptionAccessViolation(ExceptionInfo);
        break;

    case EXCEPTION_SINGLE_STEP:
        s_Instance.m_Analyzer->OnExceptionSingleStep(ExceptionInfo);
        break;

    case EXCEPTION_BREAKPOINT:
        s_Instance.m_Analyzer->OnExceptionBreakpoint(ExceptionInfo);
        break;
    }
    
    return EXCEPTION_CONTINUE_EXECUTION;
}
