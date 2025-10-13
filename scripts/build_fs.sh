#!/bin/bash
set -e

ROOT="sandbox_env"

echo "[+] Building minimal root filesystem at $ROOT..."
sudo rm -rf "$ROOT"
mkdir -p "$ROOT"/{bin,lib,lib64,proc,sys,dev,etc,usr,tmp}

# Copy busybox
if ! command -v busybox >/dev/null 2>&1; then
  echo "[!] Installing busybox..."
  sudo apt update && sudo apt install busybox -y
fi

cp /bin/busybox "$ROOT/bin/"

# Copy busybox shared libraries
echo "[+] Copying required shared libraries..."
ldd /bin/busybox | grep "=>" | while read -r line; do
  lib=$(echo "$line" | awk '{print $3}')
  if [ -f "$lib" ]; then
    dest="$ROOT$(dirname "$lib")"
    mkdir -p "$dest"
    cp -u "$lib" "$dest/"
  fi
done

# Copy loader
if [ -f /lib64/ld-linux-x86-64.so.2 ]; then
  mkdir -p "$ROOT/lib64"
  cp /lib64/ld-linux-x86-64.so.2 "$ROOT/lib64/"
fi

# Set hostname and minimal etc
echo "sandbox" | sudo tee "$ROOT/etc/hostname" > /dev/null

# Create device nodes (ignore errors if not supported)
sudo mknod -m 666 "$ROOT/dev/null" c 1 3 2>/dev/null || true
sudo mknod -m 666 "$ROOT/dev/zero" c 1 5 2>/dev/null || true
sudo mknod -m 666 "$ROOT/dev/tty" c 5 0 2>/dev/null || true
sudo mknod -m 666 "$ROOT/dev/random" c 1 8 2>/dev/null || true
sudo mknod -m 666 "$ROOT/dev/urandom" c 1 9 2>/dev/null || true

# Install symlinks using busybox (if chroot works)
echo "[+] Installing BusyBox applets..."
sudo chroot "$ROOT" /bin/busybox --install -s /bin || echo "[!] chroot skipped, will use /bin/busybox directly."

echo "[✓] Minimal root filesystem created successfully!"
