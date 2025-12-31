#pragma once


#include <cstdio>
#include <Windows.h>
#include <Psapi.h>


struct Config
{
    char CommandLine[1024];
    char CurrentDirectory[MAX_PATH];
    DWORD StartBreakpointRVA;
    DWORD EndBreakpointRVA;
};


class Tracer
{
public:
    ~Tracer();

public:
    bool LoadConfig();
    bool CreateTracedProcess();
    void DebugTracedProcess();

private:
    void OnProcessCreated(const CREATE_PROCESS_DEBUG_INFO& createProcessInfo);
    void OnProcessExited();
    void OnException();

    static void ContextEnableStartBreakpoint(CONTEXT& context, uintptr_t address);
    static void ContextDisableStartBreakpoint(CONTEXT& context);
    static bool ContextIsStartBreakpointHit(const CONTEXT& context);
    static void ContextEnableEndBreakpoint(CONTEXT& context, uintptr_t address);
    static void ContextDisableEndBreakpoint(CONTEXT& context);
    static bool ContextIsEndBreakpointHit(const CONTEXT& context);
    static void ContextEnableReturnBreakpoint(CONTEXT& context, uintptr_t address);
    static void ContextDisableReturnBreakpoint(CONTEXT& context);
    static bool ContextIsReturnBreakpointHit(const CONTEXT& context);
    static void ContextEnableSingleStepping(CONTEXT& context);
    static void ContextDisableSingleStepping(CONTEXT& context);

private:
    Config m_Config = {};

    FILE* m_TraceFile = nullptr;

    bool m_ProcessRunning = false;
    PROCESS_INFORMATION m_ProcessInformation = {};
    MODULEINFO m_MainModuleInformation = {};
};
