#!/bin/sh
# Compile an H2O VM C image (rt.c + bytecode) to a WASI .wasm module.
# Same ISA as --vm; not a translation of H2O opcodes into Wasm.
set -u
cpath=${1-}
out=${2-}
if [ -z "$cpath" ] || [ -z "$out" ]; then
	echo "usage: sh compiler/wasi-cc.sh in.c out.wasm" >&2
	exit 2
fi

SDK=${WASI_SDK_PATH-}
if [ -z "$SDK" ]; then
	for d in \
		"$HOME/.h2o/wasi-sdk" \
		/opt/wasi-sdk \
		/opt/homebrew/opt/wasi-sdk
	do
		if [ -x "$d/bin/clang" ]; then
			SDK=$d
			break
		fi
	done
fi
if [ -z "$SDK" ] || [ ! -x "$SDK/bin/clang" ]; then
	echo "h2o: need WASI SDK (clang + wasi-libc + wasm-ld)." >&2
	echo "  sh compiler/wasi-sdk.sh" >&2
	echo "  or set WASI_SDK_PATH to a wasi-sdk root." >&2
	exit 1
fi

CC="$SDK/bin/clang"
# wasi-sdk 21+ prefers wasip1; older builds still accept wasm32-wasi.
tgt=wasm32-wasip1
if ! "$CC" --target=$tgt -E -x c /dev/null >/dev/null 2>&1; then
	tgt=wasm32-wasi
fi

exec "$CC" --target=$tgt -std=c11 -O1 -o "$out" "$cpath"
