#include "Tracer.hpp"

#include <cstdint>
#include <Psapi.h>


Tracer Tracer::s_Instance;


Tracer& Tracer::Get()
{
    return s_Instance;
}

void Tracer::OnProcessAttach()
{
    MODULEINFO moduleInfo = {};
    GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(nullptr), &moduleInfo, sizeof(moduleInfo));
    m_ModuleBaseAddress = moduleInfo.lpBaseOfDll;
    m_ModuleSize = moduleInfo.SizeOfImage;

    fopen_s(&m_TraceFile, ".\\trace.txt", "w");
    
    EnableModuleExecutable();

    AddVectoredExceptionHandler(1, &Tracer::VectoredExceptionHandler);
    SetUnhandledExceptionFilter(&Tracer::TopLevelExceptionFilter);
}

void Tracer::OnProcessDetach()
{
    fclose(m_TraceFile);
}

bool Tracer::OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo)
{
    if (IsAddressInModule(exceptionInfo->ExceptionRecord->ExceptionAddress))
    {
        LogExecutedInstruction(exceptionInfo);
        SetTrapFlag(exceptionInfo->ContextRecord);
    }
    else
    {
        ClearTrapFlag(exceptionInfo->ContextRecord);
        DisableModuleExecutable();
    }

    return true;
}

bool Tracer::OnExceptionAccessViolation(EXCEPTION_POINTERS* exceptionInfo)
{
    if (
        IsAddressInModule(exceptionInfo->ExceptionRecord->ExceptionAddress) &&
        exceptionInfo->ExceptionRecord->ExceptionInformation[0] == EXCEPTION_EXECUTE_FAULT
    )
    {
        LogExecutedInstruction(exceptionInfo);
        SetTrapFlag(exceptionInfo->ContextRecord);
        EnableModuleExecutable();

        return true;
    }

    return false;
}

void Tracer::LogExecutedInstruction(const EXCEPTION_POINTERS* exceptionInfo)
{    
    const uint8_t* code = static_cast<uint8_t*>(exceptionInfo->ExceptionRecord->ExceptionAddress);
    fprintf_s(m_TraceFile, "0x%p |", code);
    
    for (int i = 0; i < 15; ++i)
    {
        fprintf_s(m_TraceFile, " %02X", code[i]);
    }
    
    const CONTEXT* c = exceptionInfo->ContextRecord;
#if _WIN64
    fprintf_s(
        m_TraceFile,
        " | rax=0x%016llX rbx=0x%016llX rcx=0x%016llX rdx=0x%016llX rsi=0x%016llX rdi=0x%016llX"
        " r8=0x%016llX r9=0x%016llX r10=0x%016llX r11=0x%016llX r12=0x%016llX r13=0x%016llX r14=0x%016llX r15=0x%016llX"
        " rbp=0x%016llX rsp=0x%016llX rip=0x%016llX eflags=0x%08X",
        c->Rax, c->Rbx, c->Rcx, c->Rdx, c->Rsi, c->Rdi, c->R8, c->R9, c->R10, c->R11, c->R12, c->R13, c->R14, c->R15,
        c->Rbp, c->Rsp, c->Rip, c->EFlags
    );
#else
    fprintf_s(
        m_TraceFile,
        " | eax=0x%08X ebx=0x%08X ecx=0x%08X edx=0x%08X esi=0x%08X edi=0x%08X ebp=0x%08X esp=0x%08X eip=0x%08X eflags=0x%08X",
        c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi, c->Ebp, c->Esp, c->Eip, c->EFlags
    );
#endif
    
    fprintf_s(m_TraceFile, "\n");
}

void Tracer::SetTrapFlag(CONTEXT* context) const
{
    context->EFlags |= 0x100;
}

void Tracer::ClearTrapFlag(CONTEXT* context) const
{
    context->EFlags &= ~0x100;
}

void Tracer::EnableModuleExecutable() const
{
    DWORD oldProtection = 0;
    VirtualProtect(m_ModuleBaseAddress, m_ModuleSize, PAGE_EXECUTE_READWRITE, &oldProtection);
}

void Tracer::DisableModuleExecutable() const
{
    DWORD oldProtection = 0;
    VirtualProtect(m_ModuleBaseAddress, m_ModuleSize, PAGE_READWRITE, &oldProtection);
}

bool Tracer::IsAddressInModule(const void* address) const
{
    const void* startAddress = m_ModuleBaseAddress;
    const void* endAddress = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_ModuleBaseAddress) + m_ModuleSize);
    
    return address >= startAddress && address < endAddress;
}

LONG CALLBACK Tracer::VectoredExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo)
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

    return handled ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

LONG CALLBACK Tracer::TopLevelExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{
    printf_s(
        "----------------------------------------\n"
        "ExceptionAddress: 0x%p\n"
        "ExceptionCode:    0x%08X\n",
        ExceptionInfo->ExceptionRecord->ExceptionAddress,
        ExceptionInfo->ExceptionRecord->ExceptionCode
    );

    ExitProcess(-1);
    return EXCEPTION_EXECUTE_HANDLER;
}
