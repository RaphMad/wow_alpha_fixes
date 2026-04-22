## Introduction

This project provides various enhancements for the World of Warcraft `0.5.3` Alpha build.

Enhancements are implemented as dlls which are loaded on application start and hook/overwrite application code -
no permanent modification of the original binary required!

## Usage

`launcher.exe WoWClient.exe fix_display.dll fix_timers.dll fix_misc.dll` (pick any combination of `dll` files)

Also included is a sample `Config.wtf` which contains a settings preset for maximum visual clarity:
  * You want to change `gxResolution` and `gxRefresh` to a value supported by your monitor
  * When using a `16:9` resolution I suggest also setting `fov 110` - but generally `4:3` and `fov 90` is what tends to look best - on widescreen resolutions character models and objects will always be "stretched" in alphas graphics engine
  * When encountering bad performance try toggling between `opengl` and `direct3d` (but usually `opengl` has way better performance on modern machines)
  * Anti-aliasing is not supported via game settings, but can be enforced externally on the driver level

## Fixes

### `fix_display.dll`

* Based on ideas from <https://github.com/Mates1500/VanillaMultiMonitorFix>
* Fixes issues related to multiple connected monitors
* Unlike the original project, does not require configuration
* It will hook the Windows API function `EnumDisplayDevicesA`, and modify it to always the return the biggest monitor as the first one to solve the following issue:
  * Alpha WoW only checks the first monitor to verify the configured resolution / refresh rate combination
  * But on modern Windows versions, `EnumDisplayDevicesA` can return monitors in different order between reboots

### `fix_timers.dll`

* Based on ideas from <https://github.com/akspa0/parp-tools/tree/reconstruction/gillijimproject_refactor/src/MdxViewer/alphaFixes_dll/AlphaFixes>
  * Also from <https://github.com/hannesmann/vanillafixes/>
* Works by measuring a single precise timer value on application start and hooking the games timer infrastructure to base its calculcations on it
* On my machine (Nvidia 1060 card) this boosted `fps` from `~60` to `~80` (in `opengl` mode, with `direct3d` the improvement was way less pronounced)
* If your performance is ok without this fix, I recommend __not__ using it since it interferes with the game engine on a very deep level
  * `64fps` is a good target, because supposedly that is the frequency used for internal calculations (thats why `direct3d` is capped at `64` and `opengl` is capped at `100` - very high `fps` values would cause glitches in combination with alphas engine)
  * In early testing, I observed some glitchy movement, especially around slopes (but those problems no longer occur now?)

### `fix_misc.dll`

* Simple fixes for misc. "annoyances":
  * Remove assertion which crashes the game when tabbing out of fullscreen while in a loading screen

## Build / Requirements

* Requires `MSVC` (e.g. as part of free Microsoft Visual Studio installations)
* Set up a build command line via `vcvars32.bat` (can also use `open_vscode.bat` if you want to invoke build tools via vscode)
* Build via `make.bat`
