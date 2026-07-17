# windows

| Area | Previous Error | Fixed |
| ---- | -------- | ---------- |
| Loopback guard | gated on `#if SOCK_VERSION_4A` (breaks resolver at 4A=0) | **always on** — skips `127.0.0.0/8` |
| Socket mode | `ioctlsocket` commented out (connect fails) | blocking for handshake, restored after |
| Failure path | `WSACleanup()` (tears down curl's Winsock) | just `WSASetLastError` + return |
| Wire structs | unpacked (works by luck) | `#pragma pack(1)` + `_Static_assert` |
| SOCKS4a | reverse-DNS stub (can't work) | `getaddrinfo` hook captures name→IP, sends real hostname |

## How SOCKS4a works here

A `connect()` hook only sees an IP, so a hostname must be captured *earlier*:

1. `tor_getaddrinfo` (hook on `ws2_32!getaddrinfo`) records `IP → hostname` as curl resolves.
2. `tor_connect` looks the destination IP up in that table:
   - **found** → `socks4a_handshake(s, host, port)` — Tor does the DNS (no leak warning).
   - **not found** → falls back to `socks4_handshake` (IP).

The table is guarded by a `CRITICAL_SECTION` (resolver runs on another thread).

> Note: `getaddrinfo` still resolves locally too, so the OS-level lookup isn't eliminated —
> but Tor stops warning because it receives a hostname. To fully suppress local DNS you'd
> return a synthetic IP from the hook (fake-IP shim); left as a future step.

## Build & run

```powershell
cd windows
make build                 # SOCKS4a (default)
# make build SOCK_VERSION_4A=0   # plain SOCKS4

# start Tor (from the windows/ project's bundle), then:
.\out\toralize.exe curl.exe https://check.torproject.org/api/ip
#   expect: {"IsTor":true,"IP":"..."}
```

MinHook and Tor are shared with `../windows` (see `MINHOOK_DIR` in the Makefile and
`../windows/get-tor.ps1`).
