# Contributing

Contributions that improve correctness, portability, build reproducibility, terminal
compatibility, or documentation are welcome.

## Development workflow

1. Create a focused branch.
2. Configure and build Debug.
3. Run CTest.
4. Keep CPM22-owned code warning-free.
5. Keep generated files out of the source tree.
6. Submit a pull request describing behavior before and after the change.

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

For CI-equivalent warning policy:

```sh
cmake -S . -B build/strict -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCPM22_WARNINGS_AS_ERRORS=ON
cmake --build build/strict
ctest --test-dir build/strict --output-on-failure
```

## Scope

Please keep unrelated formatting changes separate from functional changes. Do not
commit generated `build/`, `zout/`, `.cim`, `.lst`, or generated header files.

## Third-party files

Do not add or replace historical software, manuals, ROMs, binaries, or other
third-party artifacts unless their redistribution status has been checked and
recorded in `THIRD_PARTY.md`.
