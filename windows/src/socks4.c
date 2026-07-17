#include <stdio.h>
#include <stdlib.h>
#include "socks4.h"
#include "utils.h"
//Win Socket
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
/*
    toralize IP <192.16.23.1> Port <80>
*/


int socks4_handshake(SOCKET socket, const SOCKADDR_IN* dst) {
    _proxy_request req;
    req.vn = 4;
    req.cn = 1;
    req.dstport = dst->sin_port;
    req.dstip   = dst->sin_addr.s_addr; 
    memcpy((char *) req.userid, PROXY_NAME, 8);
    LOG_PRINTF(stdout, "[socks4] socks4 handshake init");
    if (send(socket, (const char *)&req, (int)REQUEST_SIZE, 0) == SOCKET_ERROR){
        LOG_PRINTF(stderr, "[socks4] socks4 handshake failed");
        return -1;
    }

    char buf[RESPONSE_SIZE] = {0};
    if (receive_full(socket, buf, (int)RESPONSE_SIZE) < 0) {
        LOG_PRINTF(stdout, "[socks4] socks4 receive_error error");
        return -1;
    }

    _proxy_response *res = (_proxy_response *)buf;
    LOG_PRINTF(stdout, "[socks4] proxy_response, res: %d, please refer to file %s", res->cn, "windows/src/socks4.h");
    return (res->cn == REQUEST_GRANTED) ? 0 : -1;   // 90 == granted
}