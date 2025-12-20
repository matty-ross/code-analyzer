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

bool TracerLoader::LoadConfig()
{
    static constexpr char configFileName[] = ".\\config.ini";

    if (GetFileAttributesA(configFileName) == INVALID_FILE_ATTRIBUTES)
    {
        printf_s("Failed to load the tracer loader config because the file \"%s\" doesn't exist.\n", configFileName);
        return false;
    }

    GetPrivateProfileStringA("Config", "CommandLine", "", m_Config.CommandLine, sizeof(m_Config.CommandLine), configFileName);
    GetPrivateProfileStringA("Config", "CurrentDirectory", "", m_Config.CurrentDirectory, sizeof(m_Config.CurrentDirectory), configFileName);

    printf_s(
        "Loaded the tracer loader config.\n"
        "    - CommandLine: \"%s\"\n"
        "    - CurrentDirectory: \"%s\"\n",
        m_Config.CommandLine,
        m_Config.CurrentDirectory
    );

    return true;
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
        printf_s("Failed to create the traced process: %ld.\n", GetLastError());
        return false;
    }
    else
    {
        printf_s(
            "Created the traced process.\n"
            "    - Process ID: %lu\n"
            "    - Main thread ID: %lu\n",
            m_TracedProcessInformation.dwProcessId,
            m_TracedProcessInformation.dwThreadId
        );
    }

    return true;
}

bool TracerLoader::InjectTracerDll()
{
    static constexpr char tracerDllName[] = ".\\tracer.dll";
    
    void* tracerDllNameAddress = VirtualAllocEx(
        m_TracedProcessInformation.hProcess,
        nullptr,
        sizeof(tracerDllName),
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );
    if (tracerDllNameAddress == nullptr)
    {
        printf_s("Failed to allocate tracer DLL name in the traced process: %ld.\n", GetLastError());
        return false;
    }
    else
    {
        printf_s("Allocated tracer DLL name in the traced process at address 0x%p.\n", tracerDllNameAddress);
    }

    if (WriteProcessMemory(
        m_TracedProcessInformation.hProcess,
        tracerDllNameAddress,
        tracerDllName,
        sizeof(tracerDllName),
        nullptr
    ) == FALSE)
    {
        printf_s("Failed to write tracer DLL name into the traced process: %ld.\n", GetLastError());
        return false;
    }
    else
    {
        printf_s("Wrote tracer DLL name into the traced process.\n");
    }
    
    DWORD injectThreadId = 0;
    HANDLE injectThread = CreateRemoteThread(
        m_TracedProcessInformation.hProcess,
        nullptr,
        0,
        reinterpret_cast<PTHREAD_START_ROUTINE>(LoadLibraryA), // This assumes that kernel32.dll is loaded at the same base address.
        tracerDllNameAddress,
        0,
        &injectThreadId
    );
    if (injectThread == NULL)
    {
        printf_s("Failed to create a thread that injects the tracer DLL into the traced process: %ld.\n", GetLastError());
        return false;
    }
    else
    {
        printf_s(
            "Created a thread that injects the tracer DLL into the traced process.\n"
            "    - Thread ID: %lu\n",
            injectThreadId
        );
    }

    WaitForSingleObject(injectThread, INFINITE);
    printf_s("The thread that injects the tracer DLL into the traced process has exited.\n");

    DWORD exitCode = 0;
    GetExitCodeThread(injectThread, &exitCode);
    CloseHandle(injectThread);
    
    if (exitCode == 0)
    {
        printf_s("Failed to inject the tracer DLL into the traced process.\n");
        return false;
    }
    else
    {
        printf_s("Injected the tracer DLL into the traced process.\n");
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
        printf_s("Failed to get the main thread's context of the traced process: %ld.\n", GetLastError());
        return false;
    }
    else
    {
        printf_s("Got the main thread's context of the traced process.\n");
    }

    threadContext.EFlags |= 0x100; // Set the CPU trap flag.
    
    if (SetThreadContext(m_TracedProcessInformation.hThread, &threadContext) == FALSE)
    {
        printf_s("Failed to set the main thread's context of the traced process: %ld.\n", GetLastError());
        return false;
    }
    else
    {
        printf_s("Set the main thread's context of the traced process.\n");
    }
    
    if (ResumeThread(m_TracedProcessInformation.hThread) == -1)
    {
        printf_s("Failed to resume the main thread of the traced process: %ld.\n", GetLastError());
        return false;
    }
    else
    {
        printf_s("Resumed the main thread of the traced process.\n");
    }

    WaitForSingleObject(m_TracedProcessInformation.hProcess, INFINITE);
    printf_s("The traced process has exited.\n");

    return true;
}
