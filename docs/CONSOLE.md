# Console and PuTTY configuration

CPM22 exposes its console as a raw TCP service on port 1234. The historical default
launcher uses a saved PuTTY session named `CPM`.

## Tested PuTTY settings

```text
Host: localhost
Port: 1234
Connection type: Raw
Local echo: Force off
Local line editing: Force off
Backspace key: Control-H
```

In a PuTTY session file these relevant values are:

```text
Protocol=raw
LocalEcho=1
LocalEdit=1
BackspaceIsDelete=0
```

`LocalEcho=2` and `LocalEdit=2` mean Auto, which is unsuitable for this raw CP/M
console because menu programs such as Turbo Pascal expect individual key presses.

## Current keyboard limitation

Modern terminal cursor and Delete keys normally send multi-byte ANSI escape sequences
(for example Right Arrow commonly sends `ESC [ C`). CP/M and historical Turbo Pascal
keyboard command tables do not automatically interpret all modern terminal sequences.

Backspace should therefore use Control-H. Cursor/Delete handling should be configured
inside the historical application (for example with Turbo Pascal's installation tool)
or implemented by a deliberate terminal-translation layer; do not globally map
Control-S without considering CP/M console flow-control semantics.
