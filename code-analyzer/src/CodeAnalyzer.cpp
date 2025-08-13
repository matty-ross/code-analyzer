#include "CodeAnalyzer.hpp"


static constexpr char k_ConfigFileName[] = ".\\config.ini";
static constexpr char k_SingleStepFileName[] = ".\\output\\single_step.bin";


CodeAnalyzer CodeAnalyzer::s_Instance;


CodeAnalyzer& CodeAnalyzer::Get()
{
    return s_Instance;
}

void CodeAnalyzer::OnProcessAttach()
{
    LoadConfig();

    GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(nullptr), &m_ModuleInfo, sizeof(m_ModuleInfo));
    SetUnhandledExceptionFilter(&CodeAnalyzer::TopLevelExceptionFilter);

    if (m_Config.SingleStep)
    {
        fopen_s(&m_SingleStepFile, k_SingleStepFileName, "wb");
    }
}

void CodeAnalyzer::OnProcessDetach()
{
    if (m_Config.SingleStep)
    {
        fclose(m_SingleStepFile);
    }
    
    MessageBoxA(NULL, "Finished code analysis.", "Code Analyzer", MB_OK);
}

void CodeAnalyzer::LoadConfig()
{
    static constexpr char section[] = "Config";
    
    m_Config.SingleStep = GetPrivateProfileIntA(section, "SingleStep", 0, k_ConfigFileName);
}

void CodeAnalyzer::OnExceptionAccessViolation(EXCEPTION_POINTERS* exceptionInfo)
{
}

void CodeAnalyzer::OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo)
{
    DWORD eip = exceptionInfo->ContextRecord->Eip;
    if (m_Config.SingleStep)
    {
        fwrite(&eip, sizeof(eip), 1, m_SingleStepFile);
    }
    
    DisableSingleStepping(exceptionInfo);
    if (m_Config.SingleStep)
    {
        SetGuardPage();
    }
}

void CodeAnalyzer::OnExceptionGuardPage(EXCEPTION_POINTERS* exceptionInfo)
{
    if (m_Config.SingleStep)
    {
        EnableSingleStepping(exceptionInfo);
    }
}

void CodeAnalyzer::EnableSingleStepping(EXCEPTION_POINTERS* exceptionInfo)
{
    exceptionInfo->ContextRecord->EFlags |= 0x100;
}

void CodeAnalyzer::DisableSingleStepping(EXCEPTION_POINTERS* exceptionInfo)
{
    exceptionInfo->ContextRecord->EFlags &= ~0x100;
}

void CodeAnalyzer::SetGuardPage()
{
    DWORD oldProtection = 0;
    VirtualProtect(m_ModuleInfo.lpBaseOfDll, m_ModuleInfo.SizeOfImage, PAGE_EXECUTE_READWRITE | PAGE_GUARD, &oldProtection);
}

LONG CALLBACK CodeAnalyzer::TopLevelExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{
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
