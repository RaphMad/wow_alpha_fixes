@echo off

if not "%1" == "clean" (
    cd _out
    cl ..\launcher.c
    cl ..\fix_display.c ..\minhook_134\lib\libMinHook.x86.lib User32.lib /LD
    cl ..\fix_timers.c ..\minhook_134\lib\libMinHook.x86.lib User32.lib Winmm.lib /LD
    cd ..
) else (
    del /Q /S _out\*
    type nul > _out\.gitkeep
)
