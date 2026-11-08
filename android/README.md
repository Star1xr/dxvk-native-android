# DXVK native – Android port (D3D9 -> Vulkan)

Builds `libdxvk_d3d9.so` for `arm64-v8a`, `armeabi-v7a`, `x86_64`, `x86`.
Only the D3D9 frontend is built (DXGI/D3D10/D3D11 are off), which is what Source-engine
`shaderapidx9` needs.

## Build

Host needs: meson, ninja, glslangValidator, python3, Android NDK r23+ (r25+ recommended).

    export ANDROID_NDK_HOME=/path/to/android-ndk-r26d
    export SDL2_INCLUDE=/path/to/SDL/include        # SDL.h or SDL2/SDL.h
    export SDL2_LIB_DIR=/path/to/libs/arm64-v8a     # dir with libSDL2.so for that ABI
    ./android/build-android.sh arm64-v8a            # optional: API (default 24), out dir
    ./android/build-android.sh armeabi-v7a

Output: `build-android/<abi>/lib/libdxvk_d3d9.so` -> copy to `jniLibs/<abi>/`.

* API level must be **>= 24** (Vulkan). Gate loading at runtime on Android 7.0+ / Vulkan support.
* libc++ is linked statically and everything except `Direct3DCreate9*` / `D3DPERF_*` is hidden,
  so it can live next to an engine built with gnustl (NDK r10e) without symbol clashes.
* `libSDL2.so` is **not** bundled: link against (and ship) the same libSDL2.so as the host app,
  so DXVK talks to the same SDL instance as the engine.

## What was changed vs. upstream

* meson: `android` platform (uses the Linux util sources, NDK `libvulkan`, external SDL2 paths,
  hidden visibility, `-llog`), new options `dxvk_android_sdl2_{include,lib}_dir`.
* `include/native/android/SDL2/*`: `<SDL2/SDL.h>` -> `<SDL.h>` forwarding shim (only used when the
  real `SDL2/` directory is not on the include path).
* x86-only code guarded for ARM: `x86intrin.h`, `tzcnt` inline asm, SSE `bcmpeq`, `replaceNaN`,
  `_mm_pause` (-> `yield`).
* Logger mirrors to logcat (tag `DXVK`) because stderr is discarded in app processes.

## Using it from an engine

1. `dlopen("libdxvk_d3d9.so")` + `dlsym("Direct3DCreate9")` (or `Direct3DCreate9Ex`).
   Compile the engine's D3D9 code against `include/native/directx` + `include/native/windows`
   instead of a GL translation layer (togl/togles).
2. `D3DPRESENT_PARAMETERS::hDeviceWindow` must be an `SDL_Window*` created with
   `SDL_WINDOW_VULKAN` (not `SDL_WINDOW_OPENGL`).
3. Env vars: `DXVK_LOG_LEVEL`, `DXVK_LOG_PATH`, `DXVK_CONFIG_FILE`, `DXVK_HUD` work as upstream.
   Use a writable path (app files dir) for `DXVK_LOG_PATH` and the state cache
   (`DXVK_STATE_CACHE_PATH`).
4. DXVK 1.9.x expects desktop-class Vulkan features (e.g. geometry/tessellation shaders, BC texture
   compression). Many mobile drivers do not expose them; check with `vulkaninfo` / the DXVK log.
