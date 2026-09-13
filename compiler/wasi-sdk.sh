#!/bin/sh
# Fetch wasi-sdk into ~/.h2o/wasi-sdk (not vendored in the repo).
set -u
VER=34.0
TAG=wasi-sdk-34
DEST=${H2O_WASI_SDK-}
if [ -z "$DEST" ]; then
	DEST="$HOME/.h2o/wasi-sdk"
fi
if [ -x "$DEST/bin/clang" ]; then
	echo "$DEST"
	exit 0
fi
os=$(uname -s)
arch=$(uname -m)
case $os in
	Darwin) os=macos ;;
	Linux) os=linux ;;
	*) echo "wasi-sdk.sh: unsupported OS $os" >&2; exit 1 ;;
esac
case $arch in
	arm64|aarch64) arch=arm64 ;;
	x86_64|amd64) arch=x86_64 ;;
	*) echo "wasi-sdk.sh: unsupported arch $arch" >&2; exit 1 ;;
esac
url="https://github.com/WebAssembly/wasi-sdk/releases/download/${TAG}/wasi-sdk-${VER}-${arch}-${os}.tar.gz"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
echo "fetch $url" >&2
if command -v curl >/dev/null 2>&1; then
	curl -fsSL "$url" -o "$tmp/sdk.tgz"
elif command -v wget >/dev/null 2>&1; then
	wget -q "$url" -O "$tmp/sdk.tgz"
else
	echo "need curl or wget" >&2
	exit 1
fi
mkdir -p "$HOME/.h2o"
tar -xzf "$tmp/sdk.tgz" -C "$tmp"
inner=$(find "$tmp" -maxdepth 1 -type d -name 'wasi-sdk-*' | head -1)
if [ -z "$inner" ]; then
	echo "wasi-sdk.sh: unexpected tarball layout" >&2
	exit 1
fi
rm -rf "$DEST"
mv "$inner" "$DEST"
echo "$DEST"
