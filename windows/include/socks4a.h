#pragma once
#include "socks4.h"
#include <winsock2.h>
#include <stdint.h>

/*
    SOCKS4a — like SOCKS4, but instead of a resolved IP you send:
        DSTIP    = 0.0.0.1   (impossible IP => "a hostname follows")
        HOSTNAME = <name>\0  appended right after the userid NUL

    The PROXY (Tor) does the DNS, so the name never leaks to your local
    resolver. Requires the caller to actually possess the hostname — in the
    injected DLL that comes from the getaddrinfo hook (see toralize_dll.c).

    Request layout:
        VN | CN | DSTPORT | DSTIP=0.0.0.1 | USERID | 0x00 | HOSTNAME | 0x00
*/
int socks4a_handshake(SOCKET socket, const char *host, uint16_t port);
