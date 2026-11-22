#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <windows.h>
#include <stdio.h>
#include <wchar.h>

#define ID_ZIPFILE 101

static void remove_dir_recursive(const wchar_t *dir_path) {
    wchar_t pattern[1024];
    if (_snwprintf(pattern, sizeof(pattern) / sizeof(pattern[0]), L"%ls\\*", dir_path) < 0) {
        return;
    }

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(pattern, &ffd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0) {
                continue;
            }
            wchar_t child[1024];
            if (_snwprintf(child, sizeof(child) / sizeof(child[0]), L"%ls\\%ls", dir_path, ffd.cFileName) < 0) {
                continue;
            }
            SetFileAttributesW(child, FILE_ATTRIBUTE_NORMAL);
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                remove_dir_recursive(child);
            } else {
                DeleteFileW(child);
            }
        } while (FindNextFileW(hFind, &ffd));
        FindClose(hFind);
    }

    SetFileAttributesW(dir_path, FILE_ATTRIBUTE_NORMAL);
    RemoveDirectoryW(dir_path);
}

static void escape_ps_single_quotes(const wchar_t *src, wchar_t *dst, size_t dst_cap) {
    size_t j = 0;
    for (size_t i = 0; src[i] != L'\0' && j + 1 < dst_cap; ++i) {
        if (src[i] == L'\'') {
            if (j + 2 >= dst_cap) break;
            dst[j++] = L'\'';
            dst[j++] = L'\'';
        } else {
            dst[j++] = src[i];
        }
    }
    dst[j] = L'\0';
}

static BOOL run_hidden_command(wchar_t *cmd_line) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (!CreateProcessW(NULL, cmd_line, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return FALSE;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 1;
    BOOL ok = GetExitCodeProcess(pi.hProcess, &exit_code) && (exit_code == 0);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return ok;
}

static BOOL extract_archive(const wchar_t *zip_path, const wchar_t *work_dir) {
    wchar_t sys_dir[MAX_PATH];
    UINT len = GetSystemDirectoryW(sys_dir, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        wchar_t tar_path[MAX_PATH];
        if (_snwprintf(tar_path, MAX_PATH, L"%ls\\tar.exe", sys_dir) > 0 &&
            GetFileAttributesW(tar_path) != INVALID_FILE_ATTRIBUTES) {
            wchar_t tar_cmd[2048];
            if (_snwprintf(tar_cmd, sizeof(tar_cmd) / sizeof(tar_cmd[0]),
                           L"\"%ls\" -xf \"%ls\" -C \"%ls\"", tar_path, zip_path, work_dir) > 0) {
                if (run_hidden_command(tar_cmd)) {
                    return TRUE;
                }
            }
        }
    }

    // Fallback PowerShell si tar.exe est absent ou échoue
    wchar_t zip_esc[1024];
    wchar_t dir_esc[1024];
    escape_ps_single_quotes(zip_path, zip_esc, sizeof(zip_esc) / sizeof(zip_esc[0]));
    escape_ps_single_quotes(work_dir, dir_esc, sizeof(dir_esc) / sizeof(dir_esc[0]));

    wchar_t ps_cmd[4096];
    if (_snwprintf(ps_cmd, sizeof(ps_cmd) / sizeof(ps_cmd[0]),
                   L"powershell.exe -NoProfile -WindowStyle Hidden -Command \"Expand-Archive -LiteralPath '%ls' -DestinationPath '%ls' -Force\"",
                   zip_esc, dir_esc) <= 0) {
        return FALSE;
    }
    return run_hidden_command(ps_cmd);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)pCmdLine;
    (void)nCmdShow;

    wchar_t temp_path[MAX_PATH];
    DWORD temp_len = GetTempPathW(MAX_PATH, temp_path);
    if (temp_len == 0 || temp_len >= MAX_PATH) {
        MessageBoxW(NULL, L"Erreur : Impossible de localiser le dossier temporaire.", L"Erreur", MB_ICONERROR);
        return 1;
    }

    wchar_t work_dir[MAX_PATH];
    if (_snwprintf(work_dir, MAX_PATH, L"%lsUndawnReroll_%lu_%lu",
                   temp_path, GetCurrentProcessId(), GetTickCount()) < 0) {
        MessageBoxW(NULL, L"Erreur : Chemin temporaire trop long.", L"Erreur", MB_ICONERROR);
        return 1;
    }

    if (!CreateDirectoryW(work_dir, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        MessageBoxW(NULL, L"Erreur : Impossible de créer le dossier temporaire.", L"Erreur", MB_ICONERROR);
        return 1;
    }

    int exit_status = 1;
    wchar_t zip_path[MAX_PATH];
    if (_snwprintf(zip_path, MAX_PATH, L"%ls\\app.zip", work_dir) < 0) {
        MessageBoxW(NULL, L"Erreur : Chemin ZIP trop long.", L"Erreur", MB_ICONERROR);
        goto cleanup;
    }

    HRSRC hRes = FindResourceW(hInstance, MAKEINTRESOURCEW(ID_ZIPFILE), RT_RCDATA);
    if (!hRes) {
        MessageBoxW(NULL, L"Erreur : Impossible de trouver la ressource ZIP.", L"Erreur", MB_ICONERROR);
        goto cleanup;
    }

    HGLOBAL hData = LoadResource(hInstance, hRes);
    if (!hData) {
        MessageBoxW(NULL, L"Erreur : Impossible de charger la ressource ZIP.", L"Erreur", MB_ICONERROR);
        goto cleanup;
    }

    DWORD data_size = SizeofResource(hInstance, hRes);
    const void *pData = LockResource(hData);
    if (!pData || data_size == 0) {
        MessageBoxW(NULL, L"Erreur : Ressource ZIP invalide.", L"Erreur", MB_ICONERROR);
        goto cleanup;
    }

    HANDLE hFile = CreateFileW(zip_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        MessageBoxW(NULL, L"Erreur : Impossible d'écrire le fichier ZIP temporaire.", L"Erreur", MB_ICONERROR);
        goto cleanup;
    }

    const BYTE *ptr = (const BYTE *)pData;
    DWORD remaining = data_size;
    BOOL write_ok = TRUE;
    while (remaining > 0) {
        DWORD written = 0;
        if (!WriteFile(hFile, ptr, remaining, &written, NULL) || written == 0) {
            write_ok = FALSE;
            break;
        }
        ptr += written;
        remaining -= written;
    }
    CloseHandle(hFile);

    if (!write_ok) {
        MessageBoxW(NULL, L"Erreur : Écriture incomplète du fichier ZIP temporaire.", L"Erreur", MB_ICONERROR);
        goto cleanup;
    }

    if (!extract_archive(zip_path, work_dir)) {
        MessageBoxW(NULL, L"Erreur lors de la décompression de l'application.", L"Erreur", MB_ICONERROR);
        goto cleanup;
    }

    wchar_t exe_path[MAX_PATH];
    if (_snwprintf(exe_path, MAX_PATH, L"%ls\\undawn_reroll.exe", work_dir) < 0) {
        MessageBoxW(NULL, L"Erreur : Chemin de l'exécutable trop long.", L"Erreur", MB_ICONERROR);
        goto cleanup;
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    if (CreateProcessW(exe_path, NULL, NULL, NULL, FALSE, 0, NULL, work_dir, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        exit_status = 0;
    } else {
        MessageBoxW(NULL, L"Impossible de lancer undawn_reroll.exe.", L"Erreur", MB_ICONERROR);
    }

cleanup:
    remove_dir_recursive(work_dir);
    return exit_status;
}
