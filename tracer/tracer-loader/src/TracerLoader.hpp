#pragma once


#include <Windows.h>


class TracerLoader
{
public:
    ~TracerLoader();

public:
    bool CreateTracedProcess();
    bool InjectTracerDll();

private:
    PROCESS_INFORMATION m_TracedProcessInformation = {};
};
