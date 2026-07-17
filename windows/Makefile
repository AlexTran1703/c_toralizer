#Build -> DEBUG CONSOLE
#Relse -> WITHOUT DEBUG
all: clean-log build
build: clean build-debug-dll build-debug-exe
release: clean build-dll build-exe

MINHOOK_INC := -Ithird_party/minhook/include
MINHOOK_SRC := $(wildcard third_party/minhook/src/*.c third_party/minhook/src/hde/*.c)

#Debug
build-debug-dll:
	clang -shared src/toralize_dll.c src/socks4.c src/utils.c $(MINHOOK_SRC) \
	      -o out/toralize.dll -Iinclude -DDEBUG_FLAG $(MINHOOK_INC) -lws2_32

build-debug-exe:
	clang src/toralize_launch.c -o out/toralize.exe -Iinclude -DDEBUG_FLAG

#Release
build-dll:
	clang -shared src/toralize_dll.c src/socks4.c src/utils.c $(MINHOOK_SRC) \
	      -o out/toralize.dll -Iinclude $(MINHOOK_INC) -lws2_32

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

.PHONY: all build clean