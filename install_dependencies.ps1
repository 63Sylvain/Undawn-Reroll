# Script d'installation des dépendances pour Undawn Reroll
# Ce script installe MSYS2 et configure GTK3 pour la compilation

Write-Host "Installation des dépendances pour Undawn Reroll..." -ForegroundColor Green

# Vérifier si MSYS2 est déjà installé
$msys2Path = "C:\msys64"
if (Test-Path $msys2Path) {
    Write-Host "MSYS2 est déjà installé dans $msys2Path" -ForegroundColor Yellow
} else {
    Write-Host "Téléchargement et installation de MSYS2..." -ForegroundColor Blue
    
    # Télécharger MSYS2
    $msys2Url = "https://github.com/msys2/msys2-installer/releases/latest/download/msys2-x86_64-latest.exe"
    $installerPath = "$env:TEMP\msys2-installer.exe"
    
    try {
        Invoke-WebRequest -Uri $msys2Url -OutFile $installerPath -UseBasicParsing
        Write-Host "Téléchargement terminé. Lancement de l'installation..." -ForegroundColor Blue
        
        # Installer MSYS2 en mode silencieux
        Start-Process -FilePath $installerPath -ArgumentList "--confirm-command", "--accept-messages", "--root", "C:\msys64" -Wait
        
        # Nettoyer le fichier temporaire
        Remove-Item $installerPath -Force
        
        Write-Host "MSYS2 installé avec succès!" -ForegroundColor Green
    } catch {
        Write-Host "Erreur lors du téléchargement/installation de MSYS2: $_" -ForegroundColor Red
        exit 1
    }
}

# Configurer MSYS2 et installer les paquets nécessaires
Write-Host "Configuration de MSYS2 et installation des paquets..." -ForegroundColor Blue

$msys2Bash = "C:\msys64\usr\bin\bash.exe"
if (Test-Path $msys2Bash) {
    # Mettre à jour MSYS2
    & $msys2Bash -lc "pacman -Syu --noconfirm"
    
    # Installer les outils de développement et GTK3
    & $msys2Bash -lc "pacman -S --noconfirm mingw-w64-x86_64-toolchain"
    & $msys2Bash -lc "pacman -S --noconfirm mingw-w64-x86_64-gtk3"
    & $msys2Bash -lc "pacman -S --noconfirm mingw-w64-x86_64-pkg-config"
    & $msys2Bash -lc "pacman -S --noconfirm make"
    
    Write-Host "Paquets installés avec succès!" -ForegroundColor Green
} else {
    Write-Host "Erreur: MSYS2 bash non trouvé à $msys2Bash" -ForegroundColor Red
    exit 1
}

# Créer un script de compilation
$compileScript = @'
#!/bin/bash
# Script de compilation pour Undawn Reroll

echo "Compilation d\'Undawn Reroll..."

# Ajouter les chemins MSYS2 au PATH
export PATH="/mingw64/bin:$PATH"

# Vérifier que les outils sont disponibles
if ! command -v gcc &> /dev/null; then
    echo "Erreur: GCC non trouvé"
    exit 1
fi

if ! command -v pkg-config &> /dev/null; then
    echo "Erreur: pkg-config non trouvé"
    exit 1
fi

# Compiler le programme
make clean
make

if [ $? -eq 0 ]; then
    echo "Compilation réussie!"
    echo "Copie des DLL nécessaires..."
    
    # Copier les DLL GTK3 nécessaires
    cp /mingw64/bin/libgtk-3-0.dll .
    cp /mingw64/bin/libgdk-3-0.dll .
    cp /mingw64/bin/libglib-2.0-0.dll .
    cp /mingw64/bin/libgobject-2.0-0.dll .
    cp /mingw64/bin/libgio-2.0-0.dll .
    cp /mingw64/bin/libgmodule-2.0-0.dll .
    cp /mingw64/bin/libgthread-2.0-0.dll .
    cp /mingw64/bin/libcairo-2.dll .
    cp /mingw64/bin/libcairo-gobject-2.dll .
    cp /mingw64/bin/libpango-1.0-0.dll .
    cp /mingw64/bin/libpangocairo-1.0-0.dll .
    cp /mingw64/bin/libpangowin32-1.0-0.dll .
    cp /mingw64/bin/libatk-1.0-0.dll .
    cp /mingw64/bin/libgdk_pixbuf-2.0-0.dll .
    cp /mingw64/bin/libintl-8.dll .
    cp /mingw64/bin/libffi-8.dll .
    cp /mingw64/bin/libpcre2-8-0.dll .
    cp /mingw64/bin/zlib1.dll .
    cp /mingw64/bin/libpng16-16.dll .
    cp /mingw64/bin/libfreetype-6.dll .
    cp /mingw64/bin/libfontconfig-1.dll .
    cp /mingw64/bin/libexpat-1.dll .
    cp /mingw64/bin/libbz2-1.dll .
    cp /mingw64/bin/libharfbuzz-0.dll .
    cp /mingw64/bin/libgraphite2.dll .
    cp /mingw64/bin/libiconv-2.dll .
    cp /mingw64/bin/libwinpthread-1.dll .
    cp /mingw64/bin/libgcc_s_seh-1.dll .
    
    echo "DLL copiées. Le programme devrait maintenant fonctionner!"
else
    echo "Erreur de compilation"
    exit 1
fi
'@

$compileScriptPath = "f:\Q\Bureau\Prog\C C++\Undawn_Reroll\compile.sh"
$compileScript | Out-File -FilePath $compileScriptPath -Encoding UTF8

Write-Host "Script de compilation créé: $compileScriptPath" -ForegroundColor Green

# Créer un script batch pour lancer la compilation facilement
$batchScript = @'
@echo off
echo Compilation d\'Undawn Reroll avec MSYS2...
C:\msys64\usr\bin\bash.exe -lc "cd \'%~dp0\' && ./compile.sh"
pause
'@

$batchScriptPath = "f:\Q\Bureau\Prog\C C++\Undawn_Reroll\compile.bat"
$batchScript | Out-File -FilePath $batchScriptPath -Encoding ASCII

Write-Host "Script batch créé: $batchScriptPath" -ForegroundColor Green
Write-Host "" 
Write-Host "Installation terminée!" -ForegroundColor Green
Write-Host "Pour compiler le programme, exécutez: compile.bat" -ForegroundColor Yellow
Write-Host "Ou utilisez MSYS2 directement avec le script compile.sh" -ForegroundColor Yellow