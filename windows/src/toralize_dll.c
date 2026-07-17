#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "socks4.h"
#include "socks4a.h"
#include "utils.h"
#include <windows.h>
#include "MinHook.h"
//Win Socket
#include <winsock2.h>
#include <ws2tcpip.h>

/* default off (plain SOCKS4) unless the build defines it */
#ifndef SOCK_VERSION_4A
#define SOCK_VERSION_4A 0
#endif

typedef int (WSAAPI *connect_t)(SOCKET, SOCKADDR*, int);
static connect_t real_connect = NULL;

/* --------------------------------------------------------------------------
   SOCKS4a support: capture hostname -> IP as curl resolves, so we can send
   the real name to Tor at connect() time. Compiled only when SOCK_VERSION_4A.
   -------------------------------------------------------------------------- */
#if SOCK_VERSION_4A == 1
typedef int (WSAAPI *getaddrinfo_t)(const char*, const char*, const ADDRINFOA*, ADDRINFOA**);
static getaddrinfo_t real_getaddrinfo = NULL;

#define MAP_MAX 256
typedef struct { uint32_t ip; char host[256]; } dns_entry;
static dns_entry        g_map[MAP_MAX];
static int              g_map_count = 0;
static CRITICAL_SECTION g_map_lock;

static void remember(uint32_t ip, const char *host) {
    if (!host || !host[0]) return;
    EnterCriticalSection(&g_map_lock);
    int i = g_map_count % MAP_MAX;                 // simple ring; newest wins
    g_map[i].ip = ip;
    size_t hl = strlen(host);
    if (hl >= sizeof g_map[i].host) hl = sizeof g_map[i].host - 1;
    memcpy(g_map[i].host, host, hl);
    g_map[i].host[hl] = 0;
    g_map_count++;
    LeaveCriticalSection(&g_map_lock);
}

static int lookup(uint32_t ip, char *out, size_t n) {
    int found = 0;
    int limit = g_map_count < MAP_MAX ? g_map_count : MAP_MAX;
    EnterCriticalSection(&g_map_lock);
    for (int i = limit - 1; i >= 0; --i)           // search newest first
        if (g_map[i].ip == ip) {
            size_t hl = strlen(g_map[i].host);
            if (hl >= n) hl = n - 1;
            memcpy(out, g_map[i].host, hl);
            out[hl] = 0;
            found = 1;
            break;
        }
    LeaveCriticalSection(&g_map_lock);
    return found;
}

/* Passive hook: let the real resolver run, then remember name -> each IPv4. */
int WSAAPI tor_getaddrinfo(const char *node, const char *service,
                           const ADDRINFOA *hints, ADDRINFOA **res) {
    int rc = real_getaddrinfo(node, service, hints, res);
    if (rc == 0 && node && res && *res) {
        for (ADDRINFOA *p = *res; p; p = p->ai_next)
            if (p->ai_family == AF_INET)
                remember(((SOCKADDR_IN*)p->ai_addr)->sin_addr.s_addr, node);
    }
    return rc;
}
#endif  /* SOCK_VERSION_4A == 1 */

/* --------------------------------------------------------------------------
   The connect() hook — reroute AF_INET connections through the Tor proxy.
   -------------------------------------------------------------------------- */
int WSAAPI tor_connect(SOCKET s, SOCKADDR* dst_sa, int len) {
    if (dst_sa->sa_family != AF_INET)
        return real_connect(s, dst_sa, len);

    const SOCKADDR_IN* dst = (const SOCKADDR_IN *) dst_sa;

    /* ALWAYS skip loopback: curl's async DNS resolver uses 127.0.0.0/8 sockets
       internally, and Tor refuses loopback targets. (Not version-specific.) */
    if ((ntohl(dst->sin_addr.s_addr) >> 24) == 127)
        return real_connect(s, dst_sa, len);

    char ip[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &dst->sin_addr, ip, sizeof ip);
    LOG_PRINTF(stdout, "[toralize] intercept %s:%u", ip, (unsigned)ntohs(dst->sin_port));

    /* curl's socket is non-blocking; force blocking for the SOCKS handshake */
    u_long nb = 0;
    ioctlsocket(s, FIONBIO, &nb);

    SOCKADDR_IN proxy = {0};
    proxy.sin_family = AF_INET;
    proxy.sin_port   = htons(PROXY_PORT);
    inet_pton(AF_INET, PROXY_HOST, &proxy.sin_addr);

    int ok = (real_connect(s, (SOCKADDR *)&proxy, sizeof proxy) == 0);
    if (ok) {
#if SOCK_VERSION_4A == 1
        char host[256];
        if (lookup(dst->sin_addr.s_addr, host, sizeof host)) {
            ok = (socks4a_handshake(s, host, ntohs(dst->sin_port)) == 0);   // name -> Tor resolves
        } else {
            LOG_PRINTF(stdout, "[toralize] no captured hostname; falling back to SOCKS4");
            ok = (socks4_handshake(s, dst) == 0);
        }
#else
        ok = (socks4_handshake(s, dst) == 0);
#endif
    }

    if (!ok) {
        LOG_PRINTF(stderr, "[toralize] tunnel FAILED");
        WSASetLastError(WSAECONNREFUSED);       // do NOT WSACleanup() — that kills curl's Winsock
        return SOCKET_ERROR;
    }

    /* restore non-blocking so curl's select()/recv() behave as it expects */
    u_long back = 1;
    ioctlsocket(s, FIONBIO, &back);

    LOG_PRINTF(stdout, "[toralize] tunneled through Tor OK");
    return 0;
}

/* --------------------------------------------------------------------------
   Hook installation on a worker thread (never do heavy work under loader lock).
   -------------------------------------------------------------------------- */
static DWORD WINAPI install_hooks(LPVOID param) {
    (void)param;

    MH_STATUS s_init   = MH_Initialize();
    MH_STATUS s_conn   = MH_CreateHookApi(L"ws2_32", "connect", &tor_connect, (void **)&real_connect);
#if SOCK_VERSION_4A == 1
    InitializeCriticalSection(&g_map_lock);
    MH_STATUS s_gai    = MH_CreateHookApi(L"ws2_32", "getaddrinfo", &tor_getaddrinfo, (void **)&real_getaddrinfo);
#endif
    MH_STATUS s_enable = MH_EnableHook(MH_ALL_HOOKS);

    LOG_PRINTF(stdout, "[install_hooks] init=%d connect=%d enable=%d SOCKS4A=%d",
               s_init, s_conn, s_enable, SOCK_VERSION_4A);
#if SOCK_VERSION_4A == 1
    LOG_PRINTF(stdout, "[install_hooks] getaddrinfo=%d", s_gai);
#endif

    HANDLE ev = OpenEventA(EVENT_MODIFY_STATE, FALSE, "ToralizeHooksReady");
    if (ev) { SetEvent(ev); CloseHandle(ev); }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID r) {
    (void)r;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);                       // fewer loader callbacks
        HANDLE t = CreateThread(NULL, 0, install_hooks, NULL, 0, NULL);
        if (t) CloseHandle(t);
    } else if (reason == DLL_PROCESS_DETACH) {
        MH_Uninitialize();
    }
    return TRUE;                                            // return FAST — don't block the loader
}
