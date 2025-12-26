#include "TracerLoader.hpp"

#include <cstdio>
#include <Psapi.h>


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
    static constexpr char sectionName[] = "Config";

    if (GetFileAttributesA(configFileName) == INVALID_FILE_ATTRIBUTES)
    {
        printf_s("Failed to load the tracer loader config because the file \"%s\" doesn't exist.\n", configFileName);
        return false;
    }

    GetPrivateProfileStringA(sectionName, "CommandLine", "", m_Config.CommandLine, sizeof(m_Config.CommandLine), configFileName);
    GetPrivateProfileStringA(sectionName, "CurrentDirectory", "", m_Config.CurrentDirectory, sizeof(m_Config.CurrentDirectory), configFileName);
    
    char startRvaBuffer[9] = {};
    GetPrivateProfileStringA(sectionName, "StartRVA", "FFFFFFFF", startRvaBuffer, sizeof(startRvaBuffer), configFileName);
    sscanf_s(startRvaBuffer, "%08X", &m_Config.StartRVA);
    
    char endRvaBuffer[9] = {};
    GetPrivateProfileStringA(sectionName, "EndRVA", "FFFFFFFF", endRvaBuffer, sizeof(endRvaBuffer), configFileName);
    sscanf_s(endRvaBuffer, "%08X", &m_Config.EndRVA);

    printf_s(
        "Loaded the tracer loader config.\n"
        "    - CommandLine: \"%s\"\n"
        "    - CurrentDirectory: \"%s\"\n"
        "    - StartRVA: 0x%08X\n"
        "    - EndRVA: 0x%08X\n",
        m_Config.CommandLine,
        m_Config.CurrentDirectory,
        m_Config.StartRVA,
        m_Config.EndRVA
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
    HMODULE mainModule = NULL; // The first module in the array is the main module, hence no array.
    DWORD bytesNeeded = 0;
    if (EnumProcessModules(
        m_TracedProcessInformation.hProcess,
        &mainModule,
        sizeof(mainModule),
        &bytesNeeded
    ) == FALSE)
    {
        printf_s("Failed to get the main module of the traced process: %ld.\n", GetLastError());
        return false;
    }
    else
    {
        printf_s("Got the main module of the traced process.\n");
    }

    MODULEINFO mainModuleInformation = {};
    if (GetModuleInformation(
        m_TracedProcessInformation.hProcess,
        mainModule,
        &mainModuleInformation,
        sizeof(mainModuleInformation)
    ) == FALSE)
    {
        printf_s("Failed to get the main module information from the traced process: %ld.\n", GetLastError());
        return false;
    }
    else
    {
        printf_s("Got the main module information from the traced process.\n");
    }

    CONTEXT mainThreadContext =
    {
        .ContextFlags = CONTEXT_DEBUG_REGISTERS,
    };
    if (m_Config.StartRVA != -1)
    {
        mainThreadContext.Dr0 = reinterpret_cast<uintptr_t>(mainModuleInformation.lpBaseOfDll) + m_Config.StartRVA; // set start breakpoint
        mainThreadContext.Dr7 |= (1 << 0) | (1 << 8); // enable start breakpoint
    }
    if (m_Config.EndRVA != -1)
    {
        mainThreadContext.Dr1 = reinterpret_cast<uintptr_t>(mainModuleInformation.lpBaseOfDll) + m_Config.EndRVA; // set end breakpoint
        mainThreadContext.Dr7 |= (1 << 2) | (1 << 8); // enable end breakpoint
    }
    if (SetThreadContext(m_TracedProcessInformation.hThread, &mainThreadContext) == FALSE)
    {
        printf_s("Failed to set breakpoints for the main thread of the traced process: %ld.\n", GetLastError());
        return false;
    }
    else
    {
        printf_s("Set breakpoints for the main thread of the traced process.\n");
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
