#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ws2tcpip.h>

static void get_log_filename(char *buffer, size_t buffer_size)
{
    SYSTEMTIME now;

    GetLocalTime(&now);

    LPCTSTR folderPath = TEXT("log");

    if (CreateDirectory(folderPath, NULL)) {
    } else {
        DWORD error = GetLastError();
        if (error == ERROR_ALREADY_EXISTS) {
            //printf("Directory already exists.\n");
        } else {
            //printf("Failed to create directory. Error code: %lu\n", error);
        }
    }

    snprintf(
        buffer,
        buffer_size,
        "%s/toralize_%04u_%02u_%02u_%02u_%02u.log",
        folderPath,
        (unsigned)now.wYear,
        (unsigned)now.wMonth,
        (unsigned)now.wDay,
        (unsigned)now.wHour,
        (unsigned)now.wMinute
    );
}

#ifdef DEBUG_FLAG
static void set_console_color(
    HANDLE console,
    WORD color,
    WORD *original_color
)
{
    CONSOLE_SCREEN_BUFFER_INFO info;

    *original_color = 0;

    if (console == NULL ||
        console == INVALID_HANDLE_VALUE) {
        return;
    }

    if (GetConsoleScreenBufferInfo(console, &info)) {
        *original_color = info.wAttributes;
        SetConsoleTextAttribute(console, color);
    }
}

static void restore_console_color(
    HANDLE console,
    WORD original_color
)
{
    if (console != NULL &&
        console != INVALID_HANDLE_VALUE &&
        original_color != 0) {

        SetConsoleTextAttribute(console, original_color);
    }
}
#endif

void log_printf(FILE *stream, const char *format, ...)
{
    static char log_filename[MAX_PATH] = {0};

    SYSTEMTIME now;
    char message[MAX_LOG_PRINT_SIZE];
    const char *stream_name;

    int prefix_length;
    int body_length;
    int total_length;

    va_list args;

    HANDLE log_file;
    DWORD written;

#ifdef DEBUG_FLAG
    HANDLE console_handle;

    WORD original_color;
    WORD output_color;
#endif

    if (stream == NULL || format == NULL) {
        return;
    }

#ifdef DEBUG_FLAG
    /*
     * Determine output type and color.
     */
    if (stream == stderr) {
        stream_name = "stderr";
        console_handle = GetStdHandle(STD_ERROR_HANDLE);

        output_color =
            FOREGROUND_RED |
            FOREGROUND_INTENSITY;
    } else {
        stream_name = "stdout";
        console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

        output_color =
            FOREGROUND_GREEN |
            FOREGROUND_INTENSITY;
    }
#else
    stream_name = (stream == stderr) ? "stderr" : "stdout";
#endif
    GetLocalTime(&now);

    /*
     * Generate one log filename per process.
     */
    if (log_filename[0] == '\0') {
        get_log_filename(
            log_filename,
            sizeof(log_filename)
        );
    }

    /*
     * Example:
     * {2026_07_17_23_06_22, stdout}: [toralize]
     */
    prefix_length = snprintf(
        message,
        sizeof(message),
        "{%04u_%02u_%02u_%02u_%02u_%02u, %s}: ",
        (unsigned)now.wYear,
        (unsigned)now.wMonth,
        (unsigned)now.wDay,
        (unsigned)now.wHour,
        (unsigned)now.wMinute,
        (unsigned)now.wSecond,
        stream_name
    );

    if (prefix_length < 0 ||
        (size_t)prefix_length >= sizeof(message)) {
        return;
    }

    /*
     * Append the caller's formatted message.
     */
    va_start(args, format);

    body_length = vsnprintf(
        message + prefix_length,
        sizeof(message) - (size_t)prefix_length,
        format,
        args
    );

    va_end(args);

    if (body_length < 0) {
        return;
    }

    if ((size_t)body_length >=
        sizeof(message) - (size_t)prefix_length) {

        total_length = (int)sizeof(message) - 1;
    } else {
        total_length = prefix_length + body_length;
    }

    /*
     * Print colored output to stdout or stderr.
     */
#ifdef DEBUG_FLAG
    set_console_color(console_handle, output_color, &original_color);

    fwrite(message, 1, (size_t)total_length, stream);
    fwrite("\n", 1, 1, stream);
    fflush(stream);

    restore_console_color(console_handle, original_color);
#endif
    /*
     * Append the same message to the log file.
     * ANSI color codes are not written to the file.
     */
    log_file = CreateFileA(
        log_filename,
        FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (log_file == INVALID_HANDLE_VALUE) {
        return;
    }

    WriteFile(
        log_file,
        message,
        (DWORD)total_length,
        &written,
        NULL
    );

    WriteFile(
        log_file,
        "\r\n",
        2,
        &written,
        NULL
    );

    CloseHandle(log_file);
}

int receive_full(SOCKET s, char *buffer_response, const int response_size) {
    LOG_PRINTF(stdout, "[receive_full] receive init");
    int total_received = 0;
    while (total_received < response_size) {
        int result = recv(s, buffer_response + total_received,        // append, not overwrite
                          response_size - total_received, 0);
        if (result > 0) {
            total_received += result;
        }
        else if (result == 0) {
            LOG_PRINTF(stderr, "[receive_full] peer closed early");
            return -1;    // peer closed early
        }
        else {
            LOG_PRINTF(stderr, "[receive_full] recv error");
            return -1;    // recv error — caller checks WSAGetLastError() if it wants
        }
    }

    LOG_PRINTF(stdout, "[receive_full] receive close, received: %d bytes", total_received);
    return total_received;
}
