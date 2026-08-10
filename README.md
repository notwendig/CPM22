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
- automatic CP/M 2.2, CBIOS and boot-sector assembly with `zmac`
- automatic `.cpm22.sys` creation with size validation
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
- the Z80 CMake project/package providing `Z80::Z80`
- PuTTY for the historical default console launcher

With the usual `zilogz80-code` layout, CPM22 automatically uses the sibling `../Z80`
source tree when no installed Z80 CMake package is available.

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
│   ├── bios.cim
│   ├── cpm22.cim
│   ├── zexdoc.cim
│   └── zexall.cim
└── disks/
    ├── drivea/
    │   ├── .cpm22.sys
    │   ├── zexdoc.com
    │   └── zexall.com
    └── drived/
```

## CMake targets

- `CPM22` — host emulator executable (`cpm-2.2`)
- `CPM22_images` — CP/M system image plus ZEX programs
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
