#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <Windows.h>


void ToogleStartBreakpoint(CONTEXT& context, bool enable)
{
    if (enable)
    {
        context.Dr0 = 0x08281000;
        context.Dr7 |= (1 << 0);
    }
    else
    {
        context.Dr0 = 0x00000000;
        context.Dr7 &= ~(1 << 0);
    }
}

void ToggleReturnBreakpoint(CONTEXT& context, bool enable, uintptr_t returnAddress)
{
    if (enable)
    {
        context.Dr1 = returnAddress;
        context.Dr7 |= (1 << 2);
    }
    else
    {
        context.Dr1 = 0x00000000;
        context.Dr7 &= ~(1 << 2);
    }
}

void EnableSingleStepping(CONTEXT& context)
{
    context.EFlags |= 0x100;
}

void LogExecutedInstruction(HANDLE process, CONTEXT& context, const EXCEPTION_RECORD& exceptionRecord, FILE* file)
{
    uint8_t code[15] = {};
    ReadProcessMemory(process, exceptionRecord.ExceptionAddress, code, 15, nullptr);
    
    fprintf_s(file, "0x%p |", exceptionRecord.ExceptionAddress);

    for (int i = 0; i < 15; ++i)
    {
        fprintf_s(file, " %02X", code[i]);
    }

    CONTEXT& c = context;

#if _WIN64
    fprintf_s(
        file,
        " | rax=0x%016llX rbx=0x%016llX rcx=0x%016llX rdx=0x%016llX rsi=0x%016llX rdi=0x%016llX"
        " r8=0x%016llX r9=0x%016llX r10=0x%016llX r11=0x%016llX r12=0x%016llX r13=0x%016llX r14=0x%016llX r15=0x%016llX"
        " rbp=0x%016llX rsp=0x%016llX rip=0x%016llX eflags=0x%08X",
        c.Rax, c.Rbx, c.Rcx, c.Rdx, c.Rsi, c.Rdi, c.R8, c.R9, c.R10, c.R11, c.R12, c.R13, c.R14, c.R15,
        c.Rbp, c.Rsp, c.Rip, c.EFlags
    );
#else
    fprintf_s(
        file,
        " | eax=0x%08X ebx=0x%08X ecx=0x%08X edx=0x%08X esi=0x%08X edi=0x%08X ebp=0x%08X esp=0x%08X eip=0x%08X eflags=0x%08X",
        c.Eax, c.Ebx, c.Ecx, c.Edx, c.Esi, c.Edi, c.Ebp, c.Esp, c.Eip, c.EFlags
    );
#endif

    fprintf_s(file, "\n");
}


void HandleException(HANDLE process, HANDLE thread, const EXCEPTION_RECORD& exceptionRecord, FILE* file)
{
    if (exceptionRecord.ExceptionCode != EXCEPTION_SINGLE_STEP)
    {
        return;
    }

    CONTEXT context =
    {
        .ContextFlags = CONTEXT_ALL,
    };
    GetThreadContext(thread, &context);

    if (context.Dr6 & (1 << 0))
    {
        ToogleStartBreakpoint(context, false);
    }
    if (context.Dr6 & (1 << 1))
    {
        ToggleReturnBreakpoint(context, false, 0);
    }
    
    if ((uintptr_t)exceptionRecord.ExceptionAddress >= 0x00400000 && (uintptr_t)exceptionRecord.ExceptionAddress < 0x0837D000)
    {
        /*printf_s("executed 0x%p\n", exceptionRecord.ExceptionAddress);
        system("pause > nul");
        LogExecutedInstruction(process, context, exceptionRecord, file);*/

        static uint32_t instructionsExecuted = 0;

        ++instructionsExecuted;
        if (instructionsExecuted < 500'000)
        {
            EnableSingleStepping(context);
        }
    }
    else
    {
        uintptr_t returnAddress = 0;
        ReadProcessMemory(process, (void*)context.Esp, &returnAddress, 4, nullptr);

        ToggleReturnBreakpoint(context, true, returnAddress);
    }

    SetThreadContext(thread, &context);
}


int main()
{
    char commandLine[] = "D:\\Reversing\\Tracer\\BurnoutPR.exe";
    char currentDirectory[] = "D:\\Reversing\\Tracer";
    
    STARTUPINFOA startupInfo =
    {
        .cb = sizeof(STARTUPINFOA),
    };
    PROCESS_INFORMATION processInformation = {};
    CreateProcessA(
        nullptr,
        commandLine,
        nullptr,
        nullptr,
        FALSE,
        DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS,
        nullptr,
        currentDirectory,
        &startupInfo,
        &processInformation
    );

    FILE* file = nullptr;
    fopen_s(&file, "trace.txt", "w");

    bool running = true;
    while (running)
    {
        DEBUG_EVENT debugEvent = {};
        WaitForDebugEvent(&debugEvent, INFINITE);
        
        switch (debugEvent.dwDebugEventCode)
        {
        case CREATE_PROCESS_DEBUG_EVENT:
            {
                printf_s("Process created at 0x%p\n", debugEvent.u.CreateProcessInfo.lpBaseOfImage);
                CONTEXT context =
                {
                    .ContextFlags = CONTEXT_ALL,
                };
                GetThreadContext(processInformation.hThread, &context);
                ToogleStartBreakpoint(context, true);
                SetThreadContext(processInformation.hThread, &context);
            }
            break;

        case EXIT_PROCESS_DEBUG_EVENT:
            printf_s("Process exited with code 0x%08X\n", debugEvent.u.ExitProcess.dwExitCode);
            running = false;
            break;

        case EXCEPTION_DEBUG_EVENT:
            //printf_s("Exception rised at 0x%p with code 0x%08X\n", debugEvent.u.Exception.ExceptionRecord.ExceptionAddress, debugEvent.u.Exception.ExceptionRecord.ExceptionCode);
            HandleException(processInformation.hProcess, processInformation.hThread, debugEvent.u.Exception.ExceptionRecord, file);
            break;
        }
        
        ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_CONTINUE);
    }

    fclose(file);

    CloseHandle(processInformation.hThread);
    CloseHandle(processInformation.hProcess);
    
    return EXIT_SUCCESS;
}
