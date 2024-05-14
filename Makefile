CC = gcc

CFLAGS = -Wall -I./include

# executable name
EXEC = cerve

SRCDIRS = src 
SRCS = $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))

help:	## Show all Makefile targets.
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[33m%-30s\033[0m %s\n", $$1, $$2}'

build: 	## Build cerve
	$(CC) $(CFLAGS) -o $(EXEC) $(SRCS)

clean:	## Clean the mess created by build
	@rm -f $(EXEC)
