# Makefile to CMake migration

## Original targets and their CMake equivalents

| Makefile rule | CMake equivalent |
|---|---|
| `all` | default build (`CPM22_images` + `CPM22`) |
| `zout/%.cim` | `cpm22_assemble()` custom commands in `cpm22/CMakeLists.txt` |
| `src/boot.cpp` via `srec_cat` | generated `generated/boot.h` via `BinaryToHeader.cmake` |
| `.cpm22.sys` concatenation | `BuildSystemImage.cmake` with per-component and total-size validation |
| `zexdoc.com`, `zexall.com` | generated drive-A files under the build tree |
| object/dependency rules | native CMake compiler dependency tracking |
| `cpm-2.2` link | target `CPM22`, output name `cpm-2.2` |
| `clean` | `cmake --build <build-dir> --target clean` |

## CP/M assembly subdirectory

The top-level project contains only `add_subdirectory(cpm22)` for the CP/M image
pipeline. `cpm22/CMakeLists.txt` owns assembly of the four image components and the ZEX
programs. It exports only the generated paths needed by the host executable.

The former combined `cpm22.asm`/`cpm22.cim` representation has been replaced by
separate CCP and BDOS inputs and outputs:

```text
cpm22/boot.asm -> zout/boot.cim
cpm22/ccp.asm  -> zout/ccp.cim
cpm22/bdos.asm -> zout/bdos.cim
cpm22/bios.asm -> zout/bios.cim
```

The final image order is:

```text
boot.cim + ccp.cim + bdos.cim + bios.cim -> disks/A/.cpm22.sys
```

## Preserved constants

- boot sector size: 128 bytes
- CCP size: `0x806` = 2054 bytes
- BDOS size: `0xDFA` = 3578 bytes
- CCP + BDOS size: 5632 bytes
- maximum BIOS size: 896 bytes
- maximum system-track image: `2 * 26 * 128` = 6656 bytes
- CP/M system filename: `.cpm22.sys`

The derived limits are calculated with CMake `math(EXPR ...)`; the generated binaries
are checked again at build and test time.

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
