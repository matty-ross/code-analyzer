#include "SingleStep.hpp"


static constexpr char k_AddressesFileName[] = "addresses.bin";

static constexpr DWORD k_TrapFlag = 1 << 8;


namespace Analyzer
{
    SingleStep::SingleStep()
    {
        GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(nullptr), &m_ModuleInfo, sizeof(m_ModuleInfo));

        fopen_s(&m_AddressesFile, k_AddressesFileName, "wb");
    }

    SingleStep::~SingleStep()
    {
        fclose(m_AddressesFile);
    }
    
    void SingleStep::OnExceptionAccessViolation(EXCEPTION_POINTERS* exceptionInfo)
    {
        ChangeModulePagesProtection(PAGE_EXECUTE_READWRITE);
        exceptionInfo->ContextRecord->EFlags |= k_TrapFlag;
    }
    
    void SingleStep::OnExceptionSingleStep(EXCEPTION_POINTERS* exceptionInfo)
    {
        DWORD eip = exceptionInfo->ContextRecord->Eip;
        fwrite(&eip, sizeof(eip), 1, m_AddressesFile);
        
        exceptionInfo->ContextRecord->EFlags &= ~k_TrapFlag;
        ChangeModulePagesProtection(PAGE_READWRITE);
    }
    
    void SingleStep::ChangeModulePagesProtection(DWORD newProtection)
    {
        DWORD oldProtection = 0;
        VirtualProtect(m_ModuleInfo.lpBaseOfDll, m_ModuleInfo.SizeOfImage, newProtection, &oldProtection);
    }
}
