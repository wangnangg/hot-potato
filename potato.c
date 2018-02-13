#include "potato.h"

int potato_size(int hops)
{
    return offsetof(potato, trace) + hops * sizeof(int);
}
