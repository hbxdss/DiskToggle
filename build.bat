@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cd /d "%~dp0"
cl /EHsc /O2 /W3 /DUNICODE /D_UNICODE DiskToggle.cpp ^
   user32.lib gdi32.lib setupapi.lib newdev.lib advapi32.lib shell32.lib cfgmgr32.lib ^
   /Fe:DiskToggle.exe ^
   /link /SUBSYSTEM:WINDOWS /MANIFESTUAC:"level='requireAdministrator' uiAccess='false'"
echo.
if exist DiskToggle.exe (
    echo === 编译成功 ===
    dir DiskToggle.exe
) else (
    echo === 编译失败 ===
)
pause
