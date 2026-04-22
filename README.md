## Introduction

This project provides various enhancements for the World of Warcraft `0.5.3` Alpha build.

Enhancements are implemented as DLLs that are loaded at application start and hook or
overwrite application code — no permanent modification of the original binary is required!

## Usage

`launcher.exe WoWClient.exe fix_display.dll fix_timers.dll fix_misc.dll`
(pick any combination of the `.dll` files)

A sample `Config.wtf` is also included, containing a settings preset for maximum visual clarity:

* Change `gxResolution` and `gxRefresh` to values supported by your monitor.
* When using a `16:9` resolution I also suggest setting `fov 110` — but generally `4:3` with
  `fov 90` tends to look best. On widescreen resolutions, character models and objects will
  always appear "stretched" in Alpha's graphics engine.
* If you encounter poor performance, try toggling between `opengl` and `direct3d` (though
  `opengl` usually performs significantly better on modern machines).
* Anti-aliasing is not exposed through the in-game settings, but it can be forced externally
  at the driver level.

## Fixes

### `fix_display.dll`

* Based on ideas from <https://github.com/Mates1500/VanillaMultiMonitorFix>
* Fixes issues related to having multiple connected monitors.
* Unlike the original project, requires no configuration.
* Hooks the Windows API function `EnumDisplayDevicesA` and modifies it to always return the
  largest monitor first, in order to solve the following issue:
  * Alpha WoW only checks the first monitor to verify the configured resolution / refresh
    rate combination.
  * On modern Windows versions, however, `EnumDisplayDevicesA` may return monitors in a
    different order between reboots.

### `fix_timers.dll`

* Based on ideas from
  <https://github.com/akspa0/parp-tools/tree/reconstruction/gillijimproject_refactor/src/MdxViewer/alphaFixes_dll/AlphaFixes>
  and <https://github.com/hannesmann/vanillafixes/>
* Works by sampling a single high-precision timer value at application start and hooking the
  game's timer infrastructure to base its calculations on that value.
* On my machine (NVIDIA 1060) this boosted FPS from ~60 to ~80 in `opengl` mode; the
  improvement under `direct3d` was much less pronounced.
* If your performance is acceptable without this fix, I recommend **not** using it, since it
  interferes with the game engine at a very deep level.
  * ~64 FPS is a good target, since that is reportedly the frequency used for the engine's
    internal calculations (which is why `direct3d` is capped at 64 and `opengl` at 100 — very
    high FPS values cause glitches in combination with Alpha's engine).
  * In early testing I observed some glitchy movement, especially around slopes, but I have
    not been able to reproduce these issues recently.

### `fix_misc.dll`

Simple fixes for miscellaneous annoyances:

* Removes an assertion that crashes the game when tabbing out of fullscreen during a loading
  screen.

## Build / Requirements

* Requires `MSVC` (e.g. from a free Microsoft Visual Studio / Build Tools installation).
* Set up a build command-line environment via `vcvars32.bat` (or use `open_vscode.bat` if you
  prefer to invoke the build tools from VS Code).
* Then build via `make.bat`.
