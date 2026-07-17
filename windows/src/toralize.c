#include <stdio.h>
#include <stdlib.h>
#include "toralize.h"
#include "utils.h"
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
    p_r->dstport = htons(dstport);
    inet_pton(AF_INET, dstip, &p_r->dstip);
    strncpy_s((char *) p_r->userid, 8,  (const char *) PROXY_NAME, 8);
    return p_r;
}

_proxy_response* init_proxy_response() {
    _proxy_response* p_r;
    p_r = (_proxy_response *) malloc(RESPONSE_SIZE);

    return p_r;
}

//result = recv(sock, buffer_response, (int) RESPONSE_SIZE - total_received, 0);
int receive_full(SOCKET s, char *buffer_response, const int response_size) {
    int total_received = 0;
    int result;
    while (
        total_received < response_size 
    )
    {
        result = recv(s, buffer_response, (int) response_size - total_received, 0);
        if (result > 0) {
            total_received += result;
        }
        else if (result == 0) {
            fprintf(
                stderr,
                "Proxy closed the connection before sending a complete response.\n"
            );
            closesocket(s);
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
    return total_received;
}

int main(int argc, const char *argv[]) {
    
    WSADATA wsa_data;
    SOCKET sock = INVALID_SOCKET;
    SOCKADDR_IN server_addr;
    
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

    server_addr.sin_port = htons(PROXY_PORT);
    server_addr.sin_family = AF_INET;

    result = inet_pton(AF_INET, (const char *) PROXY_HOST, &server_addr.sin_addr);

    if (result != 1) {
        fprintf(stderr, "Invalid address\n");
        closesocket(sock);
        WSACleanup();
        return EXIT_FAILURE;
    }   

    TIME_ECLAPSE("connect",
    result = connect(sock, (const SOCKADDR *) &server_addr, sizeof(server_addr))
    );

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
    req = init_proxy_request((const char *) host ,(const int) port);

    TIME_ECLAPSE("send",
    result = send(sock, (const char *) req, (int) REQUEST_SIZE, 0)
    );

    if (result == SOCKET_ERROR) {
        fprintf(stderr,
                "send failed: %d\n",
                WSAGetLastError());
    } 

    char buffer_response[RESPONSE_SIZE] = {0};
    TIME_ECLAPSE("receive",
    result = receive_full(sock, buffer_response, RESPONSE_SIZE)
    );

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


    char buff_temp[512];
    memset(buff_temp, 0, sizeof(buff_temp));

    snprintf(buff_temp, 511,
        "HEAD / HTTP/1.0\r\n"
        "HOST: 46.227.66.141:80\r\n"
        "\r\n"
    );

    TIME_ECLAPSE("send",
    result = send(sock, (const char *) buff_temp, (int) 512, 0)
    );

    if (result == SOCKET_ERROR) {
        fprintf(stderr,
                "send failed: %d\n",
                WSAGetLastError());
    }

    memset(buff_temp, 0, sizeof(buff_temp));
    TIME_ECLAPSE("receive",
    result = receive_full(sock, buff_temp, 512)
    );

    fprintf(stdout, "Receive:\r\n %s\r\n", buff_temp);


    closesocket(sock);
    WSACleanup();

    return EXIT_SUCCESS;
}


