@echo off
echo Building bob16...

REM Collect all .c files recursively
setlocal enabledelayedexpansion
set SOURCES=

for /r src %%f in (*.c) do (
    set SOURCES=!SOURCES! "%%f"
)


REM Create bin folder if missing
if not exist bin mkdir bin

REM Compile everything
gcc %SOURCES% ^
    -I include ^
    -o bin\bob16.exe

if %errorlevel%==0 (
    echo Build successful, running...
    call run.bat
) else (
    echo Build failed.
)
