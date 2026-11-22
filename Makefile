CC = gcc
WINDRES = windres
CFLAGS = -O2 -Wall -Wextra -std=c99 -finput-charset=UTF-8 -mwindows -D_WIN32_WINNT=0x0600 -D__USE_MINGW_ANSI_STDIO=1 -DUNICODE -D_UNICODE
LDFLAGS = -mwindows -municode -static-libgcc -s -lcomctl32 -lshell32 -lole32 -ladvapi32 -lgdi32 -luser32
LAUNCHER_CFLAGS = -O2 -Wall -Wextra -std=c99 -finput-charset=UTF-8 -mwindows -DUNICODE -D_UNICODE
LAUNCHER_LDFLAGS = -mwindows -municode -static-libgcc -s

RM = rm -f
POWERSHELL = /c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe

.PHONY: all clean release onefile

all: undawn_reroll.exe

undawn_reroll.res: undawn_reroll.rc undawn_reroll.manifest unnamed.ico
	$(WINDRES) -c 65001 $< -O coff -o $@

undawn_reroll.exe: undawn_reroll.c undawn_reroll.res
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

release: undawn_reroll.exe
	rm -rf Release/UndawnReroll
	mkdir -p Release/UndawnReroll
	cp undawn_reroll.exe Release/UndawnReroll/
	$(POWERSHELL) -NoProfile -Command "Compress-Archive -Path 'Release\\UndawnReroll\\*' -DestinationPath 'UndawnReroll_Portable.zip' -Force"

launcher.res: launcher.rc UndawnReroll_Portable.zip unnamed.ico
	$(WINDRES) -c 65001 launcher.rc -O coff -o $@

onefile: release launcher.res
	$(CC) $(LAUNCHER_CFLAGS) -o UndawnReroll_OneFile.exe launcher.c launcher.res $(LAUNCHER_LDFLAGS)

clean:
	$(RM) undawn_reroll.exe undawn_reroll.res launcher.res UndawnReroll_OneFile.exe UndawnReroll_Portable.zip
