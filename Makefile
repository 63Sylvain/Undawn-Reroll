CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -mwindows -D_WIN32_WINNT=0x0600 -D__USE_MINGW_ANSI_STDIO=1 -DUNICODE -D_UNICODE $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS = $(shell pkg-config --libs gtk+-3.0) -lshlwapi -lole32 -loleaut32 -luuid -mwindows -municode -static-libgcc -Wl,-e,wWinMain -lcomctl32 -lgdi32 -lws2_32 -lwinmm -limm32 -lsetupapi -lcfgmgr32 -lusp10 -lwininet -loleacc -Wl,--subsystem,windows -lole32 -loleaut32 -luuid

# Pour Windows, utiliser rm -f
RM = rm -f

all: undawn_reroll

# Compilation du fichier de ressources
undawn_reroll.res: undawn_reroll.rc
	windres $< -O coff -o $@

undawn_reroll: undawn_reroll.c undawn_reroll.res
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) undawn_reroll.exe

.PHONY: all clean