ifdef DEBUG
debugKey := -g
DEBUGKEYNORMAL := $(subst $\",,$(debugKey))
endif



# Build stages
all: all-before player
all-before:
	mkdir -p build/


# Run targets
run: 
	DISPLAY=:0 ./player $(CURDIR)/test.mp4
run-debug: 
	DISPLAY=:0 gdb player

# Sources compilation
player: main.o
	gcc -o player build/main.o `pkg-config --libs gtk+-3.0 gstreamer-1.0 gstreamer-video-1.0`

main.o:
	gcc `pkg-config --cflags gtk+-3.0 gstreamer-1.0 gstreamer-video-1.0` $(DEBUGKEYNORMAL) -c -o build/main.o src/main.c 