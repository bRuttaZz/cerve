# configs
DEBUG = true
CC = gcc

CFLAGS = -I./include

# for wayland
WAYLAND_INCS = wayland-client
CFLAGS += $(shell pkg-config --cflags $(WAYLAND_INCS) --libs $(WAYLAND_INCS) )

# for ffmpeg
FFMPEG_INCS = libavformat libavcodec libavutil
CFLAGS += $(shell pkg-config --cflags $(FFMPEG_INCS) --libs $(FFMPEG_INCS) )

# gnu linux specials
CFLAGS += -D_GNU_SOURCE


ifeq ($(DEBUG), true)
	CFLAGS += -Wall
	CFLAGS += -g
else
	CFLAGS += -02
endif


# executable name
EXEC = cerve

ENTRY_POINT = src/main.c
SRCS = $(foreach dir, src/lib src/lib/utils src/lib/server src/lib/video, $(wildcard $(dir)/*.c))
TESTS += $(foreach dir, tests, $(wildcard $(dir)/*.c))


help:	## Show all Makefile targets.
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[33m%-30s\033[0m %s\n", $$1, $$2}'

test: 	## run test cases
	@echo -e "Compiling test cases.."
	$(CC) $(CFLAGS) -o test_$(EXEC) $(TESTS) $(SRCS) -g
	@echo -e "\nExecuting the test cases..!\n"
	@./test_$(EXEC)

build: 	## Build cerve
	$(CC) $(CFLAGS) -o $(EXEC) $(ENTRY_POINT) $(SRCS)

clean:	## Clean the mess created by build
	@rm -f $(EXEC) test_$(EXEC)
