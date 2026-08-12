# Visual Studio Code

CPM22 includes a checked-in VS Code configuration for the CMake preset based build.

## Requirements

- Visual Studio Code
- CMake Tools (`ms-vscode.cmake-tools`)
- C/C++ (`ms-vscode.cpptools`)
- CMake, Ninja, GCC, GDB and zmac available on the host
- the sibling Z80 project available as expected by `CMakeLists.txt`

## Open the project

```bash
cd /home/juergen/Projects/zilogz80-code/CPM22
code .
```

Accept the workspace's recommended extensions if VS Code offers them.

## Debug build

Press `Ctrl+Shift+B`. The default build task runs, in sequence:

```bash
cmake --preset debug
cmake --build --preset debug --parallel
```

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

## Run without debugging

Run **Tasks: Run Task** and select:

```text
CPM22: Run Debug
```

The CP/M console remains available on TCP port 1234 as implemented by CPM22. Attach the configured PuTTY RAW session separately.
