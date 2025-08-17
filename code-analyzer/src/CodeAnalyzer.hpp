#pragma once


#include <Windows.h>


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

private:
    static LONG CALLBACK TopLevelExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo);

private:
    static CodeAnalyzer s_Instance;
};
