# fbneo-vita

`fbneo-vita` is a PS Vita frontend built around `FBNeo`.

This repository is Vita-only. It contains:

- the Vita application code
- the RGUI-based frontend and in-game menu
- `FBNeo` as a git submodule
- `libcross2d` as a git submodule
- packaged Vita assets and UI resources

## Features

- PS Vita target only
- `FBNeo` core only
- RGUI frontend and in-game menu
- per-game and global configuration support
- Vita packaging output as `fbneo-vita.vpk`

## Requirements

- CMake
- Ninja or Make
- a working VitaSDK installation
- VitaSDK tools available through `VITASDK`

Typical environment:

```bash
export VITASDK=/path/to/vitasdk
export PATH="$VITASDK/bin:$PATH"
```

## Build

Initialize submodules:

```bash
git submodule update --init --recursive
```

Configure:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release \
  -DPLATFORM_VITA=ON
```

Build the VPK:

```bash
cmake --build build --target fbneo-vita.vpk -j"$(nproc)"
```

Output:

- `build/fbneo-vita.vpk`

## Vita Package Assets

The `fbneo-vita.vpk` target copies `data/vita/romfs` into the package, so changes
under `data/vita/romfs/sce_sys` are included automatically.

- `data/vita/romfs/sce_sys/icon0.png`
  Bubble icon (`128x128`).
- `data/vita/romfs/sce_sys/pic0.png`
  Package image shown on the Vita info screen (`960x544`).
- `data/vita/romfs/sce_sys/livearea/contents/bg.png`
  LiveArea background (`840x500`).
- `data/vita/romfs/sce_sys/livearea/contents/startup.png`
  LiveArea startup image (`280x158`).

After exporting replacement artwork, rewrite the Vita PNGs before building:

```bash
scripts/fix-vita-package-pngs.sh
```

The script strips metadata and converts the package art to indexed `PNG8`, which
avoids Vita installer issues with some truecolor PNGs.

## Optional Build Flag

- `-DOPTION_LIGHT=ON`
  Reduces the included driver set by disabling console drivers and trimming some heavier arcade content.

## Project Layout

- `src`
  Application code, RGUI, runtime layer, and Vita-specific `FBNeo` integration.
- `src/fbneo`
  Local `FBNeo` bridge code used by the frontend.
- `src/runtime`
  Runtime UI, configuration, skin loading, and shared app infrastructure.
- `data`
  Packaged fonts, skin files, LiveArea assets, and bundled data such as `hiscore.dat`.
- `scripts`
  Helper utilities, including Vita package PNG normalization.
- `external/FBNeo`
  `FBNeo` git submodule.
- `external/libcross2d`
  `libcross2d` git submodule for rendering, input, audio, and platform support.

## Runtime Data

On Vita, application data is stored under:

- `ux0:/data/fbneo-vita/`

This directory is used for configuration, saves, cheats, and other runtime-generated files.

## Notes

- This project is intended for Vita builds only.
- The repository is structured as a standalone Vita application rather than a multi-core frontend.
