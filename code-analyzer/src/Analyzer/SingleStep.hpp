#pragma once


#include <cstdio>
#include <Windows.h>
#include <Psapi.h>


namespace Analyzer
{
    class SingleStep
    {
    public:
        SingleStep();
        ~SingleStep();

    public:
        void OnExceptionAccessViolation(EXCEPTION_POINTERS* exceptionInfo);
        void OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo);

    private:
        void ChangeModulePagesProtection(DWORD newProtection);

    private:
        MODULEINFO m_ModuleInfo = {};

        FILE* m_AddressesFile = nullptr;
    };
}
