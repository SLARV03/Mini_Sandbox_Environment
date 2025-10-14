#!/bin/bash
set -e

echo "[+] Updating package lists..."
sudo apt update -y

echo ""
echo "[+] Installing dependencies..."
#VERY IMPORTANT, WRITE ALL DEPENDENCIES IN THE NEXT LINE
sudo apt install -y build-essential libseccomp-dev debootstrap busybox

echo ""
echo "[+] Preparing minimal sandbox root..."
bash scripts/build_fs.sh

echo ""
echo "[+] Building C source files with make..."
make

echo ""
echo "[✓] Setup complete."
echo ""
echo "You can now run your sandbox using:"
echo "---------------------------------------------------"
echo "sudo ./scripts/run_sandbox.sh sandbox_env /bin/sh"
echo "---------------------------------------------------"