#include <Windows.h>

#include "CodeAnalyzer.hpp"


BOOL WINAPI DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD     fdwReason,
    _In_ LPVOID    lpvReserved
)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        CodeAnalyzer::Get().OnProcessAttach();
        break;

    case DLL_PROCESS_DETACH:
        CodeAnalyzer::Get().OnProcessDetach();
        break;
    }
    
    return TRUE;
}
