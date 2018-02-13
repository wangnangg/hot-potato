#pragma once
#include <stddef.h>
typedef struct
{
    int remain_hops;
    int trace_size;
    int trace[0];
} potato;

int potato_size(int hops);
