#pragma once


#include <cstdio>
#include <Windows.h>
#include <Psapi.h>


struct Config
{
    bool SingleStep = false;
};


class CodeAnalyzer
{
public:
    static CodeAnalyzer& Get();

public:
    void OnProcessAttach();
    void OnProcessDetach();

private:
    void LoadConfig();

    void OnExceptionAccessViolation(EXCEPTION_POINTERS* exceptionInfo);
    void OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo);
    void OnExceptionGuardPage(EXCEPTION_POINTERS* exceptionInfo);

    void EnableSingleStepping(EXCEPTION_POINTERS* exceptionInfo);
    void DisableSingleStepping(EXCEPTION_POINTERS* exceptionInfo);

    void SetGuardPage();

private:
    static LONG CALLBACK TopLevelExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo);

private:
    static CodeAnalyzer s_Instance;

private:
    Config m_Config = {};
    
    MODULEINFO m_ModuleInfo = {};

    FILE* m_SingleStepFile = nullptr;
};
