#!/bin/bash
set -e
set -o pipefail

ROOT_BASE="sandbox_env"       # Base directory for all sandboxes
DEPS=(build-essential libseccomp-dev debootstrap busybox)

# Ask for number of instances
read -p "[?] Enter number of sandbox instances to create: " INSTANCE_COUNT
if ! [[ "$INSTANCE_COUNT" =~ ^[0-9]+$ && "$INSTANCE_COUNT" -gt 0 ]]; then
    echo "[!] Invalid number of instances."
    exit 1
fi

# Check & install dependencies
echo ""
echo "[*] Checking required dependencies..."
MISSING=()
for pkg in "${DEPS[@]}"; do
    if ! dpkg -s "$pkg" >/dev/null 2>&1; then
        MISSING+=("$pkg")
    fi
done

if [ ${#MISSING[@]} -eq 0 ]; then
    echo "[✓] All dependencies already installed."
else
    echo "[!] Missing dependencies: ${MISSING[*]}"
    echo "[+] Updating package lists..."
    sudo apt update -y
    echo "[+] Installing missing packages..."
    sudo apt install -y "${MISSING[@]}"
fi

# Build sandboxes
echo ""
echo "[+] Building $INSTANCE_COUNT sandbox instance(s)..."
for ((i=1; i<=INSTANCE_COUNT; i++)); do
    ROOT="${ROOT_BASE}_$i"
    echo ""
    echo "----------------------------------------------"
    echo "[*] Setting up sandbox instance $i at $ROOT"
    echo "----------------------------------------------"

    if [ -d "$ROOT" ]; then
        echo "[!] Detected existing sandbox environment: $ROOT"
        read -p "Do you want to remove and rebuild it? (y/n): " confirm
        if [[ "$confirm" =~ ^[Yy]$ ]]; then
            echo "[*] Removing old sandbox $i..."
            sudo rm -rf "$ROOT"
        else
            echo "[✓] Keeping existing sandbox $i. Skipping rebuild."
            SKIP_BUILD=1
        fi
    else
        unset SKIP_BUILD
    fi

    if [ -z "$SKIP_BUILD" ]; then
        echo "[+] Building minimal sandbox filesystem for instance $i..."
        sudo bash scripts/build_fs.sh "$ROOT"
    else
        echo "[✓] Skipped filesystem rebuild for instance $i (reusing existing sandbox)."
    fi
done

# Build C binaries
echo ""
echo "[+] Building C source files with make..."
make

echo ""
echo "[✓] All sandboxes built successfully!"
echo ""
echo "----------------------------------------------"
echo "          [+] Launching Sandbox..."
echo "----------------------------------------------"

# Ask which instance to run
read -p "[?] Enter sandbox instance number to run (1-$INSTANCE_COUNT): " RUN_INSTANCE
if ! [[ "$RUN_INSTANCE" =~ ^[0-9]+$ && "$RUN_INSTANCE" -le "$INSTANCE_COUNT" && "$RUN_INSTANCE" -gt 0 ]]; then
    echo "[!] Invalid instance number."
    exit 1
fi

ROOT="${ROOT_BASE}_$RUN_INSTANCE"
sudo ./scripts/run_sandbox.sh "$ROOT" /bin/sh
