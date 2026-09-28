# Building CPM22

## Repository layout

```text
CPM22/
├── Z80/                 # git submodule
├── cmake/               # image-generation and verification scripts
├── cpm22/               # boot, CCP, BDOS, BIOS and ZEX assembly sources
├── disks/               # source runtime disks A, B, I and J
└── src/                 # host-side emulator
```

Initialize the Z80 submodule after cloning:

```sh
git submodule update --init --recursive
```

If Z80 is elsewhere, pass its source tree explicitly:

```sh
cmake -S . -B build/custom -G Ninja \
  -DZ80_SOURCE_DIR=/path/to/Z80 \
  -DCMAKE_BUILD_TYPE=Debug
```

The selected Z80 source tree must provide the CMake target `Z80::Z80`.

## zmac

`zmac` must be on `PATH` (or in `$HOME/bin`). A typical source build is:

```sh
git clone https://github.com/gp48k/zmac.git
make -C zmac/src -j1
install -m 0755 zmac/src/zmac "$HOME/bin/zmac"
```

The upstream Makefile uses `bison -y`; install a C compiler, `make`, and `bison` before
building it.

## CP/M system image

The top-level build delegates the assembly pipeline with `add_subdirectory(cpm22)`.
The subdirectory assembles these independent files:

| Source | Generated file | Checked size |
|---|---|---:|
| `cpm22/boot.asm` | `zout/boot.cim` | 128 bytes |
| `cpm22/ccp.asm` | `zout/ccp.cim` | `0x806` = 2054 bytes |
| `cpm22/bdos.asm` | `zout/bdos.cim` | `0xDFA` = 3578 bytes |
| `cpm22/bios.asm` | `zout/bios.cim` | at most 896 bytes |

`cmake/BuildSystemImage.cmake` concatenates the results as
`boot + CCP + BDOS + BIOS`. The resulting `disks/A/.cpm22.sys` may occupy at most
`2 * 26 * 128 = 6656` bytes. Configuration calculates all derived sizes with CMake
`math(EXPR ...)`; the build verifies the actual assembled file sizes.

## Presets

```sh
cmake --list-presets
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

The project declares CMake 3.21 as its minimum and therefore intentionally uses preset
schema version 3.

## Strict build

```sh
cmake -S . -B build/strict -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCPM22_WARNINGS_AS_ERRORS=ON
cmake --build build/strict
ctest --test-dir build/strict --output-on-failure
```

## Reconfigure after changing the assembly layout

CMake normally detects changes automatically. If an older build directory still
contains rules for the former combined `cpm22.cim`, configure the same build directory
again before building:

```sh
cmake -S . -B build/Desktop_Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build/Desktop_Debug
ctest --test-dir build/Desktop_Debug --output-on-failure
```
