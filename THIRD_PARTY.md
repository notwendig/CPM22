# Third-party material

CPM22 is a historical/retro-computing project and its current tree contains material
that did not originate in the CPM22 host-emulator source.

## Important publication note

**Do not assume that the root `LICENSE` applies to every file in this repository.**
Before making the repository public, verify that you have the right to redistribute
each historical binary/manual/source artifact, or remove that artifact from the public
repository.

Areas requiring particular review include:

- `cpm/` — reconstructed CP/M 2.2 source and related assembler material.
- `disks/A/` — historical CP/M utilities and binaries.
- `disks/D/` — Turbo Pascal program files, messages, overlays, examples, and tools.
- `doc/` — historical manuals and PDF documentation.

## Build dependencies

The build also depends on external projects that are not relicensed by CPM22:

- **Z80** — provided through the external CMake target `Z80::Z80`; see that project's
  own repository and license.
- **zmac** — Z-80 macro cross-assembler; see the upstream `gp48k/zmac` repository and
  its own notices.

This file is a provenance warning, not a legal determination of any third-party work's
copyright or license status.
