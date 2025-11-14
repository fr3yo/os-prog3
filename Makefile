# Makefile for User-Level Thread Library (Student Version)
# CISC 361 - Programming Assignment 3

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Wno-deprecated-declarations -g -O0 -D_XOPEN_SOURCE=600
LDFLAGS =

# Library files
LIB_OBJS = t_lib.o
LIB_NAME = libudthread.a

# Test programs
# Uncomment as you complete each phase:
TESTS = test_phase1
TESTS += test_phase2
TESTS += test_phase3
# TESTS += test_phase4_mailbox test_phase4_messaging
TESTS += rendezvous partialorder

# For complete testing (uncomment when all phases done):
# TESTS = test_phase1 test_phase2 test_phase3 \
#         test_phase4_mailbox test_phase4_messaging \
#         rendezvous partialorder

# Default target: build library and tests
all: $(LIB_NAME) $(TESTS)

# Build the static library
$(LIB_NAME): $(LIB_OBJS)
	@echo "Building thread library: $(LIB_NAME)"
	ar rcs $(LIB_NAME) $(LIB_OBJS)
	@echo "Library built successfully"

# Compile library object files
t_lib.o: t_lib.c t_lib.h ud_thread.h
	$(CC) $(CFLAGS) -c t_lib.c -o t_lib.o

# Build test programs
test_phase1: test_phase1.c $(LIB_NAME)
	@echo "Building test_phase1"
	$(CC) $(CFLAGS) test_phase1.c -o test_phase1 -L. -ludthread $(LDFLAGS)

test_phase2: test_phase2.c $(LIB_NAME)
	@echo "Building test_phase2"
	$(CC) $(CFLAGS) test_phase2.c -o test_phase2 -L. -ludthread $(LDFLAGS)

test_phase3: test_phase3.c $(LIB_NAME)
	@echo "Building test_phase3"
	$(CC) $(CFLAGS) test_phase3.c -o test_phase3 -L. -ludthread $(LDFLAGS)

test_phase4_mailbox: test_phase4_mailbox.c $(LIB_NAME)
	@echo "Building test_phase4_mailbox"
	$(CC) $(CFLAGS) test_phase4_mailbox.c -o test_phase4_mailbox -L. -ludthread $(LDFLAGS)

test_phase4_messaging: test_phase4_messaging.c $(LIB_NAME)
	@echo "Building test_phase4_messaging"
	$(CC) $(CFLAGS) test_phase4_messaging.c -o test_phase4_messaging -L. -ludthread $(LDFLAGS)

rendezvous: rendezvous.c $(LIB_NAME)
	@echo "Building rendezvous"
	$(CC) $(CFLAGS) rendezvous.c -o rendezvous -L. -ludthread $(LDFLAGS)

partialorder: partialorder.c $(LIB_NAME)
	@echo "Building partialorder"
	$(CC) $(CFLAGS) partialorder.c -o partialorder -L. -ludthread $(LDFLAGS)

# Run all tests (only runs tests that are built)
test: all
	@echo ""
	@echo "========================================="
	@echo "Running Phase 1 Test"
	@echo "========================================="
	@if [ -f test_phase1 ]; then ./test_phase1; fi
	@echo ""
	@if [ -f test_phase2 ]; then \
		echo "========================================="; \
		echo "Running Phase 2 Test"; \
		echo "========================================="; \
		./test_phase2; \
		echo ""; \
	fi
	@if [ -f test_phase3 ]; then \
		echo "========================================="; \
		echo "Running Phase 3 Test"; \
		echo "========================================="; \
		./test_phase3; \
		echo ""; \
	fi
	@if [ -f test_phase4_mailbox ]; then \
		echo "========================================="; \
		echo "Running Phase 4 Mailbox Test"; \
		echo "========================================="; \
		./test_phase4_mailbox; \
		echo ""; \
	fi
	@if [ -f test_phase4_messaging ]; then \
		echo "========================================="; \
		echo "Running Phase 4 Messaging Test"; \
		echo "========================================="; \
		./test_phase4_messaging; \
		echo ""; \
	fi
	@if [ -f rendezvous ]; then \
		echo "========================================="; \
		echo "Running Rendezvous Problem"; \
		echo "========================================="; \
		./rendezvous; \
		echo ""; \
	fi
	@if [ -f partialorder ]; then \
		echo "========================================="; \
		echo "Running Partial Order Problem"; \
		echo "========================================="; \
		./partialorder; \
	fi

# Run individual test phases
test1: test_phase1
	./test_phase1

test2: test_phase2
	./test_phase2

test3: test_phase3
	./test_phase3

test4: test_phase4_mailbox test_phase4_messaging
	./test_phase4_mailbox
	@echo ""
	./test_phase4_messaging

test_homework: rendezvous partialorder
	./rendezvous
	@echo ""
	./partialorder

# Clean up build artifacts
clean:
	@echo "Cleaning build artifacts"
	rm -f $(LIB_OBJS) $(LIB_NAME) $(TESTS)
	rm -f *.o *~ core
	rm -rf *.dSYM

# Show help
help:
	@echo "UD Thread Library - Makefile targets:"
	@echo ""
	@echo "  make all          - Build library and all uncommented test programs"
	@echo "  make clean        - Remove all build artifacts"
	@echo "  make test         - Build and run all tests that are uncommented"
	@echo "  make test1        - Run Phase 1 test only"
	@echo "  make test2        - Run Phase 2 test only"
	@echo "  make test3        - Run Phase 3 test only"
	@echo "  make test4        - Run Phase 4 tests only"
	@echo "  make test_homework - Run homework problems (rendezvous & partial order)"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "To enable more tests, uncomment them in the TESTS variable at the top"

.PHONY: all clean test test1 test2 test3 test4 test_homework help
