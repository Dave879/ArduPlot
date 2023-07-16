
#include "arduplot.h"

ArduPlot app;

#ifdef DEBUG
void *operator new(std ::size_t count)
{
    auto ptr = malloc(count);
    TracyAlloc(ptr, count);
    return ptr;
}
void operator delete(void *ptr) noexcept
{
    TracyFree(ptr);
    free(ptr);
}
#endif

int main()
{
    app.run();
    return 0;
}