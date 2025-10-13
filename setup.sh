#!/bin/bash
set -e

echo "[+] Installing dependencies..."
sudo apt update -y

#VERY IMPORTANT, WRITE ALL DEPENDENCIES IN THE NEXT LINE
sudo apt install -y build-essential libseccomp-dev debootstrap busybox

echo "[+] Preparing minimal sandbox root..."
bash scripts/build_fs.sh

echo "[✓] Setup complete. You can now run:"
echo "    make && sudo ./scripts/run_sandbox.sh sandbox_env /bin/sh"
