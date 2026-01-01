#include "Tracer.hpp"


Tracer::~Tracer()
{
    if (m_ProcessInformation.hThread != NULL)
    {
        CloseHandle(m_ProcessInformation.hThread);
    }
    if (m_ProcessInformation.hProcess != NULL)
    {
        CloseHandle(m_ProcessInformation.hProcess);
    }
}

bool Tracer::LoadConfig()
{
    static constexpr char configFileName[] = ".\\config.ini";
    static constexpr char sectionName[] = "Tracer";

    if (GetFileAttributesA(configFileName) == INVALID_FILE_ATTRIBUTES)
    {
        printf_s("Failed to load the tracer config because the file \"%s\" doesn't exist.\n", configFileName);
        return false;
    }

    GetPrivateProfileStringA(sectionName, "CommandLine", "", m_Config.CommandLine, sizeof(m_Config.CommandLine), configFileName);
    GetPrivateProfileStringA(sectionName, "CurrentDirectory", "", m_Config.CurrentDirectory, sizeof(m_Config.CurrentDirectory), configFileName);

    char startBreakpointRvaBuffer[9] = {};
    GetPrivateProfileStringA(sectionName, "StartBreakpointRVA", "FFFFFFFF", startBreakpointRvaBuffer, sizeof(startBreakpointRvaBuffer), configFileName);
    sscanf_s(startBreakpointRvaBuffer, "%08X", &m_Config.StartBreakpointRVA);

    char endBreakpointRvaBuffer[9] = {};
    GetPrivateProfileStringA(sectionName, "EndBreakpointRVA", "FFFFFFFF", endBreakpointRvaBuffer, sizeof(endBreakpointRvaBuffer), configFileName);
    sscanf_s(endBreakpointRvaBuffer, "%08X", &m_Config.EndBreakpointRVA);

    printf_s(
        "Loaded the tracer config.\n"
        "    - CommandLine: \"%s\"\n"
        "    - CurrentDirectory: \"%s\"\n"
        "    - StartBreakpointRVA: 0x%08X\n"
        "    - EndBreakpointRVA: 0x%08X\n",
        m_Config.CommandLine,
        m_Config.CurrentDirectory,
        m_Config.StartBreakpointRVA,
        m_Config.EndBreakpointRVA
    );

    return true;
}

bool Tracer::CreateTracedProcess()
{
    STARTUPINFOA startupInfo =
    {
        .cb = sizeof(STARTUPINFOA),
    };

    if (CreateProcessA(
        nullptr,
        m_Config.CommandLine,
        nullptr,
        nullptr,
        FALSE,
        CREATE_NEW_CONSOLE | DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS,
        nullptr,
        m_Config.CurrentDirectory,
        &startupInfo,
        &m_ProcessInformation
    ) == FALSE)
    {
        printf_s("Failed to create the traced process: %lu.\n", GetLastError());
        return false;
    }
    else
    {
        printf_s(
            "Created the traced process.\n"
            "    - Process ID: %lu\n"
            "    - Main thread ID: %lu\n",
            m_ProcessInformation.dwProcessId,
            m_ProcessInformation.dwThreadId
        );
    }

    return true;
}

void Tracer::DebugTracedProcess()
{
    while (!m_ProcessExited)
    {
        DEBUG_EVENT debugEvent = {};
        WaitForDebugEvent(&debugEvent, INFINITE);

        switch (debugEvent.dwDebugEventCode)
        {
        case CREATE_PROCESS_DEBUG_EVENT:
            OnProcessCreated(debugEvent.u.CreateProcessInfo);
            break;

        case EXIT_PROCESS_DEBUG_EVENT:
            OnProcessExited(debugEvent.u.ExitProcess);
            break;

        case EXCEPTION_DEBUG_EVENT:
            OnException(debugEvent.u.Exception.ExceptionRecord);
            break;
        }

        ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_CONTINUE);
    }
}

