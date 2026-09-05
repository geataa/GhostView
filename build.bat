@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo   GhostView - MSVC Derleyici Scripti
echo ========================================================

if not exist bin mkdir bin

:: Find vcvars64.bat
set "VCVARS="
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
)

if "%VCVARS%"=="" (
    echo HATA: Visual Studio C++ araclari bulunamadi!
    pause
    exit /b 1
)

echo Visual Studio ortami baslatiliyor: %VCVARS%
call "%VCVARS%"

echo.
echo Kaynaklar derleniyor (Icon ve Manifest)...
rc /nologo /fo bin\GhostView.res GhostView.rc

echo.
echo Derleme basliyor...
cl /nologo /W3 /O2 /MT /EHsc /std:c++17 /utf-8 ^
    src\main.cpp ^
    src\ViewerApp.cpp ^
    src\ImageLoader.cpp ^
    src\FolderNavigator.cpp ^
    src\HudRenderer.cpp ^
    src\ThumbnailBar.cpp ^
    src\Localization.cpp ^
    bin\GhostView.res ^
    /Fe:bin\GhostView.exe ^
    /link /SUBSYSTEM:WINDOWS ^
    d3d11.lib dxgi.lib d2d1.lib dwrite.lib dcomp.lib windowscodecs.lib propsys.lib shlwapi.lib comdlg32.lib shell32.lib user32.lib gdi32.lib ole32.lib advapi32.lib

if %ERRORLEVEL% EQU 0 (
    copy /Y bin\GhostView.exe GhostView.exe >nul
    echo.
    echo ========================================================
    echo   DERLEME BASARILI!
    echo   Cikti: GhostView.exe
    echo ========================================================
) else (
    echo.
    echo DERLEME HATASI OLUSTU! Kod: %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
