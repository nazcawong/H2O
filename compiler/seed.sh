#!/bin/sh
# Compile the portable C snapshot into bin/h2o. No Python.
set -u
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SEED="$ROOT/compiler/seed.c"
IMG="$ROOT/bin/h2o"
if [ ! -f "$SEED" ]; then
	echo "missing $SEED" >&2
	exit 1
fi
if ! command -v cc >/dev/null 2>&1; then
	echo "need cc to seed bin/h2o from compiler/seed.c" >&2
	exit 1
fi
mkdir -p "$ROOT/bin"
err="$ROOT/bin/seed-cc.err"
if cc -std=c11 -O1 -o "$IMG" "$SEED" 2>"$err"; then
	rm -f "$err"
	exit 0
fi
if cc -std=c11 -O1 -o "$IMG" "$SEED" -ldl 2>"$err"; then
	rm -f "$err"
	exit 0
fi
cat "$err" >&2
exit 1
