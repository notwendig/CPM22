# Building CPM22

## Recommended layout

```text
zilogz80-code/
├── Z80/
└── CPM22/
```

If Z80 is elsewhere, pass its source tree explicitly:

```sh
cmake -S . -B build/custom -G Ninja \
  -DZ80_SOURCE_DIR=/path/to/Z80 \
  -DCMAKE_BUILD_TYPE=Debug
```

An installed Z80 CMake package providing `Z80::Z80` is preferred automatically when
available.

## zmac

`zmac` must be on `PATH` (or in `$HOME/bin`). A typical source build is:

```sh
git clone https://github.com/gp48k/zmac.git
make -C zmac/src -j1
install -m 0755 zmac/src/zmac "$HOME/bin/zmac"
```

The upstream Makefile uses `bison -y`; install a C compiler, `make`, and `bison` before
building it.

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
