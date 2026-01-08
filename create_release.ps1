$ErrorActionPreference = "Stop"

Write-Host "=== Création de la version de déploiement Undawn Reroll ===" -ForegroundColor Cyan

# 1. Lancer la compilation via MSYS2 directement (pour éviter le "pause" du .bat)
Write-Host "1. Lancement de la compilation..."
$currentDir = $PWD.Path -replace "\\", "/"
$bashCmd = "cd '$currentDir' && ./compile.sh"
$proc = Start-Process "C:\msys64\usr\bin\bash.exe" -ArgumentList "-lc ""$bashCmd""" -Wait -NoNewWindow -PassThru

if ($proc.ExitCode -ne 0) {
    Write-Error "La compilation a échoué."
}

# 2. Préparer le dossier de sortie
$releaseDir = "Release"
$appDir = "$releaseDir\UndawnReroll"
$zipFile = "UndawnReroll_Portable.zip"

if (Test-Path $releaseDir) {
    Remove-Item $releaseDir -Recurse -Force
}
if (Test-Path $zipFile) {
    Remove-Item $zipFile -Force
}

New-Item -ItemType Directory -Path $appDir | Out-Null

# 3. Copier les fichiers
Write-Host "2. Copie des fichiers..."

# Copier l'exécutable
Copy-Item "undawn_reroll.exe" -Destination $appDir

# Copier toutes les DLLs présentes (générées par compile.sh)
$dlls = Get-ChildItem -Filter "*.dll"
foreach ($dll in $dlls) {
    Copy-Item $dll.FullName -Destination $appDir
}

# 4. Créer l'archive ZIP
Write-Host "3. Création de l'archive ZIP..."
Compress-Archive -Path "$appDir\*" -DestinationPath $zipFile

Write-Host "=== SUCCÈS ! ===" -ForegroundColor Green
Write-Host "Le fichier prêt à partager est ici : $PWD\$zipFile"
Write-Host "Tu peux envoyer ce fichier ZIP à tes amis. Ils devront juste le décompresser et lancer undawn_reroll.exe."
