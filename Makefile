CC = gcc
WINDRES = windres
CFLAGS = -O2 -Wall -Wextra -std=c99 -finput-charset=UTF-8 -mwindows -D_WIN32_WINNT=0x0600 -D__USE_MINGW_ANSI_STDIO=1 -DUNICODE -D_UNICODE
LDFLAGS = -mwindows -municode -static-libgcc -s -lcomctl32 -lshell32 -lole32 -ladvapi32 -lgdi32 -luser32

RM = rm -f
POWERSHELL = /c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe

.PHONY: all clean release

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

clean:
	$(RM) undawn_reroll.exe undawn_reroll.res UndawnReroll_Portable.zip
