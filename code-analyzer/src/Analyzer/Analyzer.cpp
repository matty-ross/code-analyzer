#include "Analyzer.hpp"


Analyzer::Config& Analyzer::GetConfig()
{
    return m_Config;
}

void Analyzer::SetTrapFlag(CONTEXT* context)
{
    context->EFlags |= 0x100;
}
    
void Analyzer::ClearTrapFlag(CONTEXT* context)
{
    context->EFlags &= ~0x100;
}
    
void Analyzer::AddBreakpoint(void* address)
{
    if (!m_Breakpoints.contains(address))
    {
        m_Breakpoints[address] = *static_cast<BYTE*>(address);
        *static_cast<BYTE*>(address) = 0xCC;
    }
}

void Analyzer::RemoveBreakpoint(void* address)
{
    if (m_Breakpoints.contains(address))
    {
        *static_cast<BYTE*>(address) = m_Breakpoints[address];
        m_Breakpoints.erase(address);
    }
}
