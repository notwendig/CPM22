# Changelog

All notable CPM22 project-maintenance changes are documented here.

## [Unreleased]

### Changed

- Moved the complete CP/M image pipeline into the `cpm22/` CMake subdirectory.
- Assemble `ccp.asm` and `bdos.asm` independently as `ccp.cim` and `bdos.cim`.
- Build `.cpm22.sys` explicitly as `boot + CCP + BDOS + BIOS`.
- Calculate the component limits and the 6656-byte system-track limit with CMake
  `set()` and `math(EXPR ...)`.
- Updated README, build, migration, and editor documentation for the current source,
  output, submodule, and disk-directory layout.

### Fixed

- Removed the stale build reference to the nonexistent `cpm22/cpm22.asm`.
- Replaced the duplicate `CPM_FILE` argument with separate `CCP_FILE` and `BDOS_FILE`
  inputs throughout image creation and artifact verification.
- Verify that the generated image size exactly equals boot + CCP + BDOS + BIOS and
  remains within two 26-sector tracks.

## [1.0.0] - 2026-08-11

### Added

- CMake-only build replacing the historical handwritten Makefile.
- Debug and Release CMake presets.
- Reproducible `zmac` assembly of CP/M, CBIOS, boot loader, ZEXDOC, and ZEXALL.
- Build-time validation of boot/CCP-BDOS/system-image sizes.
- Generated `boot.h` in the build tree.
- CTest artifact verification.
- GitHub Actions Debug/Release CI.
- GitHub issue templates, pull-request template, CODEOWNERS, and Dependabot config.
- Contributor, security, console, build, and third-party documentation.

### Fixed

- First-boot out-of-bounds drive selection that could cause a segmentation fault.
- Several host-memory transfer boundary/copy defects.
- Deprecated host-directory traversal and copy warnings.
- Console CR/LF handling that caused CP/M `DIR` to stop after the first entry.
- Thread-unsafe console input buffering.
- `--rcfile` option fallthrough.
- Warning-clean CPM22 compilation under strict GCC diagnostics.

### Changed

- The external Z80 target is consumed as `Z80::Z80` instead of hard-coded library paths.
- CMake preset schema now matches the declared CMake 3.21 minimum.
