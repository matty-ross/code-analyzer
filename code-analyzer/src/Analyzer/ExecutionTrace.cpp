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

    InstallBreakpoint(m_Config.StartAddress);
    InstallBreakpoint(m_Config.EndAddress);

    fopen_s(&m_OutputFile, ".\\output\\execution_trace.txt", "w");
}

void ExecutionTrace::OnProcessDetach()
{
    fclose(m_OutputFile);
}

void ExecutionTrace::OnExceptionAccessViolation(EXCEPTION_POINTERS* exceptionInfo)
{
    void* instructionAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;

    switch (exceptionInfo->ExceptionRecord->ExceptionInformation[0])
    {
    case EXCEPTION_EXECUTE_FAULT:
        fprintf_s(m_OutputFile, "%p\n", instructionAddress);
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

void ExecutionTrace::OnExceptionBreakpoint(EXCEPTION_POINTERS* exceptionInfo)
{
    void* instructionAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;

    if (instructionAddress == m_Config.StartAddress)
    {
        UninstallBreakpoint(instructionAddress);
        DisableModuleExecutable();
    }
    if (instructionAddress == m_Config.EndAddress)
    {
        UninstallBreakpoint(instructionAddress);
        EnableModuleExecutable();
        DisableTrapFlag(exceptionInfo);
    }
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

void ExecutionTrace::InstallBreakpoint(void* address)
{
    DWORD oldProtection = 0;
    VirtualProtect(address, 1, PAGE_EXECUTE_READWRITE, &oldProtection);

    m_Breakpoints[address] = *static_cast<BYTE*>(address);
    *static_cast<BYTE*>(address) = 0xCC;

    VirtualProtect(address, 1, oldProtection, &oldProtection);
}

void ExecutionTrace::UninstallBreakpoint(void* address)
{
    DWORD oldProtection = 0;
    VirtualProtect(address, 1, PAGE_EXECUTE_READWRITE, &oldProtection);

    *static_cast<BYTE*>(address) = m_Breakpoints[address];
    m_Breakpoints.erase(address);

    VirtualProtect(address, 1, oldProtection, &oldProtection);
}
