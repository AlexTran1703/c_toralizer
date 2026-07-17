#include <stdio.h>
#include <stdlib.h>
#include "src/toralize.h"

//Win Socket
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
/*
    toralize IP <192.16.23.1> Port <80>
*/

_proxy_request* init_proxy_request(const char *dstip, const int dstport) {
    _proxy_request* p_r;
    p_r = (_proxy_request *) malloc(REQUEST_SIZE);
    
    p_r->vn = 4;
    p_r->cn = 1;
    p_r->dstport = htons((byte16) dstport);
    inet_pton(AF_INET, dstip, &p_r->dstip);
    strncpy_s((char *) p_r->userid, 8,  (const char *) PROXY_NAME, 8);
    return p_r;
}

_proxy_response* init_proxy_response() {
    _proxy_response* p_r;
    p_r = (_proxy_response *) malloc(RESPONSE_SIZE);

    return p_r;
}

int main(int argc, const char *argv[]) {
    
    WSADATA wsa_data;
    SOCKET sock = INVALID_SOCKET;
    SOCKADDR_IN server_addr;
    
    LARGE_INTEGER frequency;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    QueryPerformanceFrequency(&frequency);
    double elapsed_ms;
    /* Bounding insufficient input*/
    if (argc < 3) {
        fprintf(stderr, "Usage toralize: toralize <IP> <PORT>\r\n");
        return EXIT_FAILURE;
    }

    /* Initialize Winsock 2.2. */
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);

    if (result != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", result);
        return EXIT_FAILURE;
    }

    char *host = (char *) argv[1];
    int port = atoi(argv[2]);

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "socket failed: %d\n",
                WSAGetLastError());

        WSACleanup();
        return EXIT_FAILURE;
    }

    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;

    result = inet_pton(AF_INET, (const char *) host, &server_addr.sin_addr);

    if (result != 1) {
        fprintf(stderr, "Invalid address\n");
        closesocket(sock);
        WSACleanup();
        return EXIT_FAILURE;
    }


    QueryPerformanceCounter(&start);
    result = connect(sock, (const SOCKADDR *) &server_addr, sizeof(server_addr));
    QueryPerformanceCounter(&end);

    elapsed_ms =
        (double)(end.QuadPart - start.QuadPart) * 1000.0 /
        (double)frequency.QuadPart;

    printf("[Time eclapsed] connect took %.2f ms\n", elapsed_ms);

    if (result == SOCKET_ERROR) {
        fprintf(stderr, "connect function failed with error: %d\n", WSAGetLastError());
        result = closesocket(sock);
        if (result == SOCKET_ERROR)
            fprintf(stderr, "closesocket function failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return EXIT_FAILURE;
    }

    _proxy_request* req;
    /*Init request to proxy*/
    req = init_proxy_request((const char *) host, (const int)  port);
    result = send(sock, (const char *) req, (int) REQUEST_SIZE - 1, 0);

    if (result == SOCKET_ERROR) {
        fprintf(stderr,
                "send failed: %d\n",
                WSAGetLastError());
    } 

    char buffer_response[RESPONSE_SIZE] = {0};
    int total_received = 0;

    QueryPerformanceCounter(&start);
    while(total_received < RESPONSE_SIZE) {
        result = recv(sock, buffer_response, (int) RESPONSE_SIZE - total_received, 0);
            
        if (result > 0) {
            total_received += result;
        }
        else if (result == 0) {
            fprintf(
                stderr,
                "Proxy closed the connection before sending a complete response.\n"
            );
            QueryPerformanceCounter(&end);

            elapsed_ms =
                (double)(end.QuadPart - start.QuadPart) * 1000.0 /
                (double)frequency.QuadPart;

            printf("[Time eclapsed] receive took %.2f ms\n", elapsed_ms);
            closesocket(sock);
            WSACleanup();
            return EXIT_FAILURE;
        }
        else {
            fprintf(
                stderr,
                "recv failed: %d\n",
                WSAGetLastError()
            );
            return EXIT_FAILURE;
        }
    }

    QueryPerformanceCounter(&end);

    elapsed_ms =
        (double)(end.QuadPart - start.QuadPart) * 1000.0 /
        (double)frequency.QuadPart;

    printf("[Time eclapsed] receive took %.2f ms\n", elapsed_ms);

    _proxy_response* res = (_proxy_response*) buffer_response;
    if (res->cn != REQUEST_GRANTED) {
        fprintf(stderr, "Unable to tranverse the proxy, error code: %d\r\n", res->cn);
        closesocket(sock);
        WSACleanup();
        return EXIT_FAILURE;
    }
    else {
        fprintf(stdout, "Successfully tranverse the proxy via host: %s, port %d\r\n", host, port);
    }

    closesocket(sock);
    WSACleanup();

    return EXIT_SUCCESS;
}