@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
set "BUILD_DIR=%ROOT%\build"
set "FQBN=arduino:avr:mega:cpu=atmega2560"
set "AVR_GCC_BIN="

for /f "delims=" %%D in ('dir /b /ad "%LOCALAPPDATA%\Arduino15\packages\arduino\tools\avr-gcc" 2^>nul') do set "AVR_GCC_BIN=%LOCALAPPDATA%\Arduino15\packages\arduino\tools\avr-gcc\%%D\bin"

if not defined AVR_GCC_BIN (
    echo AVR-GCC from Arduino CLI was not found.
    exit /b 1
)

if not exist "%AVR_GCC_BIN%\avr-objdump.exe" (
    echo avr-objdump.exe was not found in "%AVR_GCC_BIN%".
    exit /b 1
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

arduino-cli compile --fqbn %FQBN% --build-path "%BUILD_DIR%" "%ROOT%"
if errorlevel 1 exit /b %errorlevel%

"%AVR_GCC_BIN%\avr-objdump.exe" -h -S -C -z "%BUILD_DIR%\minibook2560.ino.elf" > "%BUILD_DIR%\minibook2560.lss"
if errorlevel 1 exit /b %errorlevel%

echo.
echo Assembler listing: "%BUILD_DIR%\minibook2560.lss"