void Tracer::OnProcessCreated(const CREATE_PROCESS_DEBUG_INFO& createProcessInfo)
{
    fopen_s(&m_TraceFile, ".\\trace.txt", "w");
    
    CONTEXT context =
    {
        .ContextFlags = CONTEXT_DEBUG_REGISTERS,
    };
    GetThreadContext(createProcessInfo.hThread, &context);
    
    if (m_Config.StartBreakpointRVA != -1)
    {
        uintptr_t address = reinterpret_cast<uintptr_t>(createProcessInfo.lpBaseOfImage) + m_Config.StartBreakpointRVA;
        ContextEnableStartBreakpoint(context, address);
    }
    if (m_Config.EndBreakpointRVA != -1)
    {
        uintptr_t address = reinterpret_cast<uintptr_t>(createProcessInfo.lpBaseOfImage) + m_Config.EndBreakpointRVA;
        ContextEnableEndBreakpoint(context, address);
    }

    SetThreadContext(createProcessInfo.hThread, &context);
}

void Tracer::OnProcessExited(const EXIT_PROCESS_DEBUG_INFO& exitProcessInfo)
{
    fclose(m_TraceFile);

    printf_s("The traced process has exited with code 0x%08X.\n", exitProcessInfo.dwExitCode);
    m_ProcessExited = true;
}

void Tracer::OnException(const EXCEPTION_RECORD& exceptionRecord)
{
    printf_s("Exception rised at 0x%p with code 0x%08X.\n", exceptionRecord.ExceptionAddress, exceptionRecord.ExceptionCode); // TODO: remove
    
    switch (exceptionRecord.ExceptionCode)
    {
    case EXCEPTION_BREAKPOINT:
        OnExceptionBreakpoint(exceptionRecord);
        break;

    case EXCEPTION_SINGLE_STEP:
        OnExceptionSingleStep(exceptionRecord);
        break;

    default:
        printf_s(
            "Unknown exception.\n"
            "    - Exception code: 0x%08X\n"
            "    - Exception address: 0x%p\n",
            exceptionRecord.ExceptionCode,
            exceptionRecord.ExceptionAddress
        );
        break;
    }
}

void Tracer::OnExceptionBreakpoint(const EXCEPTION_RECORD& exceptionRecord)
{
    if (!m_LoaderBreakpointHit)
    {
        m_LoaderBreakpointHit = true;

        HMODULE mainModule = NULL; // The first module in the array is the main module, hence no array.
        DWORD bytesNeeded = 0;
        if (EnumProcessModules(
            m_ProcessInformation.hProcess,
            &mainModule,
            sizeof(mainModule),
            &bytesNeeded
        ) == FALSE)
        {
            printf_s("Failed to get the main module of the traced process: %ld.\n", GetLastError());
            return;
        }
        else
        {
            printf_s("Got the main module of the traced process.\n");
        }

        if (GetModuleInformation(
            m_ProcessInformation.hProcess,
            mainModule,
            &m_MainModuleInformation,
            sizeof(m_MainModuleInformation)
        ) == FALSE)
        {
            printf_s("Failed to get the main module information of the traced process: %lu.\n", GetLastError());
            return;
        }
        else
        {
            printf_s(
                "Got the main module information of the traced process.\n"
                "    - Base address: 0x%p\n"
                "    - Size: 0x%08X\n",
                m_MainModuleInformation.lpBaseOfDll,
                m_MainModuleInformation.SizeOfImage
            );
        }
    }
}

void Tracer::OnExceptionSingleStep(const EXCEPTION_RECORD& exceptionRecord)
{
    CONTEXT context =
    {
        .ContextFlags = CONTEXT_ALL,
    };
    GetThreadContext(m_ProcessInformation.hThread, &context);

    if (ContextIsStartBreakpointHit(context))
    {
        ContextDisableStartBreakpoint(context);
        m_Tracing = true;
    }
    if (ContextIsEndBreakpointHit(context))
    {
        ContextDisableEndBreakpoint(context);
        m_Tracing = false;
    }
    if (ContextIsReturnBreakpointHit(context))
    {
        ContextDisableReturnBreakpoint(context);
    }

    if (m_Tracing)
    {
        if (IsAddressInMainModule(exceptionRecord.ExceptionAddress))
        {
            LogExecutedInstruction(exceptionRecord.ExceptionAddress, context);

            ContextEnableSingleStepping(context);
        }
        else
        {
            uintptr_t returnAddress = GetReturnAddress(context);
            ContextEnableReturnBreakpoint(context, returnAddress);
            
            ContextDisableSingleStepping(context);
        }
    }

    SetThreadContext(m_ProcessInformation.hThread, &context);
}

