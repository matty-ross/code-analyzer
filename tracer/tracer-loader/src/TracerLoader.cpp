#include "TracerLoader.hpp"

#include <cstdio>


TracerLoader::~TracerLoader()
{
    if (m_TracedProcessInformation.hThread != NULL)
    {
        CloseHandle(m_TracedProcessInformation.hThread);
    }
    if (m_TracedProcessInformation.hProcess != NULL)
    {
        CloseHandle(m_TracedProcessInformation.hProcess);
    }
}

void TracerLoader::LoadConfig()
{
    static constexpr char configFileName[] = ".\\config.ini";

    GetPrivateProfileStringA("Config", "CommandLine", "", m_Config.CommandLine, sizeof(m_Config.CommandLine), configFileName);
    GetPrivateProfileStringA("Config", "CurrentDirectory", "", m_Config.CurrentDirectory, sizeof(m_Config.CurrentDirectory), configFileName);
}

bool TracerLoader::CreateTracedProcess()
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
        CREATE_SUSPENDED,
        nullptr,
        m_Config.CurrentDirectory,
        &startupInfo,
        &m_TracedProcessInformation
    ) == FALSE)
    {
        printf_s("Failed to create the traced process: %ld\n", GetLastError());
        return false;
    }

    return true;
}

bool TracerLoader::InjectTracerDll()
{
    static constexpr char tracerDllName[] = "tracer.dll";
    
    void* tracerDllNameAddress = VirtualAllocEx(
        m_TracedProcessInformation.hProcess,
        nullptr,
        sizeof(tracerDllName),
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );
    if (tracerDllNameAddress == nullptr)
    {
        printf_s("Failed to allocate tracer DLL name in the traced process: %ld\n", GetLastError());
        return false;
    }

    if (WriteProcessMemory(
        m_TracedProcessInformation.hProcess,
        tracerDllNameAddress,
        tracerDllName,
        sizeof(tracerDllName),
        nullptr
    ) == FALSE)
    {
        printf_s("Failed to write tracer DLL name into the traced process: %ld\n", GetLastError());
        return false;
    }
    
    HANDLE injectThread = CreateRemoteThread(
        m_TracedProcessInformation.hProcess,
        nullptr,
        0,
        reinterpret_cast<PTHREAD_START_ROUTINE>(LoadLibraryA), // This assumes that kernel32.dll is loaded at the same base address.
        tracerDllNameAddress,
        0,
        nullptr
    );
    if (injectThread == NULL)
    {
        printf_s("Failed to create a thread that injects the tracer DLL into the traced process: %ld\n", GetLastError());
        return false;
    }

    WaitForSingleObject(injectThread, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeThread(injectThread, &exitCode);
    CloseHandle(injectThread);
    
    if (exitCode == 0)
    {
        printf_s("Failed to inject the tracer DLL into the traced process.\n");
        return false;
    }

    return true;
}

bool TracerLoader::RunTracedProcess()
{
    CONTEXT threadContext =
    {
        .ContextFlags = CONTEXT_CONTROL,
    };
    
    if (GetThreadContext(m_TracedProcessInformation.hThread, &threadContext) == FALSE)
    {
        printf_s("Failed to get the main thread's context of the traced process: %ld\n", GetLastError());
        return false;
    }

    threadContext.EFlags |= 1 << 8; // Set the CPU trap flag.
    
    if (SetThreadContext(m_TracedProcessInformation.hThread, &threadContext) == FALSE)
    {
        printf_s("Failed to set the main thread's context of the traced process: %ld\n", GetLastError());
        return false;
    }
    
    if (ResumeThread(m_TracedProcessInformation.hThread) == -1)
    {
        printf_s("Failed to resume the main thread of the traced process: %ld\n", GetLastError());
        return false;
    }

    WaitForSingleObject(m_TracedProcessInformation.hProcess, INFINITE);

    return true;
}
