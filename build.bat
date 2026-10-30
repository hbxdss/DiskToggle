@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64 >nul 2>&1
cd /d "%~dp0"
cl /EHsc /O2 /W3 /utf-8 /DUNICODE /D_UNICODE DiskToggle.cpp user32.lib gdi32.lib setupapi.lib newdev.lib advapi32.lib shell32.lib cfgmgr32.lib /Fe:DiskToggle.exe /link /SUBSYSTEM:WINDOWS
pause
