# SP400 emulator

This project emulates the Sega SP-400 plotter. The original Makefile targets
Linux; the CMake and VS Code files add a 64-bit Windows build using Microsoft
Visual C++ and SDL2.

## Windows prerequisites

- Visual Studio 2022 or Build Tools 2022 with **Desktop development with C++**.
- VS Code with the recommended Microsoft C/C++ extension.
- `sp400_6805.bin` in the repository root.

The firmware is a local runtime file and is intentionally ignored by Git.

## Build in VS Code

1. Open this folder in VS Code.
2. Select **Terminal > Run Build Task**.
3. Choose **Windows: Build Debug** (it is the default build task).

The first build downloads SDL's official 2.32.2 Visual C development archive.
The setup script verifies its SHA-256 hash and uses the archive's headers, x64
`SDL2.lib`, and x64 `SDL2.dll`. A repository-root SDL DLL is not required.

The output is written to `build/windows/Debug/`. The build copies both
`SDL2.dll` and `sp400_6805.bin` beside `sp400.exe`.

To build from a PowerShell terminal instead:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1 -Configuration Debug
```

## Run and debug

Press **F5**, select **SP400 emulator (MSVC Debug)**, and enter the Windows COM
port connected to the plotter input, for example `COM3`.

From a terminal, run:

```powershell
.\build\windows\Debug\sp400.exe COM3
```

The Windows serial implementation opens the port at 4800 baud, 8 data bits, no
parity, and one stop bit (4800 8N1). It also drives RTS from the emulator's busy
state. Port names above `COM9` are supported.

## Linux

Install the SDL2 development package for your distribution, initialize the
`m68emu` submodule, and use the existing Makefile:

```sh
git submodule update --init
make
./sp400 /dev/ttyUSB0
```
