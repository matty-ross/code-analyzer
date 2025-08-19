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

    DWORD oldProtection = 0;
    VirtualProtect(m_ModuleBaseAddress, m_ModuleSize, PAGE_EXECUTE_READWRITE, &oldProtection);

    InstallBreakpoint(m_Config.StartAddress);
    InstallBreakpoint(m_Config.EndAddress);

    fopen_s(&m_OutputFile, ".\\output\\execution_trace.txt", "w");
}

void ExecutionTrace::OnProcessDetach()
{
    fclose(m_OutputFile);
}

void ExecutionTrace::OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo)
{
    void* instructionAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;
    
    if (IsAddressInModule(instructionAddress))
    {
        LogExecutedInstruction(instructionAddress);

        EnableTrapFlag(exceptionInfo);
    }
    else
    {
        void* returnAddress = *reinterpret_cast<void**>(exceptionInfo->ContextRecord->Esp);
        InstallBreakpoint(returnAddress);
        DisableTrapFlag(exceptionInfo);
    }
}

void ExecutionTrace::OnExceptionBreakpoint(EXCEPTION_POINTERS* exceptionInfo)
{
    void* instructionAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;

    LogExecutedInstruction(instructionAddress);
    
    UninstallBreakpoint(instructionAddress);
    EnableTrapFlag(exceptionInfo);
    
    if (instructionAddress == m_Config.EndAddress)
    {
        DisableTrapFlag(exceptionInfo);
    }
}

bool ExecutionTrace::IsAddressInModule(void* address) const
{
    void* startAddress = m_ModuleBaseAddress;
    void* endAddress = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_ModuleBaseAddress) + m_ModuleSize);
    return address >= startAddress && address < endAddress;
}

void ExecutionTrace::LogExecutedInstruction(void* instructionAddress) const
{
    fprintf_s(m_OutputFile, "%p\n", instructionAddress);
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
    if (!m_Breakpoints.contains(address))
    {
        m_Breakpoints[address] = *static_cast<BYTE*>(address);
        *static_cast<BYTE*>(address) = 0xCC;
    }
}

void ExecutionTrace::UninstallBreakpoint(void* address)
{
    if (m_Breakpoints.contains(address))
    {
        *static_cast<BYTE*>(address) = m_Breakpoints[address];
        m_Breakpoints.erase(address);
    }
}
