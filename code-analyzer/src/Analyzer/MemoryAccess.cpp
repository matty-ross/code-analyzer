#include "MemoryAccess.hpp"

#include <cstring>
#include <Psapi.h>


void MemoryAccess::LoadConfig(const char* fileName)
{
    static constexpr char appName[] = "MemoryAccess";
    char buffer[32] = {};

    GetPrivateProfileStringA(appName, "StartAddress", "", buffer, sizeof(buffer), fileName);
    sscanf_s(buffer, "%p", &m_Config.StartAddress);

    GetPrivateProfileStringA(appName, "EndAddress", "", buffer, sizeof(buffer), fileName);
    sscanf_s(buffer, "%p", &m_Config.EndAddress);
}

void MemoryAccess::OnProcessAttach()
{
    MODULEINFO moduleInfo = {};
    GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(nullptr), &moduleInfo, sizeof(moduleInfo));
    m_ModuleBaseAddress = moduleInfo.lpBaseOfDll;
    m_ModuleSize = moduleInfo.SizeOfImage;

    m_DuplicatedModuleBaseAddress = VirtualAlloc(nullptr, m_ModuleSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    CopyModuleToDuplicatedModule();

    fopen_s(&m_OutputFile, ".\\output\\memory_access.txt", "w");

    DisableModuleMemoryAccess();
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
        EnableTrapFlag(exceptionInfo);
        break;
    
    case EXCEPTION_WRITE_FAULT:
        if (IsAddressInDuplicatedModule(instructionAddress))
        {
            fprintf_s(m_OutputFile, "[WRITE] %p -> %p\n", TranslateDuplicatedModuleAddressToModuleAddress(instructionAddress), accessAddress);
        }
        m_NeedToCopyModuleToDuplicatedModule = true;
        EnableModuleMemoryAccess();
        EnableTrapFlag(exceptionInfo);
        break;

    case EXCEPTION_EXECUTE_FAULT:
        exceptionInfo->ContextRecord->Eip = reinterpret_cast<uintptr_t>(TranslateModuleAddressToDuplicatedModuleAddress(instructionAddress));
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
    DisableTrapFlag(exceptionInfo);
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

void MemoryAccess::EnableTrapFlag(EXCEPTION_POINTERS* exceptionInfo)
{
    exceptionInfo->ContextRecord->EFlags |= 0x100;
}

void MemoryAccess::DisableTrapFlag(EXCEPTION_POINTERS* exceptionInfo)
{
    exceptionInfo->ContextRecord->EFlags &= ~0x100;
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
    void* start = m_ModuleBaseAddress;
    void* end = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_ModuleBaseAddress) + m_ModuleSize);
    return address >= start && address < end;
}

bool MemoryAccess::IsAddressInDuplicatedModule(void* address)
{
    void* start = m_DuplicatedModuleBaseAddress;
    void* end = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_DuplicatedModuleBaseAddress) + m_ModuleSize);
    return address >= start && address < end;
}
