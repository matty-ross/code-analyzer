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
    
    SetUnhandledExceptionFilter(&Tracer::TopLevelExceptionFilter);
}

void Tracer::OnProcessDetach()
{
    fclose(m_TraceFile);
}

bool Tracer::OnException(EXCEPTION_POINTERS* exceptionInfo)
{
    // TODO: refactor context operation into functions

    bool end = false;

    if (exceptionInfo->ContextRecord->Dr6 & (1 << 0)) // is start breakpoint hit?
    {
        //printf_s("Start breakpoint hit at 0x%p\n", exceptionInfo->ExceptionRecord->ExceptionAddress);
        
        exceptionInfo->ContextRecord->Dr7 &= ~(1 << 0); // remove start breakpoint
    }
    
    if (exceptionInfo->ContextRecord->Dr6 & (1 << 1)) // is end breakpoint hit?
    {
        //printf_s("End breakpoint hit at 0x%p\n", exceptionInfo->ExceptionRecord->ExceptionAddress);

        exceptionInfo->ContextRecord->Dr7 &= ~(1 << 2); // remove end breakpoint
        end = true;
    }

    if (exceptionInfo->ContextRecord->Dr6 & (1 << 2)) // is return breakpoint hit?
    {
        //printf_s("Return breakpoint hit at 0x%p\n", exceptionInfo->ExceptionRecord->ExceptionAddress);

        exceptionInfo->ContextRecord->Dr7 &= ~(1 << 4); // remove return breakpoint
    }

    if (exceptionInfo->ContextRecord->Dr6 & (1 << 3)) // is pushfd breakpoint hit?
    {
        //printf_s("PUSHFD breakpoint hit at 0x%p\n", exceptionInfo->ExceptionRecord->ExceptionAddress);

        exceptionInfo->ContextRecord->Dr7 &= ~(1 << 6); // remove pushfd breakpoint
    }

    if (exceptionInfo->ContextRecord->Dr6 & (1 << 14)) // is trap flag enabled?
    {
        //printf_s("Single step at 0x%p\n", exceptionInfo->ExceptionRecord->ExceptionAddress);
    }

    if (!end)
    {
        if (!IsAddressInModule(exceptionInfo->ExceptionRecord->ExceptionAddress))
        {
            void* returnAddress = *reinterpret_cast<void**>(exceptionInfo->ContextRecord->Esp);

            exceptionInfo->ContextRecord->Dr2 = reinterpret_cast<uintptr_t>(returnAddress); // set return breakpoint
            exceptionInfo->ContextRecord->Dr7 |= (1 << 4) | (1 << 8); // enable return breakpoint

            exceptionInfo->ContextRecord->EFlags &= ~(1 << 8); // clear trap flag
        }
        else
        {
            LogExecutedInstruction(exceptionInfo);

            if (memcmp(exceptionInfo->ExceptionRecord->ExceptionAddress, "\x9C", 1) == 0) // pushfd
            {
                exceptionInfo->ContextRecord->Dr3 = reinterpret_cast<uintptr_t>(exceptionInfo->ExceptionRecord->ExceptionAddress) + 0x1; // set pushfd breakpoint
                exceptionInfo->ContextRecord->Dr7 |= (1 << 6) | (1 << 8); // enable pushfd breakpoint
            }
            else
            {
                exceptionInfo->ContextRecord->EFlags |= (1 << 8); // set trap flag
            }
        }
    }

    exceptionInfo->ContextRecord->Dr6 = 0; // clear status

    return true;
}

bool Tracer::IsAddressInModule(const void* address) const
{
    const void* startAddress = m_ModuleBaseAddress;
    const void* endAddress = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_ModuleBaseAddress) + m_ModuleSize);

    return address >= startAddress && address < endAddress;
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

LONG CALLBACK Tracer::TopLevelExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{
    /*printf_s(
        "----------------------------------------\n"
        "ExceptionAddress: 0x%p\n"
        "ExceptionCode:    0x%08X\n",
        ExceptionInfo->ExceptionRecord->ExceptionAddress,
        ExceptionInfo->ExceptionRecord->ExceptionCode
    );*/
    //system("pause > nul");

    bool handled = s_Instance.OnException(ExceptionInfo);
    return handled ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}
