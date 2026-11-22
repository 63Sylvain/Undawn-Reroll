@echo off
chcp 65001 >nul
title Verification des variables d'environnement GTK (Undawn Reroll)
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "%~dp0Verifier_Env_GTK.ps1" %*
