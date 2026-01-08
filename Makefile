CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -mwindows -D_WIN32_WINNT=0x0600 -D__USE_MINGW_ANSI_STDIO=1 -DUNICODE -D_UNICODE $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS = $(shell pkg-config --libs gtk+-3.0) -lshlwapi -lole32 -loleaut32 -luuid -mwindows -municode -static-libgcc -Wl,-e,wWinMain -lcomctl32 -lgdi32 -lws2_32 -lwinmm -limm32 -lsetupapi -lcfgmgr32 -lusp10 -lwininet -loleacc -Wl,--subsystem,windows -lole32 -loleaut32 -luuid

RM = rm -f

all: undawn_reroll

# Compilation du fichier de ressources
undawn_reroll.res: undawn_reroll.rc
	windres $< -O coff -o $@

undawn_reroll: undawn_reroll.c undawn_reroll.res
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) undawn_reroll.exe

.PHONY: all clean release onefile

release: undawn_reroll
	mkdir -p Release/UndawnReroll
	cp undawn_reroll.exe Release/UndawnReroll/
	cp /mingw64/bin/libgtk-3-0.dll Release/UndawnReroll/
	cp /mingw64/bin/libgdk-3-0.dll Release/UndawnReroll/
	cp /mingw64/bin/libglib-2.0-0.dll Release/UndawnReroll/
	cp /mingw64/bin/libgobject-2.0-0.dll Release/UndawnReroll/
	cp /mingw64/bin/libgio-2.0-0.dll Release/UndawnReroll/
	cp /mingw64/bin/libgmodule-2.0-0.dll Release/UndawnReroll/
	cp /mingw64/bin/libgthread-2.0-0.dll Release/UndawnReroll/
	cp /mingw64/bin/libcairo-2.dll Release/UndawnReroll/
	cp /mingw64/bin/libcairo-gobject-2.dll Release/UndawnReroll/
	cp /mingw64/bin/libpango-1.0-0.dll Release/UndawnReroll/
	cp /mingw64/bin/libpangocairo-1.0-0.dll Release/UndawnReroll/
	cp /mingw64/bin/libpangowin32-1.0-0.dll Release/UndawnReroll/
	cp /mingw64/bin/libatk-1.0-0.dll Release/UndawnReroll/
	cp /mingw64/bin/libgdk_pixbuf-2.0-0.dll Release/UndawnReroll/
	cp /mingw64/bin/libintl-8.dll Release/UndawnReroll/
	cp /mingw64/bin/libffi-8.dll Release/UndawnReroll/
	cp /mingw64/bin/libpcre2-8-0.dll Release/UndawnReroll/
	cp /mingw64/bin/zlib1.dll Release/UndawnReroll/
	cp /mingw64/bin/libpng16-16.dll Release/UndawnReroll/
	cp /mingw64/bin/libfreetype-6.dll Release/UndawnReroll/
	cp /mingw64/bin/libfontconfig-1.dll Release/UndawnReroll/
	cp /mingw64/bin/libexpat-1.dll Release/UndawnReroll/
	cp /mingw64/bin/libepoxy-0.dll Release/UndawnReroll/
	cp /mingw64/bin/libbz2-1.dll Release/UndawnReroll/
	cp /mingw64/bin/libharfbuzz-0.dll Release/UndawnReroll/
	cp /mingw64/bin/libgraphite2.dll Release/UndawnReroll/
	cp /mingw64/bin/libiconv-2.dll Release/UndawnReroll/
	cp /mingw64/bin/libfribidi-0.dll Release/UndawnReroll/
	cp /mingw64/bin/libpixman-1-0.dll Release/UndawnReroll/
	cp /mingw64/bin/libstdc++-6.dll Release/UndawnReroll/
	cp /mingw64/bin/libjpeg-8.dll Release/UndawnReroll/
	cp /mingw64/bin/libgcc_s_seh-1.dll Release/UndawnReroll/
	cp /mingw64/bin/libwinpthread-1.dll Release/UndawnReroll/
	/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe -NoProfile -Command "Compress-Archive -Path 'Release\\UndawnReroll\\*' -DestinationPath 'UndawnReroll_Portable.zip' -Force"

onefile: release launcher.res
	windres launcher.rc -O coff -o launcher.res
	$(CC) -o UndawnReroll_OneFile.exe launcher.c launcher.res -mwindows
