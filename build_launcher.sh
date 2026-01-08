#!/bin/bash
export PATH="/mingw64/bin:$PATH"
echo "Building Launcher..."
windres launcher.rc -O coff -o launcher.res
gcc -o UndawnReroll_OneFile.exe launcher.c launcher.res -mwindows
echo "Done."
