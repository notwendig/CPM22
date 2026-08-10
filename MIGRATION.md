# Makefile to CMake migration

## Original targets and their CMake equivalents

| Makefile rule | CMake equivalent |
|---|---|
| `all` | default build (`CPM22_images` + `CPM22`) |
| `zout/%.cim` | `cpm22_assemble()` custom commands |
| `src/boot.cpp` via `srec_cat` | generated `generated/boot.h` via `BinaryToHeader.cmake` |
| `.cpm22.sys` concatenation | `BuildSystemImage.cmake` with size validation |
| `zexdoc.com`, `zexall.com` | generated drive-A files under the build tree |
| object/dependency rules | native CMake compiler dependency tracking |
| `cpm-2.2` link | target `CPM22`, output name `cpm-2.2` |
| `clean` | `cmake --build <build-dir> --target clean` |

## Preserved constants

- boot sector size: 128 bytes
- CCP + BDOS size: 5632 bytes
- maximum system-track image: 6656 bytes
- CP/M system filename: `.cpm22.sys`

## Deliberately removed legacy settings

- hard-coded `/home/juergen/lib`
- manual `gcc`/`gcc` C++ compile and link commands
- explicit `-lstdc++`
- legacy `cpmfs_dbg` link dependency
- unused `BLVERSION`, `CPMFSVERSION`, `CACHE` macros
- `mktemp`, manual `.d` files, `vpath`, suffix rules
- source-tree object and generated-image directories

## Source cleanup

The unrelated Qt template files `main.cpp` and `CPM22_de_DE.ts` are not part of
the historical Makefile build and were removed from the converted package.
The actual program entry point remains `src/Z80CPM.cpp`.
