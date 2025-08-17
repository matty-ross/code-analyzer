#pragma once


#include <Windows.h>


class Analyzer
{
public:
    virtual void OnProcessAttach() {}
    virtual void OnProcessDetach() {}

    virtual void OnExceptionAccessViolation(EXCEPTION_POINTERS* exceptionInfo) {}
    virtual void OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo) {}
    virtual void OnExceptionGuardPage(EXCEPTION_POINTERS * exceptionInfo) {}
};
