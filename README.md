# CPM22

[![CMake CI](https://github.com/notwendig/CPM22/actions/workflows/cmake.yml/badge.svg)](https://github.com/notwendig/CPM22/actions/workflows/cmake.yml)

A host-side **CP/M 2.2 environment for the Z80**, combining a C++ emulator front end,
a CBIOS/host-filesystem bridge, a TCP console, and reproducible assembly of the CP/M
system image with `zmac`.

The historical Makefile build has been replaced by CMake. Generated system tracks,
ZEX test programs, generated headers, and runtime disk trees all live in the build
directory rather than modifying the source tree.

## Highlights

- CMake-only host build with Debug and Release presets
- CP/M image build isolated in the `cpm22/` CMake subdirectory
- separate assembly of boot loader, CCP, BDOS and BIOS with `zmac`
- automatic `.cpm22.sys` creation with component and total-size validation
- ZEXDOC and ZEXALL assembled onto drive A
- generated C++ boot-loader header
- host directories exposed as CP/M drives
- TCP console on port 1234
- CTest validation of all critical build artifacts
- GitHub Actions CI for Debug and Release builds
- strict compiler warnings for CPM22-owned C++ code

## Requirements

- Linux or another POSIX-like host
- CMake 3.21 or newer
- Ninja
- a C++20 compiler
- POSIX threads
- `zmac`
- the initialized `Z80/` git submodule providing `Z80::Z80`
- PuTTY for the historical default console launcher

Clone the repository with its submodule, or initialize it afterwards:

```sh
git clone --recurse-submodules git@github.com:notwendig/CPM22.git
# Existing clone:
git submodule update --init --recursive
```

If the Z80 source tree is elsewhere, configure with
`-DZ80_SOURCE_DIR=/path/to/Z80`.

## System image layout

The `cpm22/` subdirectory owns the complete assembly pipeline. It creates the system
image by concatenating the independently assembled components in this order:

```text
boot.cim + ccp.cim + bdos.cim + bios.cim -> disks/A/.cpm22.sys
```

| Component | Source | Output | Required size |
|---|---|---|---:|
| Boot sector | `cpm22/boot.asm` | `zout/boot.cim` | 128 bytes |
| CCP | `cpm22/ccp.asm` | `zout/ccp.cim` | `0x806` = 2054 bytes |
| BDOS | `cpm22/bdos.asm` | `zout/bdos.cim` | `0xDFA` = 3578 bytes |
| BIOS | `cpm22/bios.asm` | `zout/bios.cim` | at most 896 bytes |

The complete image must fit into two tracks with 26 sectors of 128 bytes each:
`2 * 26 * 128 = 6656` bytes.

## Build

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Release:

```sh
cmake --preset release
cmake --build --preset release
ctest --preset release
```

The Debug executable is:

```text
build/Desktop_Debug/cpm-2.2
```

## Run

```sh
cd build/Desktop_Debug
./cpm-2.2
```

The emulator listens on TCP port 1234. The historical launcher starts the saved PuTTY
session named `CPM`.

You can also use the CMake convenience target:

```sh
cmake --build --preset debug --target run
```

At the CP/M prompt, useful first checks are:

```text
A>DIR
A>ZEXDOC
A>ZEXALL
```

## Generated build tree

```text
build/Desktop_Debug/
├── cpm-2.2
├── generated/
│   └── boot.h
├── zout/
│   ├── boot.cim
│   ├── ccp.cim
│   ├── bdos.cim
│   ├── bios.cim
│   ├── zexdoc.cim
│   └── zexall.cim
└── disks/
    ├── A/
    │   ├── .cpm22.sys
    │   ├── zexdoc.com
    │   └── zexall.com
    ├── B/
    ├── I/
    └── J/
```

## CMake targets

- `CPM22` — host emulator executable (`cpm-2.2`)
- `CPM22_images` — boot header, CP/M system image and ZEX programs
- `run` — build and launch the emulator using the generated disk tree

## Console configuration

For the raw TCP console, PuTTY must not perform local line editing. Backspace should
send Control-H. See [docs/CONSOLE.md](docs/CONSOLE.md) for the tested settings and
current limitations of cursor/Delete key handling.

## Development

See [CONTRIBUTING.md](CONTRIBUTING.md) and [docs/BUILDING.md](docs/BUILDING.md).
The CI build enables `CPM22_WARNINGS_AS_ERRORS=ON` so new warnings in CPM22-owned
sources fail the build.

## History

The project originates from the earlier Makefile-based CP/M 2.2 emulator tree. The
migration details and removed legacy assumptions are documented in
[MIGRATION.md](MIGRATION.md).

## Licensing and third-party material

The historical source tree contains GNU GPL notices in the CPM22-owned C++ sources and
already shipped a GPLv3 license text; that text is exposed as [LICENSE](LICENSE) for
standard GitHub discovery. Individual file headers remain authoritative.

The repository also contains historical CP/M, Turbo Pascal, manuals, binaries, and
other third-party material whose redistribution terms are separate from CPM22's own
source code. **Before publishing a public repository, review [THIRD_PARTY.md](THIRD_PARTY.md).**
