#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <ws2tcpip.h>

LARGE_INTEGER get_query_frequency();
#define TIME_ECLAPSE(label, ...)                                             \
    do {                                                                      \
        LARGE_INTEGER _s, _e;                                                 \
        LARGE_INTEGER _f = get_query_frequency();                           \
        QueryPerformanceCounter(&_s);                                       \
        __VA_ARGS__;                                                          \
        QueryPerformanceCounter(&_e);                                         \
        fprintf(stdout, "[Time eclapse] %s took %.2f ms\n", (label),                \
               (double)(_e.QuadPart - _s.QuadPart) * 1000.0 /                 \
               (double) (_f).QuadPart);                                      \
    } while (0)