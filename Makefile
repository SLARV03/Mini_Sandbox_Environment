CC = gcc
CFLAGS = -O2 -Wall -Wextra -static
LDFLAGS = -lseccomp

SRC = src/main.c src/isolation.c src/resources.c src/seccomp.c src/utils.c
OBJ = $(SRC:.c=.o)
OUT = build/sandbox_cli

all: $(OUT)

$(OUT): $(OBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf build src/*.o

phase1:
	$(MAKE) all

phase2:
	@echo "Phase 2: add cgroups/seccomp (coming soon)"

phase3:
	@echo "Phase 3: automation and monitoring"