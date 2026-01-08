@echo off
echo Compilation d'Undawn Reroll avec MSYS2...
set "CURRENT_DIR=%~dp0"
set "CURRENT_DIR=%CURRENT_DIR:\=/%"
set "CURRENT_DIR=%CURRENT_DIR: =\ %"
C:\msys64\usr\bin\bash.exe -lc "cd '%CURRENT_DIR%' && ./compile.sh"
pause
