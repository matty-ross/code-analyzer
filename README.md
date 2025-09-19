# Code Analyzer

![](https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)
![](https://img.shields.io/badge/Visual%20Studio-5C2D91?style=for-the-badge&logo=visual-studio&logoColor=white)
![](https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)

Tools to analyze program behavior.


## Tracer

This tool traces the program execution and stores the trace in a file that can be analyzed later.
You must modify your executable so it loads the DLL and sets the CPU trap flag.

### x86

```asm
; define the DLL name
dll_name:
    db 'tracer.dll', 0

; new entry point
start:
    ; load the DLL
    push offset dll_name
    call dword ptr [LoadLibraryA]
    
    ; set CPU trap flag
    pushfd
    or dword ptr [esp], 0x100
    popfd
    
    ; jump to the original entry point
    jmp <original_entry_point>
```

### x64

```asm
; define the DLL name
dll_name:
    db 'tracer.dll', 0

; new entry point
start:
    ; load the DLL
    sub rsp, 0x28
    lea rcx, offset dll_name
    call qword ptr [LoadLibraryA]
    add rsp, 0x28
    
    ; set CPU trap flag
    pushfq
    or qword ptr [rsp], 0x100
    popfq
    
    ; jump to the original entry point
    jmp <original_entry_point>
```