bool Tracer::IsAddressInMainModule(const void* address) const
{
    uintptr_t startAddress = reinterpret_cast<uintptr_t>(m_MainModuleInformation.lpBaseOfDll);
    uintptr_t endAddress = reinterpret_cast<uintptr_t>(m_MainModuleInformation.lpBaseOfDll) + m_MainModuleInformation.SizeOfImage;
    
    return reinterpret_cast<uintptr_t>(address) >= startAddress && reinterpret_cast<uintptr_t>(address) < endAddress;
}

uintptr_t Tracer::GetReturnAddress(const CONTEXT& context) const
{
#if _WIN64
    void* address = reinterpret_cast<void*>(context.Rsp);
#else
    void* address = reinterpret_cast<void*>(context.Esp);
#endif
    
    uintptr_t returnAddress = 0;
    ReadProcessMemory(m_ProcessInformation.hProcess, address, &returnAddress, sizeof(returnAddress), nullptr);

    return returnAddress;
}

void Tracer::LogExecutedInstruction(const void* address, const CONTEXT& context) const
{
    BYTE code[15] = {};
    ReadProcessMemory(m_ProcessInformation.hProcess, address, code, sizeof(code), nullptr);

    fprintf_s(m_TraceFile, "0x%p |", address);

    for (int i = 0; i < 15; ++i)
    {
        fprintf_s(m_TraceFile, " %02X", code[i]);
    }

    const CONTEXT& c = context;
#if _WIN64
    fprintf_s(
        m_TraceFile,
        " | rax=0x%016llX rbx=0x%016llX rcx=0x%016llX rdx=0x%016llX rsi=0x%016llX rdi=0x%016llX"
        " r8=0x%016llX r9=0x%016llX r10=0x%016llX r11=0x%016llX r12=0x%016llX r13=0x%016llX r14=0x%016llX r15=0x%016llX"
        " rbp=0x%016llX rsp=0x%016llX rip=0x%016llX eflags=0x%08X",
        c.Rax, c.Rbx, c.Rcx, c.Rdx, c.Rsi, c.Rdi, c.R8, c.R9, c.R10, c.R11, c.R12, c.R13, c.R14, c.R15,
        c.Rbp, c.Rsp, c.Rip, c.EFlags
    );
#else
    fprintf_s(
        m_TraceFile,
        " | eax=0x%08X ebx=0x%08X ecx=0x%08X edx=0x%08X esi=0x%08X edi=0x%08X ebp=0x%08X esp=0x%08X eip=0x%08X eflags=0x%08X",
        c.Eax, c.Ebx, c.Ecx, c.Edx, c.Esi, c.Edi, c.Ebp, c.Esp, c.Eip, c.EFlags
    );
#endif

    fprintf_s(m_TraceFile, "\n");
}

void Tracer::ContextEnableStartBreakpoint(CONTEXT& context, uintptr_t address)
{
    context.Dr0 = address;
    context.Dr7 |= (1 << 0);
}

void Tracer::ContextDisableStartBreakpoint(CONTEXT& context)
{
    context.Dr0 = 0;
    context.Dr7 &= ~(1 << 0);
}

bool Tracer::ContextIsStartBreakpointHit(const CONTEXT& context)
{
    return context.Dr6 & (1 << 0);
}

void Tracer::ContextEnableEndBreakpoint(CONTEXT& context, uintptr_t address)
{
    context.Dr1 = address;
    context.Dr7 |= (1 << 2);
}

void Tracer::ContextDisableEndBreakpoint(CONTEXT& context)
{
    context.Dr1 = 0;
    context.Dr7 &= ~(1 << 2);
}

bool Tracer::ContextIsEndBreakpointHit(const CONTEXT& context)
{
    return context.Dr6 & (1 << 1);
}

void Tracer::ContextEnableReturnBreakpoint(CONTEXT& context, uintptr_t address)
{
    context.Dr2 = address;
    context.Dr7 |= (1 << 4);
}

void Tracer::ContextDisableReturnBreakpoint(CONTEXT& context)
{
    context.Dr2 = 0;
    context.Dr7 &= ~(1 << 4);
}

bool Tracer::ContextIsReturnBreakpointHit(const CONTEXT& context)
{
    return context.Dr6 & (1 << 2);
}

void Tracer::ContextEnableSingleStepping(CONTEXT& context)
{
    context.EFlags |= (1 << 8);
}

void Tracer::ContextDisableSingleStepping(CONTEXT& context)
{
    context.EFlags &= ~(1 << 8);
}
