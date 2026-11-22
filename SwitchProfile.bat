@echo off
setlocal

set "BASE=%LOCALAPPDATA%\Toaa\INTL\28014\Toaa-Win64-Shipping"
set "PROFILE=%~1"
if "%PROFILE%"=="" set "PROFILE=Th0r"

if not exist "%BASE%\intl_auth.txt.%PROFILE%" (
    echo [ERREUR] Profil introuvable : "%BASE%\intl_auth.txt.%PROFILE%"
    exit /b 1
)

copy /y "%BASE%\intl_auth.txt.%PROFILE%" "%BASE%\intl_auth.txt" >nul
if errorlevel 1 (
    echo [ERREUR] Echec de la copie du fichier d'authentification.
    exit /b 1
)

start "" "steam://rungameid/1881700"
