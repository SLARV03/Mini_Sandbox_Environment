#!/bin/bash
set -e
if [ $# -lt 2 ]; then
  echo "Usage: $0 <rootfs> <command>"
  exit 1
fi
ROOTFS=$1; shift
CMD=$@
sudo ./build/sandbox_cli "$ROOTFS" $CMD
