# Code Analyzer

![](https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)
![](https://img.shields.io/badge/Visual%20Studio-5C2D91?style=for-the-badge&logo=visual-studio&logoColor=white)
![](https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![](https://img.shields.io/badge/Python-3670A0?style=for-the-badge&logo=python&logoColor=FFDD54)

Tools to analyze program behavior.


## Tools

### Tracer Loader

A simple program that creates a new traced process and injects the tracer DLL into it.

### Tracer

A DLL that traces the program execution and stores the trace in a file that can be analyzed later.


## Usage

1. Create a directory for analysis (eg. `C:\analysis`)
1. Copy there your executable that you want to analyze (eg. `program.exe`)
1. Copy there the `tracer-loader.exe` and `tracer.dll` files (either 32-bit or 64-bit depending on your analyzed executable)
1. Create a `config.ini` file:
    ```ini
    [Config]
    CommandLine="C:\analysis\program.exe --argument=123"
    CurrentDirectory="C:\analysis"
    ```
1. Run `tracer-loader.exe`
