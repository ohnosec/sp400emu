# Sega SP-400 plotter emulator

This project emulates the Sega SP-400 plotter on Linux and Windows. It accepts
native SP-400 input from a serial port, a file, or a local TCP connection.

## Build prerequisites

### Common

- Git, including submodule support
- Python 3, used to generate build-time tables and embedded cursor assets

Initialize the `m68emu` submodule after cloning:

```sh
git submodule update --init
```

### Linux

Install a C++ toolchain, GNU Make, Python 3, and the SDL2 development package.
On Ubuntu:

```sh
sudo apt install build-essential python3 libsdl2-dev
```

Install `gdb` as well if you want to debug from VS Code.

### Windows

- Visual Studio 2022 or Build Tools 2022 with **Desktop development with C++**
- VS Code with the recommended Microsoft C/C++ extension, if desired
- Python 3. Install it from python.org and enable. **Add python.exe to PATH**
during installation.

The Windows setup script downloads SDL's official 2.32.2 Visual C development
archive on the first build. It verifies the archive's SHA-256 hash before using
its headers, x64 import library, and DLL.

## Build

| Platform | Build command | Executable |
| --- | --- | --- |
| Linux | `make` | `build/linux/sp400` |
| Windows Debug | `scripts\build-windows.ps1 -Configuration Debug` | `build\windows\Debug\sp400.exe` |
| Windows Release | `scripts\build-windows.ps1 -Configuration Release` | `build\windows\Release\sp400.exe` |

### Linux

From the project root:

```sh
make
```

The executable, objects, and generated headers are written under
`build/linux/`. Run `make clean` to remove only the Linux build output.

### Windows

From PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1 -Configuration Debug
```

The build places the executable in `build/windows/Debug/` and copies
`SDL2.dll` and `sp400_6805.bin` beside it.

## VS Code

The checked-in VS Code configurations support both platforms:

| Platform | Build task | Debugger | Launch configurations |
| --- | --- | --- | --- |
| Linux | **Linux: Build** | GDB | Serial, file, and TCP |
| Windows | **Windows: Build Debug** | MSVC | Serial, file, and TCP |

Use **Terminal > Run Build Task** to select a build. Press **F5** and select a
launch configuration under **Run and Debug**. Serial launches prompt for
`/dev/ttyUSB0` on Linux by default and `COM3` on Windows ; file and TCP launches
prompt for their corresponding input.

## Run

Run these commands from the project root so the emulator can find
`sp400_6805.bin`.

### Serial input

```sh
# Linux
./build/linux/sp400 /dev/ttyUSB0
```

```powershell
# Windows
.\build\windows\Debug\sp400.exe COM3
```

The emulator configures the serial port for 4800 baud, 8 data bits, no parity,
and one stop bit (4800 8N1). RTS reflects the emulator's ready/busy state.
Windows port names above `COM9` are supported.

### File input

```sh
# Linux
./build/linux/sp400 --file commands.txt
```

```powershell
# Windows
.\build\windows\Debug\sp400.exe --file ".\commands.txt"
```

File input is read in binary mode and every byte is passed to the emulator
unchanged. Playback is paced like a 4800-baud 8N1 serial connection and observes
the same ready/busy signal as serial input. Reaching the end of the file stops
input but leaves the SDL window open so the completed plot remains visible.
Quote paths that contain spaces.

### TCP input

```sh
# Linux
./build/linux/sp400 --tcp 4040
```

```powershell
# Windows
.\build\windows\Debug\sp400.exe --tcp 4040
```

The listener binds only to `127.0.0.1`, accepts one client at a time, and treats
all input as an opaque stream of native SP-400 bytes. TCP writes and packets do
not define command boundaries. Received bytes are delivered in order, paced
like a 4800-baud 8N1 serial link, and gated by the emulated ready signal.

A client may send immediately after the listener appears; the emulator buffers
the stream until the ROM has completed startup and entered its input parser.
Disconnecting leaves the SDL window open and returns the listener to its accept
loop. TCP mode does not send the emulated BUSY state back, so it verifies
translation and plotting rather than UART/BUSY transport behavior.

## Manual controls

- Hold **FEED**, or hold the `F` key, for Line Feed.
- Click **COLOR**, or press the `C` key, for Color Select.

## Paper navigation

When the plotter is idle, use the mouse wheel or the Up and Down arrow keys to
move through the stored paper in small steps. Page Up and Page Down move by
nearly a full page. This changes only the displayed viewport; it does not move
the emulated print head or send an input command.

The pointer changes to an open hand over the paper while navigation is
available, then to a closed hand while dragging. Hold the left mouse button and
drag the paper directly to move through it.

Manual scrolling is ignored while the firmware reports that it is busy. When
plotting resumes, the viewport automatically returns to the current print-head
position.
