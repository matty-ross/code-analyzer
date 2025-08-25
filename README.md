# Code Analyzer

![](https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)
![](https://img.shields.io/badge/Visual%20Studio-5C2D91?style=for-the-badge&logo=visual-studio&logoColor=white)
![](https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)

A DLL that can be injected into a process and analyze its behavior.


## Usage
1. Edit `config.ini` with appropriate settings
1. Create `output` directory
1. Inject the DLL into a target process
1. Observe the output file

## Analyzers
- `ExecutionTrace` - log every executed instruction
- `MemoryAccess` - log every static data access

## Features
- Support for 32-bit and 64-bit processes
- Analyze only a section of code
