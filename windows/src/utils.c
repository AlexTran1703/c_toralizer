#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ws2tcpip.h>

static bool _init_query_frequency = false;
LARGE_INTEGER _query_frequency;
LARGE_INTEGER get_query_frequency() {
    if (!_init_query_frequency) {
        QueryPerformanceFrequency(&_query_frequency);
        _init_query_frequency = true;
    }
    return _query_frequency;
}