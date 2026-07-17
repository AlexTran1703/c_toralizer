# c_toralizer (Windows)

Route any program's TCP traffic through **Tor's proxy** by injecting a DLL that
hooks Winsock `connect()` — the Windows equivalent of Linux `LD_PRELOAD` toralize.

```
toralize.exe <program> [args...]
#e.g .\out\toralize.exe curl.exe --resolve check.torproject.org:443:116.202.120.181 https://check.torproject.org/api/ip

```

---
## Current version


| Version | DST field                                     | Hostname? | Steps                                | Auth     | Status |
| ------- | --------------------------------------------- | --------- | ------------------------------------ | -------- | ------ |
| SOCKS4  | 4-byte IP (you resolve DNS)                   | no        | 1 req/resp                           | no       | ✅ done |
| SOCKS4a | IP = 0.0.0.x, host appended after userid `\0` | yes       | 1 req/resp                           | no       | ☐ todo |
| SOCKS5  | ATYP: 0x01 IPv4 / 0x03 host / 0x04 IPv6       | yes       | 3-step (greeting → [auth] → connect) | optional | ☐ todo |


## Quick start for socks4
```powershell
# 1. Get Tor (idempotent — skips if windows/tor_3rd exists)
cd windows
.\get-tor.ps1

# 2.a Start Tor (SOCKS proxy on 127.0.0.1:9050)
## Must run in detached terminal
.\tor_3rd\tor\tor.exe
# 

# 3. Build (from windows/)
make build      # debug build  (console color + DEBUG_FLAG)  -> out/toralize.exe + out/toralize.dll
make release    # no debug output

# 4. Run something through Tor
.\out\toralize.exe curl.exe --resolve check.torproject.org:443:116.202.120.181 https://check.torproject.org/api/ip
#   expect: {"IsTor":true,"IP":"..."}
```

> In PowerShell, `curl` is an alias for `Invoke-WebRequest`. **Always use `curl.exe`.**

---

## Make targets

| Target             | What it does                                                        |
| ------------------ | ------------------------------------------------------------------- |
| `make` / `all`     | `clean-log` + `build`                                               |
| `build`            | clean + debug DLL + debug EXE (`-DDEBUG_FLAG`, console color)        |
| `release`          | clean + release DLL + EXE (no debug output)                         |
| `build-debug-dll`  | `toralize.dll` with MinHook, ws2_32, DEBUG_FLAG                     |
| `build-debug-exe`  | `toralize.exe` launcher/injector                                    |
| `clean`            | wipe & recreate `out/`                                              |
| `clean-log`        | delete `log/*`                                                      |

Compiler: **clang**. Includes: `-Iinclude -Ithird_party/minhook/include`. Link: `-lws2_32`.

---

## Architecture

```
toralize.exe (launcher)                       curl.exe (target, suspended)
  CreateProcess(CREATE_SUSPENDED) ───────────► process created, main thread paused
  VirtualAllocEx + WriteProcessMemory  (dll path)
  CreateRemoteThread(LoadLibraryA) ──────────► toralize.dll loaded
                                                  DllMain -> spawns install_hooks thread
                                                    MinHook: hook ws2_32!connect -> tor_connect
                                                    SetEvent("ToralizeHooksReady")
  WaitForSingleObject("ToralizeHooksReady") ◄──── (sync: hooks ready before resume)
  ResumeThread ──────────────────────────────► curl runs; every connect() -> tor_connect()
```

`tor_connect()` (in `src/toralize_dll.c`):
1. Only intercepts `AF_INET`. Forces the socket **blocking** (`ioctlsocket FIONBIO 0`).
2. `real_connect()` to **the proxy** `127.0.0.1:9050` (not the destination).
3. `socks4_handshake(s, dst)` — sends the SOCKS4 request carrying the **destination** IP/port.
4. Restores non-blocking so curl's `select()`/`recv()` behave as expected.

---

## SOCKS4 quick reference

Request (8 bytes + null-terminated userid):

| Field   | Size | Value                                  |
| ------- | ---- | -------------------------------------- |
| `vn`    | 1    | `4` (version)                          |
| `cn`    | 1    | `1` (CONNECT)                          |
| `dstport` | 2  | destination port, **network order** (`htons`) |
| `dstip` | 4    | destination IP, **network order** (`s_addr`)  |
| `userid`| n    | ID string + `\0` terminator            |

Response (8 bytes):

| Field | Size | Notes                          |
| ----- | ---- | ------------------------------ |
| `vn`  | 1    | `0`                            |
| `cn`  | 1    | reply code (below)             |
| `_`   | 2+4  | ignored                        |

Reply codes: **`90` = granted** · `91` = rejected/failed · `92` = no identd · `93` = identd mismatch.

- **SOCKS4** = IP only (4-byte `dstip`). You must resolve the hostname yourself (`getaddrinfo`).
- **SOCKS4a** = hostname support: set `dstip` to `0.0.0.x` (x≠0) and append the hostname after userid.
- Send the **full** request size (don't drop the userid `\0`), and `recv` the full 8-byte reply
  (`receive_full` loops until all bytes arrive).

Manual test that Tor's SOCKS works:
```powershell
curl.exe --socks4a 127.0.0.1:9050 https://check.torproject.org/api/ip
```

---

## Logging / debug

- Debug builds (`-DDEBUG_FLAG`) write colored output to the console **and** a plain-text file.
- Logs go to `log/toralize_YYYY_MM_DD_HH_MM.log` (one file per process, ANSI codes stripped from the file).
- `LOG_PRINTF(stdout|stderr, fmt, ...)` — stderr => red, stdout => green (console only).
- Color only renders on a **console**; a `.log` file stores plain text.

---

## Layout

```
windows/
  include/    socks4.h  utils.h
  src/        toralize_launch.c  (EXE)   toralize_dll.c  (DLL)   socks4.c   utils.c
  third_party/minhook/            API-hooking library
  tor_3rd/                        Tor Expert Bundle (from get-tor.ps1)
  out/                            build output
  log/                            runtime logs
  Makefile     get-tor.ps1
```

## Requirements

clang · GNU make · Tor Expert Bundle (`get-tor.ps1`) · Windows 10+ (ANSI VT for colored console).
