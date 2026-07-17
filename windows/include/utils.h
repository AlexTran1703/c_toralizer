#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <ws2tcpip.h>
#include <string.h>

#define MAX_LOG_PRINT_SIZE 1024
#define LOG_PRINTF(stdio, ...) log_printf(stdio, __VA_ARGS__)

void log_printf(FILE *, const char *format, ...);
int receive_full(SOCKET s, char *buffer_response, const int response_size);