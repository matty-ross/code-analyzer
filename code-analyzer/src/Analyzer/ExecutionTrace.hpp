#pragma once


#include <cstdio>
#include <map>
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

    void OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo) override;
    void OnExceptionBreakpoint(EXCEPTION_POINTERS* exceptionInfo) override;

    bool IsAddressInModule(void* address) const;
    
    void LogExecutedInstruction(void* instructionAddress) const;

    void EnableTrapFlag(EXCEPTION_POINTERS* exceptionInfo);
    void DisableTrapFlag(EXCEPTION_POINTERS* exceptionInfo);
    void InstallBreakpoint(void* address);
    void UninstallBreakpoint(void* address);

private:
    Config m_Config = {};

    void* m_ModuleBaseAddress = nullptr;
    size_t m_ModuleSize = 0;

    std::map<void*, BYTE> m_Breakpoints;

    FILE* m_OutputFile = nullptr;
};
