# Build the SDL2 demos with MinGW/gcc (run from an MSYS2 MINGW64 shell).
#
#   pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-pkg-config
#   make            # build all demos
#   make run-fizzle # build and run the fizzle fade
#   make clean

CC      ?= gcc
CFLAGS  += -Wall -Wextra -O2 $(shell pkg-config --cflags sdl2)
LDLIBS  += $(shell pkg-config --libs sdl2)

DEMOS := fizzle_fade_sdl framebuffer_sdl

all: $(DEMOS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

run-fizzle: fizzle_fade_sdl
	./fizzle_fade_sdl

run-framebuffer: framebuffer_sdl
	./framebuffer_sdl

clean:
	rm -f $(addsuffix .exe,$(DEMOS)) $(DEMOS)

.PHONY: all clean run-fizzle run-framebuffer
