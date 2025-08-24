#pragma once


#include <cstdio>
#include <Windows.h>

#include "Analyzer.hpp"


class ExecutionTrace : public Analyzer
{
private:
    void OnProcessAttach() override;
    void OnProcessDetach() override;

    void OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo) override;
    void OnExceptionBreakpoint(EXCEPTION_POINTERS* exceptionInfo) override;

    bool IsAddressInModule(void* address) const;
    
    void LogExecutedInstruction(void* instructionAddress) const;
    void LogExternalCall(void* instructionAddress) const;

private:
    void* m_ModuleBaseAddress = nullptr;
    size_t m_ModuleSize = 0;

    FILE* m_OutputFile = nullptr;
};
