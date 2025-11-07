#!/bin/bash
set -e

if [ $# -lt 3 ]; then
  echo "Usage: $0 <rootfs> <mode> <command> [args...]"
  exit 1
fi

ROOTFS=$1
MODE=$2
shift 2
CMD=$@

# === Default limits (if not already exported) ===
export SANDBOX_RLIMIT_CPU="${SANDBOX_RLIMIT_CPU:-3}"            # seconds
export SANDBOX_RLIMIT_AS="${SANDBOX_RLIMIT_AS:-268435456}"      # 256MB
export SANDBOX_RLIMIT_NOFILE="${SANDBOX_RLIMIT_NOFILE:-64}"     # open files
export SANDBOX_RLIMIT_NPROC="${SANDBOX_RLIMIT_NPROC:-8}"        # processes

echo "[+] Starting sandbox with root: $ROOTFS"
echo "[+] Using seccomp mode: $MODE"
echo "[+] Command: $CMD"
echo "[+] Resource Limits:"
echo "    CPU:    ${SANDBOX_RLIMIT_CPU}s"
echo "    Memory: $((${SANDBOX_RLIMIT_AS}/1048576)) MB"
echo "    NOFILE: ${SANDBOX_RLIMIT_NOFILE}"
echo "    NPROC:  ${SANDBOX_RLIMIT_NPROC}"

sudo --preserve-env=SANDBOX_RLIMIT_CPU,SANDBOX_RLIMIT_AS,SANDBOX_RLIMIT_NOFILE,SANDBOX_RLIMIT_NPROC \
    ./build/sandbox_cli "$ROOTFS" "$MODE" $CMD
