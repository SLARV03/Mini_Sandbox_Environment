CC = gcc
CFLAGS = -O2 -Wall -Wextra -static
LDFLAGS = -lseccomp

SRC_DIR=src
BUILD_DIR=build

SRC = $(SRC_DIR)/main.c $(SRC_DIR)/isolation.c $(SRC_DIR)/resources.c $(SRC_DIR)/seccomp.c $(SRC_DIR)/utils.c
OBJ = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC))
OUT = $(BUILD_DIR)/sandbox_cli

all: $(OUT)

$(OUT): $(OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

phase1:
	$(MAKE) all

phase2:
	@echo "Phase 2: add cgroups/seccomp"

phase3:
	@echo "Phase 3: automation and monitoring"


#Reset: clean everything and rebuild environment
reset:
	@echo "[+] Resetting project environment..."
	@bash cleanup.sh
	@bash setup.sh

#test:
#	@bash tests/run_tests.sh

#not actual files
.PHONY: all clean phase1 phase2 phase3 reset