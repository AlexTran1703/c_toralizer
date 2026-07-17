# Tor toralizer (mirror of ../windows with working SOCKS4a)
#
#   SOCK_VERSION_4A=1  -> hook getaddrinfo + send hostname to Tor (SOCKS4a, no DNS-leak warning)
#   SOCK_VERSION_4A=0  -> plain SOCKS4 (sends resolved IP; Tor warns about DNS leak)
#
# Build:   make build         (debug: console color + file log)
#          make release
#          make build SOCK_VERSION_4A=0
SOCK_VERSION_4A ?= 1

# MinHook is shared with the sibling windows/ project. Override if you copy it in.
MINHOOK_DIR ?= ../windows/third_party/minhook
MINHOOK_INC := -I$(MINHOOK_DIR)/include
MINHOOK_SRC := $(wildcard $(MINHOOK_DIR)/src/*.c $(MINHOOK_DIR)/src/hde/*.c)

DLL_SRC := src/toralize_dll.c src/socks4.c src/socks4a.c src/utils.c
DEFS    := -DSOCK_VERSION_4A=$(SOCK_VERSION_4A)

all: clean-log build
build:   clean build-debug-dll build-debug-exe
release: clean build-dll build-exe

#Debug
build-debug-dll:
	clang -shared $(DLL_SRC) $(MINHOOK_SRC) -o out/toralize.dll \
	      -Iinclude $(MINHOOK_INC) -DDEBUG_FLAG $(DEFS) -lws2_32

build-debug-exe:
	clang src/toralize_launch.c -o out/toralize.exe -Iinclude -DDEBUG_FLAG

#Release
build-dll:
	clang -shared $(DLL_SRC) $(MINHOOK_SRC) -o out/toralize.dll \
	      -Iinclude $(MINHOOK_INC) $(DEFS) -lws2_32

build-exe:
	clang src/toralize_launch.c -o out/toralize.exe -Iinclude

#Clean
clean:
	@echo Cleaning old files
	del /f /q "out\*.*" 2>nul || exit 0
	rmdir /S /Q out 2>nul || exit 0
	mkdir out

clean-log:
	@echo Cleaning log files
	del /f /q "log\*.*" 2>nul || exit 0

.PHONY: all build release build-debug-dll build-debug-exe build-dll build-exe clean clean-log
