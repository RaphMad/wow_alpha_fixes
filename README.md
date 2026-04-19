## Introduction

This project provides various enhancements for the World of Warcraft `0.5.3` Alpha build.

Enhancements are implemented as dlls which are loaded on application start and hook/overwrite application code -
no permanent modification of the original binary necessary!

## Usage

`launcher.exe WoWClient.exe fix_display.dll fix_timers.dll`

## Fixes

### `fix_display.dll`

* Based on ideas from <https://github.com/Mates1500/VanillaMultiMonitorFix>
* Fixes issues related to multiple connected monitors
* Unlike the original project, does not require configuration
* It will hook the Windows API function `EnumDisplayDevicesA`, and modify it to always the return the biggest monitor as the first one to solve the following issue:
  * On modern Windows versions, `EnumDisplayDevicesA` can return monitors in different order after reboots
  * Alpha WoW only checks the first monitor to verify resolutions

### `fix_timers.dll`

* Based on idead from <https://github.com/akspa0/parp-tools/tree/reconstruction/gillijimproject_refactor/src/MdxViewer/alphaFixes_dll/AlphaFixes>
* Works by redirecting code paths related to timer handling to variants utilizing more modern and efficient approaches
