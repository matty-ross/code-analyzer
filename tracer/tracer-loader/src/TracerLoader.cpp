#include "TracerLoader.hpp"

#include <cstdio>


TracerLoader::~TracerLoader()
{
    if (m_TracedProcessInformation.hThread)
    {
        CloseHandle(m_TracedProcessInformation.hThread);
    }
    if (m_TracedProcessInformation.hProcess)
    {
        CloseHandle(m_TracedProcessInformation.hProcess);
    }
}

bool TracerLoader::CreateTracedProcess()
{
    STARTUPINFOA startupInfo = {};
    startupInfo.cb = sizeof(STARTUPINFOA);
    
    // TODO: fill some parameters
    if (CreateProcessA(
        "",
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        CREATE_SUSPENDED,
        nullptr,
        nullptr,
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
        reinterpret_cast<PTHREAD_START_ROUTINE>(LoadLibraryA), // This assumes that kernel32.dll is loaded at the same base address
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

    // TODO: check the thread exit code to determine if the DLL got injected successfully

    CloseHandle(injectThread);

    return true;
}
