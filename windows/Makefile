all: clean build

build:
	@echo Building toralize
	mkdir out
	clang src/toralize.c -o out/toralize.exe -I. -lws2_32

clean:
	@echo Cleaning old file
	rmdir /S /Q out
	del /Q toralize.exe 2>nul || exit 0

.PHONY: all build clean