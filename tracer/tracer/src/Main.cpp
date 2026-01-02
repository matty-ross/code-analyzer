#include "Tracer.hpp"


int main()
{
    Tracer tracer;

    if (!tracer.LoadConfig())
    {
        return EXIT_FAILURE;
    }
    if (!tracer.CreateTracedProcess())
    {
        return EXIT_FAILURE;
    }

    tracer.DebugTracedProcess();

    return EXIT_SUCCESS;
}
