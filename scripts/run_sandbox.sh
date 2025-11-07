#!/bin/bash
set -e

# Usage: ./scripts/run_sandbox.sh <ROOT> <COMMAND>
# Example: ./scripts/run_sandbox.sh sandbox_env_1 /bin/sh

ROOT="${1:-sandbox_env}"
CMD="${2:-/bin/sh}"

if [ ! -d "$ROOT" ]; then
    echo "[!] Sandbox root not found at: $ROOT"
    exit 1
fi

echo "[+] Launching sandbox at $ROOT"
echo "[*] Command: $CMD"

# Create required mount points inside sandbox
sudo mount -t proc none "$ROOT/proc" 2>/dev/null || true
sudo mount -t sysfs none "$ROOT/sys" 2>/dev/null || true
sudo mount --bind /dev "$ROOT/dev" 2>/dev/null || true
sudo mount --bind /tmp "$ROOT/tmp" 2>/dev/null || true

# Unique namespace for each sandbox (PID, UTS, Mount)
echo "[+] Starting isolated sandbox environment..."
sudo unshare --fork --pid --mount --uts --ipc --net \
    --mount-proc="$ROOT/proc" \
    chroot "$ROOT" "$CMD"

# Cleanup after exit
echo "[*] Cleaning up mounts for $ROOT..."
sudo umount -l "$ROOT/proc" 2>/dev/null || true
sudo umount -l "$ROOT/sys" 2>/dev/null || true
sudo umount -l "$ROOT/dev" 2>/dev/null || true
sudo umount -l "$ROOT/tmp" 2>/dev/null || true

echo "[✓] Sandbox at $ROOT exited cleanly."
