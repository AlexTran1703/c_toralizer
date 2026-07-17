#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define PROXY_HOST "127.0.0.1"
#define PROXY_PORT 9050
#define PROXY_NAME "toraliz"
#define REQUEST_SIZE  (sizeof(struct proxy_request))
#define RESPONSE_SIZE (sizeof(struct proxy_response))

/*
    SOCKS4 TCP proxy  — https://www.openssh.org/txt/socks4.protocol

    CONNECT request:
        +----+----+----+----+----+----+----+----+----+----+....+----+
        | VN | CD | DSTPORT |      DSTIP        | USERID       |NULL|
        +----+----+----+----+----+----+----+----+----+----+....+----+
          1    1      2              4           variable       1
*/

typedef unsigned char      byte8;
typedef unsigned short int byte16;
typedef unsigned int       byte32;

/* Wire structs must have NO padding — pack them and prove the size. */
#pragma pack(push, 1)
struct proxy_request {
    byte8  vn;                 /* 4 = SOCKS version              */
    byte8  cn;                 /* 1 = CONNECT                    */
    byte16 dstport;            /* destination port, network order */
    byte32 dstip;              /* destination IP,   network order */
    unsigned char userid[8];   /* "toraliz\0"                     */
};

/* Reply: VN|CD|DSTPORT|DSTIP  (CD 90 = granted) */
struct proxy_response {
    byte8  vn;
    byte8  cn;
    byte16 _;
    byte32 __;
};
#pragma pack(pop)

_Static_assert(sizeof(struct proxy_request)  == 16, "proxy_request must be 16 bytes");
_Static_assert(sizeof(struct proxy_response) ==  8, "proxy_response must be 8 bytes");

typedef struct proxy_request  _proxy_request;
typedef struct proxy_response _proxy_response;

enum RESPONSE_CODE {
    REQUEST_GRANTED        = 90,   /* request granted                     */
    REQUEST_FAILED         = 91,   /* rejected or failed                  */
    REQUEST_CONNECT_FAILED = 92,   /* SOCKS server cannot reach identd    */
    REQUEST_WRONG_USERED   = 93    /* client / identd user-ids differ     */
};

/* SOCKS4: hands the proxy a raw destination IP (from the SOCKADDR_IN). */
int socks4_handshake(SOCKET socket, const SOCKADDR_IN* dst);
