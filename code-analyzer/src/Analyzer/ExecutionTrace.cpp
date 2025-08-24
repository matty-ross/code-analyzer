#include "ExecutionTrace.hpp"

#include <Psapi.h>
#include <DbgHelp.h>


void ExecutionTrace::OnProcessAttach()
{
    MODULEINFO moduleInfo = {};
    GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(nullptr), &moduleInfo, sizeof(moduleInfo));
    m_ModuleBaseAddress = moduleInfo.lpBaseOfDll;
    m_ModuleSize = moduleInfo.SizeOfImage;

    DWORD oldProtection = 0;
    VirtualProtect(m_ModuleBaseAddress, m_ModuleSize, PAGE_EXECUTE_READWRITE, &oldProtection);

    if (GetConfig().StartAddress != nullptr)
    {
        AddBreakpoint(GetConfig().StartAddress);
    }
    if (GetConfig().EndAddress != nullptr)
    {
        AddBreakpoint(GetConfig().EndAddress);
    }

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

        SetTrapFlag(exceptionInfo->ContextRecord);
    }
    else
    {
        LogExternalCall(instructionAddress);
        
        void* returnAddress = *reinterpret_cast<void**>(exceptionInfo->ContextRecord->Esp);
        AddBreakpoint(returnAddress);
        ClearTrapFlag(exceptionInfo->ContextRecord);
    }
}

void ExecutionTrace::OnExceptionBreakpoint(EXCEPTION_POINTERS* exceptionInfo)
{
    void* instructionAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;

    LogExecutedInstruction(instructionAddress);
    
    RemoveBreakpoint(instructionAddress);
    if (instructionAddress == GetConfig().EndAddress)
    {
        ClearTrapFlag(exceptionInfo->ContextRecord);
    }
    else
    {
        SetTrapFlag(exceptionInfo->ContextRecord);
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

void ExecutionTrace::LogExternalCall(void* instructionAddress) const
{
    HMODULE moduleHandle = NULL;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        static_cast<LPCSTR>(instructionAddress),
        &moduleHandle
    );

    ULONG size = 0;
    IMAGE_EXPORT_DIRECTORY* exportDirectory = static_cast<IMAGE_EXPORT_DIRECTORY*>(ImageDirectoryEntryToDataEx(
        moduleHandle,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_EXPORT,
        &size,
        nullptr
    ));

    auto rva2va = [=](DWORD rva) -> void* { return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(moduleHandle) + rva); };
    auto va2rva = [=](void* va) -> DWORD { return reinterpret_cast<uintptr_t>(va) - reinterpret_cast<uintptr_t>(moduleHandle); };

    const char* moduleName = static_cast<const char*>(rva2va(exportDirectory->Name));

    DWORD* addressOfFunctions = static_cast<DWORD*>(rva2va(exportDirectory->AddressOfFunctions));
    DWORD* addressOfNames = static_cast<DWORD*>(rva2va(exportDirectory->AddressOfNames));
    WORD* addressOfNameOrdinals = static_cast<WORD*>(rva2va(exportDirectory->AddressOfNameOrdinals));
    
    int functionOrdinal = -1;
    for (DWORD i = 0; i < exportDirectory->NumberOfFunctions; ++i)
    {
        void* functionAddress = rva2va(addressOfFunctions[i]);
        if (functionAddress == instructionAddress)
        {
            functionOrdinal = i;
            break;
        }
    }

    if (functionOrdinal != -1)
    {
        for (DWORD i = 0; i < exportDirectory->NumberOfNames; ++i)
        {
            if (functionOrdinal == addressOfNameOrdinals[i])
            {
                const char* functionName = static_cast<const char*>(rva2va(addressOfNames[i]));
                fprintf_s(m_OutputFile, "; %s!%s\n", moduleName, functionName);
                return;
            }
        }

        DWORD functionBiasedOrdinal = functionOrdinal + exportDirectory->Base;
        fprintf_s(m_OutputFile, "; %s!%lu\n", moduleName, functionBiasedOrdinal);
    }
    else
    {
        DWORD rva = va2rva(instructionAddress);
        fprintf_s(m_OutputFile, "; %s + %08X\n", moduleName, rva);
    }
}
