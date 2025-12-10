#include "TracerLoader.hpp"


int main()
{
    TracerLoader tracerLoader;

    tracerLoader.CreateTracedProcess();
    tracerLoader.InjectTracerDll();
    tracerLoader.RunTracedProcess();
    
    return 0;
}
