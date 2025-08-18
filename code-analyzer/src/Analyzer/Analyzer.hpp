#pragma once


#include <Windows.h>


class Analyzer
{
public:
    virtual void LoadConfig(const char* fileName) {}
    
    virtual void OnProcessAttach() {}
    virtual void OnProcessDetach() {}

    virtual void OnExceptionAccessViolation(EXCEPTION_POINTERS* exceptionInfo) {}
    virtual void OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo) {}
    virtual void OnExceptionBreakpoint(EXCEPTION_POINTERS* exceptionInfo) {}
};
