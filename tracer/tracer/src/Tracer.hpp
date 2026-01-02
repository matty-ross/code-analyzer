#pragma once


#include <cstdlib>
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
    void OnProcessExited(const EXIT_PROCESS_DEBUG_INFO& exitProcessInfo);
    void OnException(const EXCEPTION_RECORD& exceptionRecord);

    void OnExceptionBreakpoint(const EXCEPTION_RECORD& exceptionRecord);
    void OnExceptionSingleStep(const EXCEPTION_RECORD& exceptionRecord);

    bool IsAddressInMainModule(const void* address) const;
    uintptr_t GetReturnAddress(const CONTEXT& context) const;
    void LogExecutedInstruction(const void* address, const CONTEXT& context) const;

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
    bool m_Tracing = false;

    PROCESS_INFORMATION m_ProcessInformation = {};
    MODULEINFO m_MainModuleInformation = {};
    bool m_ProcessExited = false;
    bool m_LoaderBreakpointHit = false;
};
