## Introduction

This project provides various enhancements for the World of Warcraft `0.5.3` Alpha build.

Enhancements are implemented as dlls which are loaded on application start and hook/overwrite application code -
no permanent modification of the original binary required!

## Usage

`launcher.exe WoWClient.exe fix_display.dll fix_misc.dll` (pick any combination of `dll` files)

## Fixes

### `fix_display.dll`

* Based on ideas from <https://github.com/Mates1500/VanillaMultiMonitorFix>
* Fixes issues related to multiple connected monitors
* Unlike the original project, does not require configuration
* It will hook the Windows API function `EnumDisplayDevicesA`, and modify it to always the return the biggest monitor as the first one to solve the following issue:
  * On modern Windows versions, `EnumDisplayDevicesA` can return monitors in different order after reboots
  * Alpha WoW only checks the first monitor to verify resolutions

### `fix_timers.dll`

* Based on ideas from <https://github.com/akspa0/parp-tools/tree/reconstruction/gillijimproject_refactor/src/MdxViewer/alphaFixes_dll/AlphaFixes>
  * Also from <https://github.com/hannesmann/vanillafixes/>
* Works by measuring a single precise timer value on application start and hooking the games timer infrastructure to base its calculcations on it
* However, the performance improvement of this fix seems negligible and it might lead to timing problems, so use at your own risc

### `fix_misc.dll`

* Simple fixes for misc. "annoyances":
  * Remove assertion which crashes the game when tabbing out while on the loading screen

## Build / Requirements

* Requires `MSVC` (e.g. as part of free Microsoft Visual Studio installations)
* Set up a build command line via `vcvars32.bat` (can also use `open_vscode.bat` if you want to invoke build tools via vscode)
* Build via `make.bat`
