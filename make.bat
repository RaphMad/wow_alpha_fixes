@echo off

if not "%1" == "clean" (
    cd _out
    cl ..\src\launcher.c
    cl ..\src\fix_display.c ..\libs\minhook_134\lib\libMinHook.x86.lib User32.lib /LD
    cl ..\src\fix_timers.c ..\libs\minhook_134\lib\libMinHook.x86.lib ..\libs\tsc\tsc.c Winmm.lib /LD
    cl ..\src\fix_misc.c ..\src\helpers\helpers.c /LD
    cl ..\src\world_enter_exec.c ..\libs\minhook_134\lib\libMinHook.x86.lib /LD
    cd ..
) else (
    del /Q /S _out\*
    type nul > _out\.gitkeep
)
