#!/bin/bash
set -e

# Allow dynamic ROOT path (default = sandbox_env)
if [ -n "$1" ]; then
  ROOT="$1"
else
  ROOT="sandbox_env"
fi

echo "[+] Preparing sandbox environment at $ROOT..."

# Check if sandbox exists
if [ -d "$ROOT" ]; then
  echo "[!] Existing sandbox found at: $ROOT"
  read -p "Do you want to overwrite it? (y/N): " confirm
  if [[ "$confirm" != "y" && "$confirm" != "Y" ]]; then
    echo "[✓] Keeping existing sandbox. Skipping rebuild."
    exit 0
  fi
  echo "[*] Removing old sandbox..."
  sudo rm -rf "$ROOT"
fi

echo "[+] Building minimal root filesystem..."
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
for dev in null zero tty random urandom; do
  case $dev in
    null) major=1; minor=3 ;;
    zero) major=1; minor=5 ;;
    tty) major=5; minor=0 ;;
    random) major=1; minor=8 ;;
    urandom) major=1; minor=9 ;;
  esac
  sudo mknod -m 666 "$ROOT/dev/$dev" c $major $minor 2>/dev/null || true
done

# Install symlinks using busybox (if chroot works)
echo "[+] Installing BusyBox applets..."
sudo chroot "$ROOT" /bin/busybox --install -s /bin || echo "[!] chroot skipped, will use /bin/busybox directly."

echo "[✓] Minimal root filesystem created successfully at $ROOT!"
