#pragma once


#include <cstdio>
#include <Windows.h>

#include "Analyzer.hpp"


class ExecutionTrace : public Analyzer
{
private:
    struct Config
    {
        void* StartAddress = nullptr;
        void* EndAddress = nullptr;
    };

private:
    void LoadConfig(const char* fileName) override;
    
    void OnProcessAttach() override;
    void OnProcessDetach() override;

    void OnExceptionAccessViolation(EXCEPTION_POINTERS* exceptionInfo) override;
    void OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo) override;

    void EnableModuleExecutable();
    void DisableModuleExecutable();
    void EnableTrapFlag(EXCEPTION_POINTERS* exceptionInfo);
    void DisableTrapFlag(EXCEPTION_POINTERS* exceptionInfo);

private:
    Config m_Config = {};

    void* m_ModuleBaseAddress = nullptr;
    size_t m_ModuleSize = 0;

    FILE* m_OutputFile = nullptr;
};
