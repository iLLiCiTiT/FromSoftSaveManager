#!/bin/bash
# Send a Python job to a running ghserver.py and print its output.
#
# Usage: q.sh <name> <<'EOF'
#   print(decomp(0x1404c5350))
#   EOF
#
# RE_DIR must point at the folder ghserver.py was started with (it holds jobs/).
RE_DIR="${RE_DIR:-C:/Users/iLLiCiT/tools/fromsoft-re}"
mkdir -p "$RE_DIR/jobs"
n="${1:-job}_$(date +%s%N)"
cat > "$RE_DIR/jobs/$n.tmp"
mv "$RE_DIR/jobs/$n.tmp" "$RE_DIR/jobs/$n.py"
until [ -f "$RE_DIR/jobs/$n.out" ]; do sleep 0.5; done
cat "$RE_DIR/jobs/$n.out"
rm -f "$RE_DIR/jobs/$n.out"
