PuTTY session for the CPM22 TCP console.

Recommended Linux installation:

  mkdir -p ~/.putty/sessions
  cp CPM ~/.putty/sessions/CPM

Important settings in this session:

  HostName=localhost
  PortNumber=1234
  Protocol=raw
  LocalEcho=1          (Force off)
  LocalEdit=1          (Force off)
  BackspaceIsDelete=0  (Backspace sends Ctrl-H)

The emulator starts PuTTY with "-load CPM" by default.
Modern cursor/Delete keys can still send ANSI escape sequences which historical
CP/M applications may not interpret unless configured in the application.
