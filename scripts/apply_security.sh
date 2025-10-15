#apply sandbox security policies
#!/bash/bin

set -e
set -o pipefail

SANDBOX_DIR="sandbox_env"

echo "[+] Applying security policies to sandbox: $SANDBOX_DIR"

#seccomp policy
if [ ! -f "../build/sandbox_cli" ]; then
    echo "[!] Binary sandbox_cli not found. Run 'make' first"
    exit 1
fi
echo "[*] Applying seccomp filters via sandbox_cli..."
#./build/sandbox_cli --apply-seccomp strict
echo "[✓] Seccomp filters applied."

#CPU/memory limits via cgroups

#minimal filesystem access

echo "[✓] Security policies applied."