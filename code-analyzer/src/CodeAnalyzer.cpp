#include "CodeAnalyzer.hpp"

#include <Psapi.h>


CodeAnalyzer CodeAnalyzer::s_Instance;


CodeAnalyzer& CodeAnalyzer::Get()
{
    return s_Instance;
}

void CodeAnalyzer::OnProcessAttach()
{
    MODULEINFO moduleInfo = {};
    GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(nullptr), &moduleInfo, sizeof(moduleInfo));
    m_ModuleBaseAddress = moduleInfo.lpBaseOfDll;
    m_ModuleSize = moduleInfo.SizeOfImage;

    fopen_s(&m_TraceFile, "trace.txt", "w");
    
    EnableModuleExecutable();

    AddVectoredExceptionHandler(1, &CodeAnalyzer::VectoredExceptionHandler);
}

void CodeAnalyzer::OnProcessDetach()
{
    fclose(m_TraceFile);

    MessageBoxA(NULL, "Finished code analysis.", "Code Analyzer", MB_OK);
}

bool CodeAnalyzer::OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo)
{
    if (IsAddressInModule(exceptionInfo->ExceptionRecord->ExceptionAddress))
    {
        OnExecutedInstruction(exceptionInfo);
        SetTrapFlag(exceptionInfo->ContextRecord);
    }
    else
    {
        ClearTrapFlag(exceptionInfo->ContextRecord);
        DisableModuleExecutable();
    }

    return true;
}

bool CodeAnalyzer::OnExceptionAccessViolation(EXCEPTION_POINTERS* exceptionInfo)
{
    if (IsAddressInModule(exceptionInfo->ExceptionRecord->ExceptionAddress))
    {
        OnExecutedInstruction(exceptionInfo);
        SetTrapFlag(exceptionInfo->ContextRecord);
        EnableModuleExecutable();

        return true;
    }

    return false;
}

void CodeAnalyzer::OnExecutedInstruction(EXCEPTION_POINTERS* exceptionInfo)
{
    CONTEXT* c = exceptionInfo->ContextRecord;

    fprintf_s(m_TraceFile, "0x%p | ", exceptionInfo->ExceptionRecord->ExceptionAddress);
    fprintf_s(
        m_TraceFile,
        "eax = 0x%08X; ebx = 0x%08X; ecx = 0x%08X; edx = 0x%08X; esi = 0x%08X; edi = 0x%08X; "
        "ebp = 0x%08X; esp = 0x%08X; eip = 0x%08X; eflags = 0x%08X",
        c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi,
        c->Ebp, c->Esp, c->Eip, c->EFlags
    );
    fprintf_s(m_TraceFile, "\n");
}

void CodeAnalyzer::SetTrapFlag(CONTEXT* context)
{
    context->EFlags |= 0x100;
}

void CodeAnalyzer::ClearTrapFlag(CONTEXT* context)
{
    context->EFlags &= ~0x100;
}

void CodeAnalyzer::EnableModuleExecutable()
{
    DWORD oldProtection = 0;
    VirtualProtect(m_ModuleBaseAddress, m_ModuleSize, PAGE_EXECUTE_READWRITE, &oldProtection);
}

void CodeAnalyzer::DisableModuleExecutable()
{
    DWORD oldProtection = 0;
    VirtualProtect(m_ModuleBaseAddress, m_ModuleSize, PAGE_READWRITE, &oldProtection);
}

bool CodeAnalyzer::IsAddressInModule(void* address) const
{
    void* startAddress = m_ModuleBaseAddress;
    void* endAddress = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_ModuleBaseAddress) + m_ModuleSize);
    
    return address >= startAddress && address < endAddress;
}

LONG CALLBACK CodeAnalyzer::VectoredExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo)
{
    bool handled = false;

    switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_SINGLE_STEP:
        handled = s_Instance.OnExceptionSingleStep(ExceptionInfo);
        break;

    case EXCEPTION_ACCESS_VIOLATION:
        handled = s_Instance.OnExceptionAccessViolation(ExceptionInfo);
        break;
    }

    if (!handled)
    {
        printf_s(
            "----------------------------------------\n"
            "ExceptionAddress: %p\n"
            "ExceptionCode:    %08X\n",
            ExceptionInfo->ExceptionRecord->ExceptionAddress,
            ExceptionInfo->ExceptionRecord->ExceptionCode
        );
    }
    
    return EXCEPTION_CONTINUE_EXECUTION;
}
