#include <cstdlib>

#include "TracerLoader.hpp"


int main()
{
    TracerLoader tracerLoader;
    tracerLoader.LoadConfig();

    if (!tracerLoader.CreateTracedProcess())
    {
        return EXIT_FAILURE;
    }
    if (!tracerLoader.InjectTracerDll())
    {
        return EXIT_FAILURE;
    }
    if (!tracerLoader.RunTracedProcess())
    {
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
