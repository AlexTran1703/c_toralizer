#include <stdio.h>
#include <stdint.h>

#define PROXY_IP "127.0.0.1"
#define PROXY_PORT 9090
#define PROXY_NAME "toraliz"
#define REQUEST_SIZE (sizeof (struct proxy_request))
#define RESPONSE_SIZE (sizeof (struct proxy_response))
/*
    WIN_SOCK
    https://learn.microsoft.com/en-us/windows/win32/winsock/winsock-functions
*/

/*
TCP proxy
https://www.openssh.org/txt/socks4.protocol
*/

typedef unsigned char byte8;
typedef unsigned short int byte16;
typedef unsigned int byte32;
/*
Two operations are defined: CONNECT and BIND.

1) CONNECT

The client connects to the SOCKS server and sends a CONNECT request when
it wants to establish a connection to an application server. The client
includes in the request packet the IP address and the port number of the
destination host, and userid, in the following format.

		+----+----+----+----+----+----+----+----+----+----+....+----+
		| VN | CD | DSTPORT |      DSTIP        | USERID       |NULL|
		+----+----+----+----+----+----+----+----+----+----+....+----+
 # of bytes:	   
            1    1      2              4           variable       1

VN is the SOCKS protocol version number and should be 4. CD is the
SOCKS command code and should be 1 for CONNECT request. NULL is a byte
of all zero bits.
*/

struct proxy_request {
    byte8 vn;
    byte8 cn;
    byte16 dstport;
    byte32 dstip;
    unsigned char userid[8];
};

typedef struct proxy_request _proxy_request;


/*
The SOCKS server checks to see whether such a request should be granted
based on any combination of source IP address, destination IP address,
destination port number, the userid, and information it may obtain by
consulting IDENT, cf. RFC 1413.  If the request is granted, the SOCKS
server makes a connection to the specified port of the destination host.
A reply packet is sent to the client when this connection is established,
or when the request is rejected or the operation fails. 

		+----+----+----+----+----+----+----+----+
		| VN | CD | DSTPORT |      DSTIP        |
		+----+----+----+----+----+----+----+----+
 # of bytes:
    	   1    1      2              4

VN is the version of the reply code and should be 0. CD is the result
code with one of the following values:

	90: request granted
	91: request rejected or failed
	92: request rejected becasue SOCKS server cannot connect to
	    identd on the client
	93: request rejected because the client program and identd
	    report different user-ids

The remaining fields are ignored.

The SOCKS server closes its connection immediately after notifying
the client of a failed or rejected request. For a successful request,
the SOCKS server gets ready to relay traffic on both directions. This
enables the client to do I/O on its connection as if it were directly
connected to the application server.
*/

enum RESPONSE_CODE {
	REQUEST_GRANTED = 90,
	REQUEST_FAILED = 91,
	REQUEST_CONNECT_FAILED = 92,
	REQUEST_WRONG_USERED = 93
};

struct proxy_response {
    byte8 vn;
    byte8 cn;
    byte16 _;
    byte32 __;
};

typedef struct proxy_response _proxy_response;

/*
2) BIND

The client connects to the SOCKS server and sends a BIND request when
it wants to prepare for an inbound connection from an application server.
This should only happen after a primary connection to the application
server has been established with a CONNECT.  Typically, this is part of
the sequence of actions:

-bind(): obtain a socket
-getsockname(): get the IP address and port number of the socket
-listen(): ready to accept call from the application server
-use the primary connection to inform the application server of
 the IP address and the port number that it should connect to.
-accept(): accept a connection from the application server

The purpose of SOCKS BIND operation is to support such a sequence
but using a socket on the SOCKS server rather than on the client.

The client includes in the request packet the IP address of the
application server, the destination port used in the primary connection,
and the userid.

		+----+----+----+----+----+----+----+----+----+----+....+----+
		| VN | CD | DSTPORT |      DSTIP        | USERID       |NULL|
		+----+----+----+----+----+----+----+----+----+----+....+----+
 # of bytes:	   
            1    1      2              4           variable       1

VN is again 4 for the SOCKS protocol version number. CD must be 2 to
indicate BIND request.

*/

// struct proxy_bind {
//     byte8 vn;
//     byte8 cn;
//     byte16 _;
//     byte32 __;
//     unsigned char ___[8];
// };

// typedef struct proxy_bind _proxy_bind;

/*
The SOCKS server uses the client information to decide whether the
request is to be granted. The reply it sends back to the client has
the same format as the reply for CONNECT request, i.e.,

		+----+----+----+----+----+----+----+----+
		| VN | CD | DSTPORT |      DSTIP        |
		+----+----+----+----+----+----+----+----+
 # of bytes:	   1    1      2              4

VN is the version of the reply code and should be 0. CD is the result
code with one of the following values:

	90: request granted
	91: request rejected or failed
	92: request rejected becasue SOCKS server cannot connect to
	    identd on the client
	93: request rejected because the client program and identd
	    report different user-ids.
*/

_proxy_request* init_proxy_request(const char *dstip, const int dstport);
_proxy_response* init_proxy_response();