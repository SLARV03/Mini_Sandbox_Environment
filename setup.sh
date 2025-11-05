#!/bin/bash
set -e
set -o pipefail

ROOT="sandbox_env"
DEPS=(build-essential libseccomp-dev debootstrap busybox)

if [ -d "$ROOT" ]; then
    echo "[!] Detected existing sandbox environment: $ROOT"
    read -p "Do you want to remove and rebuild it? (y/n): " confirm
    if [[ "$confirm" =~ ^[Yy]$ ]]; then
        echo "[*] Removing old sandbox..."
        sudo rm -rf "$ROOT"
    else
        echo "[✓] Keeping existing sandbox. Skipping rebuild."
        SKIP_BUILD=1
    fi
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

# Build filesystem 
if [ -z "$SKIP_BUILD" ]; then
    echo ""
    echo "[+] Building minimal sandbox filesystem..."
    sudo bash scripts/build_fs.sh
else
    echo "[✓] Skipped filesystem rebuild (reusing existing sandbox)."
fi

# Build C binaries
echo ""
echo "[+] Building C source files with make..."
make

echo ""
echo "[✓] Setup complete!"
echo ""
echo "----------------------------------------------"
echo "          [+] Launching Sandbox..."
echo "----------------------------------------------"
sudo ./scripts/run_sandbox.sh "$ROOT" /bin/sh
