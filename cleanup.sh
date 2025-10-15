#cleans up old build files and sandbox environment (sandox_env)
#!/bin/bash

set -e
set -o pipefail

SANDBOX_DIR="sandbox_env"
BUILD_DIR="build"

echo "[!] This will delete:"
echo "  -$SANDBOX_DIR/ (sandbox root filesystem)"
echo "  -$BUILD_DIR/ (compiled binaries)"
echo ""

#safety meaure
read -p "Are you sure you want to continue? (y/n): " confirm

if [[ "$confirm" != "y" && "$confirm" != "Y" ]]; then
    echo "[X] Cleanup cancelled."
    exit 0
fi

echo "[+] Cleaning up sandbox environment..."

#unmounting mounted dirs
for dir in proc sys dev; do 
    if mountpoint -q "$SANDBOX_DIR/$dir";then
        echo "[*] Unmounting /$dir..."
        sudo unmount -l "$SANDBOX_DIR/$dir"
    fi
done

#remove sandbox_env
if [ -d "$SANDBOX_DIR" ]; then
    echo "[*] Removing sandbox environment directory..."
    sudo rm -rf "$SANDBOX_DIR"
fi

#remove build dir
if [ -d "$BUILD_DIR" ]; then
    echo "[*] Removing build directory..."
    sudo rm -rf "$BUILD_DIR"
fi

#cleaning leftover .0 files in src/
find src -type f -name "*.o" -exec rm -f {} \;

echo ""
echo "[✓] Cleanup complete."
echo ""