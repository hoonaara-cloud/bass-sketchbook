@echo off
setlocal
echo === Groove Sketchbook - Windows VST3 build ===
echo.

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake not found.
    echo Install it from https://cmake.org/download/ ^(check "Add to PATH"^) and retry.
    exit /b 1
)

where git >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Git not found.
    echo Install it from https://git-scm.com/download/win and retry.
    exit /b 1
)

echo [1/2] Configuring ^(first run downloads JUCE ~200 MB, be patient^)...
cmake -S . -B build -A x64
if errorlevel 1 (
    echo [ERROR] Configure failed. Do you have Visual Studio 2022 or newer with "Desktop development with C++"?
    exit /b 1
)

echo.
echo [2/2] Building Release...
cmake --build build --config Release -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo [ERROR] Build failed. See errors above.
    exit /b 1
)

echo.
echo === DONE ===
echo VST3:       build\GrooveSketchbook_artefacts\Release\VST3\Groove Sketchbook.vst3
echo Standalone: build\GrooveSketchbook_artefacts\Release\Standalone\Groove Sketchbook.exe
echo.
echo Next: right-click install-windows.bat -^> "Run as administrator" to install the VST3.
echo Or just double-click the Standalone .exe to play without a DAW.
endlocal
