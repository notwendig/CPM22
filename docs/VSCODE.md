# Visual Studio Code

CPM22 includes a checked-in VS Code configuration for the CMake preset based build.

## Requirements

- Visual Studio Code or VSCodium
- CMake Tools (`ms-vscode.cmake-tools`)
- C/C++ (`ms-vscode.cpptools`)
- CMake, Ninja, GCC, GDB and zmac available on the host
- the initialized `Z80/` submodule, or a configured `Z80_SOURCE_DIR`

## Open the project

```bash
cd "$HOME/Projects/zilogz80-code/CPM22"
code .
```

For VSCodium use `codium .` instead. Accept the workspace's recommended extensions if
the editor offers them.

## Debug build

Press `Ctrl+Shift+B`. The default build task runs, in sequence:

```bash
cmake --preset debug
cmake --build --preset debug --parallel
```

This also builds the assembly artifacts in `build/Desktop_Debug/zout/` and creates
`build/Desktop_Debug/disks/A/.cpm22.sys` from boot, CCP, BDOS and BIOS.

The executable is written to:

```text
build/Desktop_Debug/cpm-2.2
```

## Debug with GDB

Set a breakpoint in a C++ source file and press `F5`. Select:

```text
CPM22: Debug (GDB)
```

VS Code first configures and builds the Debug preset, then launches:

```text
build/Desktop_Debug/cpm-2.2 --diskpath build/Desktop_Debug
```

with `build/Desktop_Debug` as the working directory.

For an immediate stop at program entry select:

```text
CPM22: Debug at main (GDB)
```

## Tests

Run **Tasks: Run Task** and select:

```text
CPM22: Test Debug
```

This builds the Debug configuration first and then runs:

```bash
ctest --preset debug
```

The artifact test checks the individual boot, CCP and BDOS sizes, the complete
boot+CCP+BDOS+BIOS image size, both ZEX programs, and the host executable.

## Run without debugging

Run **Tasks: Run Task** and select:

```text
CPM22: Run Debug
```

The CP/M console remains available on TCP port 1234 as implemented by CPM22. Attach the
configured PuTTY RAW session separately.
