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

    GetModuleInformation(
        m_ProcessInformation.hProcess,
        static_cast<HMODULE>(createProcessInfo.lpBaseOfImage),
        &m_MainModuleInformation,
        sizeof(m_MainModuleInformation)
    );
    
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
    // TODO: perform tracing

    printf_s("Exception rised at 0x%p with code 0x%08X.\n", exceptionRecord.ExceptionAddress, exceptionRecord.ExceptionCode);
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
