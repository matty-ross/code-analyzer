#pragma once


#include <cstdio>
#include <Windows.h>
#include <Psapi.h>


class CodeAnalyzer
{
public:
    static CodeAnalyzer& Get();

public:
    void OnProcessAttach();
    void OnProcessDetach();

private:
    void OnExceptionAccessViolation(EXCEPTION_POINTERS* exceptionInfo);
    void OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo);
    void OnExceptionGuardPage(EXCEPTION_POINTERS* exceptionInfo);

    void* GetAddressInDuplicatedModule(void* originalModuleAddress) const;
    void* GetAddressInOriginalModule(void* duplicatedModuleAddress) const;

    void DisableMemoryAccess();
    void EnableMemoryAccess();
    void EnableSingleStepping(EXCEPTION_POINTERS* exceptionInfo);
    void DisableSingleStepping(EXCEPTION_POINTERS* exceptionInfo);
    void SetGuardPage();

private:
    static LONG CALLBACK TopLevelExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo);

private:
    static CodeAnalyzer s_Instance;

private:
    MODULEINFO m_ModuleInfo = {};

    void* m_DuplicatedModule = nullptr;

    FILE* m_MemoryAccessFile = nullptr;
    FILE* m_SingleStepFile = nullptr;
};
