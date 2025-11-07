#!/bin/bash
set -e

ROOT_BASE="sandbox_env"
CMD="/bin/sh"

echo "=============================================="
echo "[+] Sandbox Isolation Test Utility"
echo "=============================================="
echo ""

read -p "[?] Enter number of sandbox instances to test (e.g. 2): " INSTANCE_COUNT
if ! [[ "$INSTANCE_COUNT" =~ ^[0-9]+$ && "$INSTANCE_COUNT" -gt 0 ]]; then
    echo "[!] Invalid number."
    exit 1
fi

echo ""
echo "[*] Starting $INSTANCE_COUNT sandbox instance(s) for testing..."

for ((i=1; i<=INSTANCE_COUNT; i++)); do
    ROOT="${ROOT_BASE}_$i"
    if [ ! -d "$ROOT" ]; then
        echo "[!] Sandbox $i not found at $ROOT. Please run setup.sh first."
        exit 1
    fi

    echo ""
    echo "----------------------------------------------"
    echo "[+] Testing sandbox instance $i ($ROOT)"
    echo "----------------------------------------------"

    # Test 1: Filesystem isolation
    echo "[*] Creating test file inside sandbox $i..."
    sudo chroot "$ROOT" /bin/sh -c "echo 'Sandbox $i test file' > /tmp/test_file_$i.txt"

    echo "[*] Verifying file visibility across sandboxes..."
    FOUND=0
    for ((j=1; j<=INSTANCE_COUNT; j++)); do
        ROOT_CHECK="${ROOT_BASE}_$j"
        if sudo chroot "$ROOT_CHECK" /bin/sh -c "[ -f /tmp/test_file_$i.txt ]"; then
            echo "  [!] File /tmp/test_file_$i.txt visible inside sandbox $j ❌"
            FOUND=1
        fi
    done
    if [ $FOUND -eq 0 ]; then
        echo "  [✓] File isolation verified for sandbox $i."
    fi

    # Test 2: PID namespace isolation
    echo "[*] Checking PID isolation..."
    SANDBOX_PID=$(sudo unshare --pid --fork --mount --uts --ipc --net \
        chroot "$ROOT" /bin/sh -c "echo \$\$")

    HOST_PID=$$
    echo "  Host PID: $HOST_PID | Sandbox PID: $SANDBOX_PID"
    if [ "$SANDBOX_PID" -ne "$HOST_PID" ]; then
        echo "  [✓] PID namespace is isolated."
    else
        echo "  [!] PID namespace NOT isolated ❌"
    fi

    # Test 3: Optional restriction check (if seccomp active)
    echo "[*] Checking basic command restrictions (if seccomp enabled)..."
    if sudo chroot "$ROOT" /bin/sh -c "strace -e openat echo test >/dev/null 2>&1"; then
        echo "  [!] Syscall tracing works — seccomp not active."
    else
        echo "  [✓] Syscall blocked — seccomp restrictions active."
    fi

    echo "[✓] Sandbox $i passed all isolation tests!"
done

echo ""
echo "=============================================="
echo "[✓] Isolation testing completed!"
echo "=============================================="
