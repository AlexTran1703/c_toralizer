#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "socks4a.h"
#include "utils.h"
//Win Socket
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

/*
    SOCKS4a CONNECT handshake.

    Unlike SOCKS4 we do not have a fixed-size struct — the hostname is
    variable length — so we build the request in a byte buffer and send
    exactly the bytes we filled (never sizeof).

        VN | CN | DSTPORT | DSTIP=0.0.0.1 | USERID | 0x00 | HOSTNAME | 0x00
*/
int socks4a_handshake(SOCKET socket, const char *host, uint16_t port) {
    unsigned char req[2 + 2 + 4 + 8 + 256];   // header + userid+NUL + hostname+NUL
    int n = 0;

    req[n++] = 4;                              // VN = SOCKS4
    req[n++] = 1;                              // CN = CONNECT

    uint16_t nport = htons(port);
    memcpy(req + n, &nport, 2); n += 2;        // DSTPORT (network order)

    req[n++] = 0; req[n++] = 0;                // DSTIP = 0.0.0.1  -> "hostname follows"
    req[n++] = 0; req[n++] = 1;

    size_t ul = strlen(PROXY_NAME);
    memcpy(req + n, PROXY_NAME, ul); n += (int)ul;
    req[n++] = 0;                              // userid NUL

    size_t hl = strlen(host);
    if (hl >= 256) {
        LOG_PRINTF(stderr, "[socks4a] hostname too long");
        return -1;
    }
    memcpy(req + n, host, hl); n += (int)hl;
    req[n++] = 0;                              // hostname NUL

    LOG_PRINTF(stdout, "[socks4a] handshake init host=%s port=%u", host, (unsigned)port);
    if (send(socket, (const char *)req, n, 0) == SOCKET_ERROR) {
        LOG_PRINTF(stderr, "[socks4a] send failed");
        return -1;
    }

    char buf[RESPONSE_SIZE] = {0};
    if (receive_full(socket, buf, (int)RESPONSE_SIZE) < 0) {
        LOG_PRINTF(stderr, "[socks4a] receive error");
        return -1;
    }

    _proxy_response *res = (_proxy_response *)buf;
    LOG_PRINTF(stdout, "[socks4a] proxy_response cn=%d (90 == granted)", res->cn);
    return (res->cn == REQUEST_GRANTED) ? 0 : -1;
}
