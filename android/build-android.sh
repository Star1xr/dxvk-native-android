#!/usr/bin/env bash
# Build libdxvk_d3d9.so (DXVK native, D3D9 only) for Android.
#
#   ANDROID_NDK_HOME=/path/to/ndk-r25+  \
#   SDL2_INCLUDE=/path/to/SDL/include   \
#   SDL2_LIB_DIR=/path/to/dir/with/libSDL2.so/for/that/abi \
#   ./android/build-android.sh arm64-v8a [api=24] [out=build-android]
#
# ABI: arm64-v8a | armeabi-v7a | x86_64 | x86
# Needs on the host: meson (>=0.55), ninja, glslangValidator, python3.
# Vulkan needs Android 7.0 / API 24+, so API < 24 can not be used (no libvulkan.so stub in the NDK).
set -euo pipefail

ABI="${1:?usage: $0 <abi> [api] [out]}"
API="${2:-24}"
SRC="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${3:-$SRC/build-android}/$ABI"

: "${ANDROID_NDK_HOME:?set ANDROID_NDK_HOME to an NDK r23+ (r25+ recommended)}"
: "${SDL2_INCLUDE:?set SDL2_INCLUDE to the directory containing SDL.h or SDL2/SDL.h}"
: "${SDL2_LIB_DIR:?set SDL2_LIB_DIR to the directory containing libSDL2.so for $ABI}"

case "$ABI" in
  arm64-v8a)   TRIPLE=aarch64-linux-android;   CPU_FAMILY=aarch64; CPU=armv8-a;  ;;
  armeabi-v7a) TRIPLE=armv7a-linux-androideabi; CPU_FAMILY=arm;     CPU=armv7;   ;;
  x86_64)      TRIPLE=x86_64-linux-android;    CPU_FAMILY=x86_64;  CPU=x86_64;  ;;
  x86)         TRIPLE=i686-linux-android;      CPU_FAMILY=x86;     CPU=i686;    ;;
  *) echo "unknown ABI $ABI" >&2; exit 1 ;;
esac

HOST_TAG="linux-x86_64"
case "$(uname -s)" in Darwin) HOST_TAG="darwin-x86_64" ;; esac
TOOLCHAIN="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$HOST_TAG"
BIN="$TOOLCHAIN/bin"
[ -x "$BIN/clang" ] || { echo "NDK clang not found in $BIN" >&2; exit 1; }

mkdir -p "$OUT"
CROSS="$OUT/android-cross.txt"

# armv7 clang wrapper names are armv7a-linux-androideabi<API>-clang, everything else <triple><API>-clang
CC="$BIN/${TRIPLE}${API}-clang"
CXX="$BIN/${TRIPLE}${API}-clang++"

# libc++ is linked statically (-static-libstdc++ == c++_static): the host process (Source engine,
# built against gnustl) must not share any STL symbols with DXVK.
cat > "$CROSS" <<EOT
[binaries]
c = '$CC'
cpp = '$CXX'
ar = '$BIN/llvm-ar'
strip = '$BIN/llvm-strip'
ranlib = '$BIN/llvm-ranlib'
pkgconfig = 'false'

[built-in options]
c_link_args = ['-static-libstdc++', '-Wl,--no-undefined', '-Wl,-z,max-page-size=16384']
cpp_link_args = ['-static-libstdc++', '-Wl,--no-undefined', '-Wl,-z,max-page-size=16384']

[host_machine]
system = 'android'
cpu_family = '$CPU_FAMILY'
cpu = '$CPU'
endian = 'little'
EOT

MESON_ARGS=(
  --cross-file "$CROSS"
  --buildtype release
  --strip
  -Ddxvk_native_force=true
  -Ddxvk_native_wsi=sdl2
  -Denable_dxgi=false -Denable_d3d10=false -Denable_d3d11=false -Denable_d3d9=true
  -Ddxvk_android_sdl2_include_dir="$SDL2_INCLUDE"
  -Ddxvk_android_sdl2_lib_dir="$SDL2_LIB_DIR"
)
[ -d "$OUT/build" ] && MESON_ARGS+=(--wipe)
meson setup "$OUT/build" "$SRC" "${MESON_ARGS[@]}"

ninja -C "$OUT/build"

LIB="$(find "$OUT/build" -name 'libdxvk_d3d9.so' | head -1)"
mkdir -p "$OUT/lib"
cp -v "$LIB" "$OUT/lib/"
echo "Done: $OUT/lib/libdxvk_d3d9.so (jniLibs/$ABI/)"
