@echo off
echo Creation de l'executable unique...

:: 1. Créer le ZIP (Release)
powershell -ExecutionPolicy Bypass -File create_release.ps1
if %ERRORLEVEL% NEQ 0 (
    echo Erreur lors de la creation de la release.
    pause
    exit /b %ERRORLEVEL%
)

:: 2. Compiler le launcher
echo Compilation du launcher...
set "CURRENT_DIR=%~dp0"
set "CURRENT_DIR=%CURRENT_DIR:\=/%"
C:\msys64\usr\bin\bash.exe -lc "cd '%CURRENT_DIR%' && ./build_launcher.sh"

if %ERRORLEVEL% NEQ 0 (
    echo Erreur lors de la compilation du launcher.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================================
echo SUCCES !
echo Votre fichier unique est pret : UndawnReroll_OneFile.exe
echo ========================================================
pause
