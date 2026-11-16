# CPM22

[English](README.md)

CP/M-2.2-Umgebung für einen Z80-Emulator auf einem POSIX-Host. Das Projekt kombiniert einen C++-Emulator, eine CBIOS-Brücke zum Host-Dateisystem, eine TCP-Konsole und die reproduzierbare Assemblierung des Systemabbilds mit `zmac`.

## Bauen und prüfen

Vorausgesetzt werden CMake ab 3.21, ein C++20-Compiler, Ninja, `zmac` und das initialisierte `Z80/`-Submodul. Die [englische Anleitung](README.md) enthält die vollständigen CMake-Befehle, Presets und Hinweise zur Konsole. Erzeugte Abbilder und Laufzeitdateien gehören ins Build-Verzeichnis; die CTest-Prüfungen kontrollieren die wichtigen Bauartefakte.

## Grenzen und Rechte

Die Host-Dateisystem-Brücke sollte nur mit bewusst freigegebenen Verzeichnissen verwendet werden. Vor der Weitergabe von CP/M-Systemdateien und Testprogrammen die Hinweise in [THIRD_PARTY.md](THIRD_PARTY.md) und [LICENSE](LICENSE) beachten.
