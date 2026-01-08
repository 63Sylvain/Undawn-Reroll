#!/bin/bash
# Script de compilation pour Undawn Reroll

echo "Compilation d\'Undawn Reroll..."

# Ajouter les chemins MSYS2 au PATH
export PATH="/mingw64/bin:$PATH"

# VÃ©rifier que les outils sont disponibles
if ! command -v gcc &> /dev/null; then
    echo "Erreur: GCC non trouvÃ©"
    exit 1
fi

if ! command -v pkg-config &> /dev/null; then
    echo "Erreur: pkg-config non trouvÃ©"
    exit 1
fi

# Compiler le programme
make clean
make

if [ $? -eq 0 ]; then
    echo "Compilation rÃ©ussie!"
    echo "Copie des DLL nÃ©cessaires..."
    
    # Copier les DLL GTK3 nÃ©cessaires
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
    cp /mingw64/bin/libepoxy-0.dll .
    cp /mingw64/bin/libbz2-1.dll .
    cp /mingw64/bin/libharfbuzz-0.dll .
    cp /mingw64/bin/libgraphite2.dll .
    cp /mingw64/bin/libiconv-2.dll .
    cp /mingw64/bin/libfribidi-0.dll .
    cp /mingw64/bin/libpixman-1-0.dll .
    cp /mingw64/bin/libstdc++-6.dll .
    cp /mingw64/bin/libjpeg-8.dll .
    cp /mingw64/bin/libgcc_s_seh-1.dll .
    cp /mingw64/bin/libwinpthread-1.dll .
    
    echo "DLL copiÃ©es. Le programme devrait maintenant fonctionner!"
else
    echo "Erreur de compilation"
    exit 1
fi
