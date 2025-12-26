#pragma once


#include <Windows.h>


struct Config
{
    char CommandLine[1024] = "";
    char CurrentDirectory[MAX_PATH] = "";
    DWORD StartRVA = 0;
    DWORD EndRVA = 0;
};


class TracerLoader
{
public:
    ~TracerLoader();

public:
    bool LoadConfig();
    bool CreateTracedProcess();
    bool InjectTracerDll();
    bool RunTracedProcess();

private:
    Config m_Config = {};
    PROCESS_INFORMATION m_TracedProcessInformation = {};
};
