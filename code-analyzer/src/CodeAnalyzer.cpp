#include "CodeAnalyzer.hpp"


static constexpr char k_MemoryAccessFileName[] = ".\\output\\memory_access.txt";
static constexpr char k_SingleStepFileName[]   = ".\\output\\single_step.bin";


CodeAnalyzer CodeAnalyzer::s_Instance;


CodeAnalyzer& CodeAnalyzer::Get()
{
    return s_Instance;
}

void CodeAnalyzer::OnProcessAttach()
{
    SetUnhandledExceptionFilter(&CodeAnalyzer::TopLevelExceptionFilter);
    
    GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(nullptr), &m_ModuleInfo, sizeof(m_ModuleInfo));
    
    m_DuplicatedModule = VirtualAlloc(nullptr, m_ModuleInfo.SizeOfImage, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (m_DuplicatedModule == nullptr)
    {
        puts("Couldn't duplicate module.");
        ExitProcess(1);
    }
    memcpy_s(m_DuplicatedModule, m_ModuleInfo.SizeOfImage, m_ModuleInfo.lpBaseOfDll, m_ModuleInfo.SizeOfImage);
    printf_s("Duplicated module at: %p\n", m_DuplicatedModule);

    fopen_s(&m_MemoryAccessFile, k_MemoryAccessFileName, "w");
    fopen_s(&m_SingleStepFile, k_SingleStepFileName, "wb");
}

void CodeAnalyzer::OnProcessDetach()
{
    fclose(m_MemoryAccessFile);
    fclose(m_SingleStepFile);
    
    MessageBoxA(NULL, "Finished code analysis.", "Code Analyzer", MB_OK);
}

void CodeAnalyzer::OnExceptionAccessViolation(EXCEPTION_POINTERS* exceptionInfo)
{
    switch (exceptionInfo->ExceptionRecord->ExceptionInformation[0])
    {
    case EXCEPTION_READ_FAULT:
    case EXCEPTION_WRITE_FAULT:
        printf_s("%p -> %08X\n", GetAddressInOriginalModule(exceptionInfo->ExceptionRecord->ExceptionAddress), exceptionInfo->ExceptionRecord->ExceptionInformation[1]);
        EnableMemoryAccess();
        SetGuardPage();
        break;
    
    case EXCEPTION_EXECUTE_FAULT:
        exceptionInfo->ContextRecord->Eip = reinterpret_cast<uintptr_t>(GetAddressInDuplicatedModule(exceptionInfo->ExceptionRecord->ExceptionAddress));
        break;
    }
}

void CodeAnalyzer::OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo)
{
    DisableMemoryAccess();
    DisableSingleStepping(exceptionInfo);
    SetGuardPage();
}

void CodeAnalyzer::OnExceptionGuardPage(EXCEPTION_POINTERS* exceptionInfo)
{
    EnableSingleStepping(exceptionInfo);
}

void* CodeAnalyzer::GetAddressInDuplicatedModule(void* originalModuleAddress) const
{
    ptrdiff_t offset = reinterpret_cast<uintptr_t>(originalModuleAddress) - reinterpret_cast<uintptr_t>(m_ModuleInfo.lpBaseOfDll);
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_DuplicatedModule) + offset);
}

void* CodeAnalyzer::GetAddressInOriginalModule(void* duplicatedModuleAddress) const
{
    ptrdiff_t offset = reinterpret_cast<uintptr_t>(duplicatedModuleAddress) - reinterpret_cast<uintptr_t>(m_DuplicatedModule);
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_ModuleInfo.lpBaseOfDll) + offset);
}

void CodeAnalyzer::DisableMemoryAccess()
{
    DWORD oldProtection = 0;
    VirtualProtect(m_ModuleInfo.lpBaseOfDll, m_ModuleInfo.SizeOfImage, PAGE_NOACCESS, &oldProtection);
}

void CodeAnalyzer::EnableMemoryAccess()
{
    DWORD oldProtection = 0;
    VirtualProtect(m_ModuleInfo.lpBaseOfDll, m_ModuleInfo.SizeOfImage, PAGE_EXECUTE_READWRITE, &oldProtection);
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
    VirtualProtect(m_DuplicatedModule, m_ModuleInfo.SizeOfImage, PAGE_EXECUTE_READWRITE | PAGE_GUARD, &oldProtection);
}

LONG CALLBACK CodeAnalyzer::TopLevelExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{
    /*printf_s(
        "ExceptionAddress: %p\n"
        "ExceptionCode:    %08X\n"
        "---------------------------------------\n",
        ExceptionInfo->ExceptionRecord->ExceptionAddress,
        ExceptionInfo->ExceptionRecord->ExceptionCode
    );*/
    
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
