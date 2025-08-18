#pragma once


#include <cstdio>
#include <Windows.h>

#include "Analyzer.hpp"


class MemoryAccess : public Analyzer
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

    void EnableModuleMemoryAccess();
    void DisableModuleMemoryAccess();
    void EnableTrapFlag(EXCEPTION_POINTERS* exceptionInfo);
    void DisableTrapFlag(EXCEPTION_POINTERS* exceptionInfo);
    
    void CopyModuleToDuplicatedModule();
    void* TranslateModuleAddressToDuplicatedModuleAddress(void* address) const;
    void* TranslateDuplicatedModuleAddressToModuleAddress(void* address) const;
    bool IsAddressInModule(void* address);
    bool IsAddressInDuplicatedModule(void* address);

private:
    Config m_Config = {};

    void* m_ModuleBaseAddress = nullptr;
    void* m_DuplicatedModuleBaseAddress = nullptr;
    size_t m_ModuleSize = 0;

    bool m_NeedToCopyModuleToDuplicatedModule = false;

    FILE* m_OutputFile = nullptr;
};
