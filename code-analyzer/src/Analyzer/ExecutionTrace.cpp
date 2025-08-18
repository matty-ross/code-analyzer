#include "ExecutionTrace.hpp"

#include <Psapi.h>


void ExecutionTrace::LoadConfig(const char* fileName)
{
    static constexpr char appName[] = "ExecutionTrace";
    char buffer[32] = {};

    GetPrivateProfileStringA(appName, "StartAddress", "", buffer, sizeof(buffer), fileName);
    sscanf_s(buffer, "%p", &m_Config.StartAddress);

    GetPrivateProfileStringA(appName, "EndAddress", "", buffer, sizeof(buffer), fileName);
    sscanf_s(buffer, "%p", &m_Config.EndAddress);
}

void ExecutionTrace::OnProcessAttach()
{
    MODULEINFO moduleInfo = {};
    GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(nullptr), &moduleInfo, sizeof(moduleInfo));
    m_ModuleBaseAddress = moduleInfo.lpBaseOfDll;
    m_ModuleSize = moduleInfo.SizeOfImage;

    fopen_s(&m_OutputFile, ".\\output\\execution_trace.txt", "w");

    DisableModuleExecutable();
}

void ExecutionTrace::OnProcessDetach()
{
    fclose(m_OutputFile);
}

void ExecutionTrace::OnExceptionAccessViolation(EXCEPTION_POINTERS* exceptionInfo)
{
    switch (exceptionInfo->ExceptionRecord->ExceptionInformation[0])
    {
    case EXCEPTION_EXECUTE_FAULT:
        fprintf_s(m_OutputFile, "%p\n", exceptionInfo->ExceptionRecord->ExceptionAddress);
        EnableModuleExecutable();
        EnableTrapFlag(exceptionInfo);
        break;
    }
}

void ExecutionTrace::OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo)
{
    DisableModuleExecutable();
    DisableTrapFlag(exceptionInfo);
}

void ExecutionTrace::EnableModuleExecutable()
{
    DWORD oldProtection = 0;
    VirtualProtect(m_ModuleBaseAddress, m_ModuleSize, PAGE_EXECUTE_READWRITE, &oldProtection);
}

void ExecutionTrace::DisableModuleExecutable()
{
    DWORD oldProtection = 0;
    VirtualProtect(m_ModuleBaseAddress, m_ModuleSize, PAGE_READWRITE, &oldProtection);
}

void ExecutionTrace::EnableTrapFlag(EXCEPTION_POINTERS* exceptionInfo)
{
    exceptionInfo->ContextRecord->EFlags |= 0x100;
}

void ExecutionTrace::DisableTrapFlag(EXCEPTION_POINTERS* exceptionInfo)
{
    exceptionInfo->ContextRecord->EFlags &= ~0x100;
}
