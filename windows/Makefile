all: clean build

SRCS := $(wildcard src/*.c)

build:
	@echo Building toralize
	mkdir out
	clang $(SRCS) -o out/toralize.exe -I. -Iinclude -lws2_32

clean:
	@echo Cleaning old file
	rmdir /S /Q out
	del /Q toralize.exe 2>nul || exit 0

.PHONY: all build clean