#include "MemoryAccess.hpp"

#include <cstring>
#include <Psapi.h>


void MemoryAccess::OnProcessAttach()
{
    MODULEINFO moduleInfo = {};
    GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(nullptr), &moduleInfo, sizeof(moduleInfo));
    m_ModuleBaseAddress = moduleInfo.lpBaseOfDll;
    m_ModuleSize = moduleInfo.SizeOfImage;

    DWORD oldProtection = 0;
    VirtualProtect(m_ModuleBaseAddress, m_ModuleSize, PAGE_EXECUTE_READWRITE, &oldProtection);

    m_DuplicatedModuleBaseAddress = VirtualAlloc(nullptr, m_ModuleSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    CopyModuleToDuplicatedModule();

    if (GetConfig().StartAddress != nullptr)
    {
        AddBreakpoint(GetConfig().StartAddress);
        AddBreakpoint(TranslateModuleAddressToDuplicatedModuleAddress(GetConfig().StartAddress));
    }
    if (GetConfig().EndAddress != nullptr)
    {
        AddBreakpoint(GetConfig().EndAddress);
        AddBreakpoint(TranslateModuleAddressToDuplicatedModuleAddress(GetConfig().EndAddress));
    }

    fopen_s(&m_OutputFile, ".\\output\\memory_access.txt", "w");
}

void MemoryAccess::OnProcessDetach()
{
    fclose(m_OutputFile);
}

void MemoryAccess::OnExceptionAccessViolation(EXCEPTION_POINTERS* exceptionInfo)
{
    void* instructionAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;
    void* accessAddress = reinterpret_cast<void*>(exceptionInfo->ExceptionRecord->ExceptionInformation[1]);
    
    switch (exceptionInfo->ExceptionRecord->ExceptionInformation[0])
    {
    case EXCEPTION_READ_FAULT:
        if (IsAddressInDuplicatedModule(instructionAddress))
        {
            fprintf_s(m_OutputFile, "[READ]  %p -> %p\n", TranslateDuplicatedModuleAddressToModuleAddress(instructionAddress), accessAddress);
        }
        EnableModuleMemoryAccess();
        SetTrapFlag(exceptionInfo->ContextRecord);
        break;
    
    case EXCEPTION_WRITE_FAULT:
        if (IsAddressInDuplicatedModule(instructionAddress))
        {
            fprintf_s(m_OutputFile, "[WRITE] %p -> %p\n", TranslateDuplicatedModuleAddressToModuleAddress(instructionAddress), accessAddress);
        }
        m_NeedToCopyModuleToDuplicatedModule = true;
        EnableModuleMemoryAccess();
        SetTrapFlag(exceptionInfo->ContextRecord);
        break;

    case EXCEPTION_EXECUTE_FAULT:
#if defined(_AMD64_)
        exceptionInfo->ContextRecord->Rip = reinterpret_cast<uintptr_t>(TranslateModuleAddressToDuplicatedModuleAddress(instructionAddress));
#else
        exceptionInfo->ContextRecord->Eip = reinterpret_cast<uintptr_t>(TranslateModuleAddressToDuplicatedModuleAddress(instructionAddress));
#endif
        break;
    }
}

void MemoryAccess::OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo)
{
    if (m_NeedToCopyModuleToDuplicatedModule)
    {
        CopyModuleToDuplicatedModule();
        m_NeedToCopyModuleToDuplicatedModule = false;
    }
    
    DisableModuleMemoryAccess();
    ClearTrapFlag(exceptionInfo->ContextRecord);
}

void MemoryAccess::OnExceptionBreakpoint(EXCEPTION_POINTERS* exceptionInfo)
{
    void* instructionAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;

    RemoveBreakpoint(instructionAddress);
    if (instructionAddress == GetConfig().StartAddress || instructionAddress == TranslateModuleAddressToDuplicatedModuleAddress(GetConfig().StartAddress))
    {
        DisableModuleMemoryAccess();
    }
    if (instructionAddress == GetConfig().EndAddress || instructionAddress == TranslateModuleAddressToDuplicatedModuleAddress(GetConfig().EndAddress))
    {
        EnableModuleMemoryAccess();
        ClearTrapFlag(exceptionInfo->ContextRecord);
    }
}

void MemoryAccess::EnableModuleMemoryAccess()
{
    DWORD oldProtection = 0;
    VirtualProtect(m_ModuleBaseAddress, m_ModuleSize, PAGE_EXECUTE_READWRITE, &oldProtection);
}

void MemoryAccess::DisableModuleMemoryAccess()
{
    DWORD oldProtection = 0;
    VirtualProtect(m_ModuleBaseAddress, m_ModuleSize, PAGE_NOACCESS, &oldProtection);
}

void MemoryAccess::CopyModuleToDuplicatedModule()
{
    memcpy_s(m_DuplicatedModuleBaseAddress, m_ModuleSize, m_ModuleBaseAddress, m_ModuleSize);
}

void* MemoryAccess::TranslateModuleAddressToDuplicatedModuleAddress(void* address) const
{
    ptrdiff_t offset = reinterpret_cast<uintptr_t>(address) - reinterpret_cast<uintptr_t>(m_ModuleBaseAddress);
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_DuplicatedModuleBaseAddress) + offset);
}

void* MemoryAccess::TranslateDuplicatedModuleAddressToModuleAddress(void* address) const
{
    ptrdiff_t offset = reinterpret_cast<uintptr_t>(address) - reinterpret_cast<uintptr_t>(m_DuplicatedModuleBaseAddress);
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_ModuleBaseAddress) + offset);
}

bool MemoryAccess::IsAddressInModule(void* address)
{
    void* startAddress = m_ModuleBaseAddress;
    void* endAddress = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_ModuleBaseAddress) + m_ModuleSize);
    return address >= startAddress && address < endAddress;
}

bool MemoryAccess::IsAddressInDuplicatedModule(void* address)
{
    void* startAddress = m_DuplicatedModuleBaseAddress;
    void* endAddress = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_DuplicatedModuleBaseAddress) + m_ModuleSize);
    return address >= startAddress && address < endAddress;
}
