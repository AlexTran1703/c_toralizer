#include <stdio.h>
#include <stdlib.h>
#include "socks4.h"
#include "utils.h"
#include <windows.h>
#include "MinHook.h"
//Win Socket
#include <winsock2.h>
#include <ws2tcpip.h>


typedef int (WSAAPI *connect_t) (SOCKET, SOCKADDR*, int);
static connect_t real_connect = NULL;

int WSAAPI tor_connect(SOCKET s, SOCKADDR* saddr_dst, int saddr_len) {
    if (saddr_dst->sa_family == AF_INET) {
        const SOCKADDR_IN* dst = (const SOCKADDR_IN *) saddr_dst;

        char ip_address[INET_ADDRSTRLEN] = {0};

        if (inet_ntop(AF_INET, &dst->sin_addr, ip_address, sizeof(ip_address)) == NULL) {
            LOG_PRINTF(stderr, "[toralize] inet_ntop failed: %d", WSAGetLastError());
            return SOCKET_ERROR;
        }

        LOG_PRINTF(stdout, "[toralize] torilize_dll started, hostname=%s, port=%d, family=%d", ip_address, (unsigned)ntohs(dst->sin_port), dst->sin_family);
        /* curl made this socket non-blocking; force blocking for the SOCKS handshake */
        u_long nb = 0;
        ioctlsocket(s, FIONBIO, &nb);                 // 0 = blocking
        SOCKADDR_IN proxy = {0};
        proxy.sin_family = AF_INET;
        proxy.sin_port   = htons(PROXY_PORT);
        inet_pton(AF_INET, PROXY_HOST, &proxy.sin_addr);

        int ok = real_connect(s, (SOCKADDR *)&proxy, sizeof(proxy)) == 0;
        if (!ok) {
            LOG_PRINTF(stderr, "[toralize] connection FAILED");
            WSACleanup();
            WSASetLastError(WSAECONNREFUSED);
            return SOCKET_ERROR;
        }
        ok = socks4_handshake(s, dst) == 0;

        if (!ok) {
            LOG_PRINTF(stderr, "[toralize] handshake FAILED");
            WSACleanup();
            WSASetLastError(WSAECONNREFUSED);
            return SOCKET_ERROR;
        }

        /* restore non-blocking so curl's select()/recv() behave as it expects */
        u_long back = 1;
        ioctlsocket(s, FIONBIO, &back);               // 1 = non-blocking
        
        LOG_PRINTF(stdout, "[toralize] tunneled through torilizer dll success");
        return 0;
    }
    return real_connect(s, saddr_dst, saddr_len);
}


static DWORD WINAPI install_hooks(LPVOID param) {
    (void)param;
    MH_STATUS s1 = MH_Initialize();
    MH_STATUS s2 = MH_CreateHookApi(L"ws2_32", "connect", &tor_connect, (void **)&real_connect);
    MH_STATUS s3 = MH_EnableHook(MH_ALL_HOOKS);
    LOG_PRINTF(stdout, "[install_hooks] init=%d create=%d enable=%d", s1, s2, s3);  // wsprintfA = Win32, no CRT

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