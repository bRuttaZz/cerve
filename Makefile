
VERSION = $(shell cat VERSION)

CC = gcc
CFLAGS = -I./include

# executable name
EXEC = cerve_v$(VERSION)

ENTRY_POINT = src/main.c
SRCS = $(foreach dir, src/lib src/lib/utils,$(wildcard $(dir)/*.c))
TESTS += $(foreach dir, tests, $(wildcard $(dir)/*.c))

# debug flags
DBUG_FLAGS = -Wall -g

# production mode flags
PROD_FLAGS = -02


help:	## Show all Makefile targets.
	@echo -e "\nCerve v$(VERSION)\n"
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[33m%-30s\033[0m %s\n", $$1, $$2}'

test: 	## run test cases
	@echo -e "Compiling test cases.."
	$(CC) $(CFLAGS) $(DBUG_FLAGS) -o test_$(EXEC) $(TESTS) $(SRCS) -g
	@echo -e "\nExecuting the test cases..!\n"
	@./test_$(EXEC)

dev: ## Build and run in debug mode
	@echo -e "Compiling cerve in debug mode.."
	$(CC) $(CFLAGS) $(DBUG_FLAGS) -o debug_$(EXEC) $(ENTRY_POINT) $(SRCS) -g
	@echo -e "\n Running cerve..!\n"
	@./debug_$(EXEC)

build: 	## Build cerve
	$(CC) $(CFLAGS) $(PROD_FLAGS) -o $(EXEC) $(ENTRY_POINT) $(SRCS)



clean:	## Clean the mess created by build
	@rm -f $(EXEC) test_$(EXEC) debug_$(EXEC)
