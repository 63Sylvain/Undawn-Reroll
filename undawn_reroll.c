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
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

#define APP_TITLE           L"Undawn Reroll"
#define GAME_PROCESS_NAME   L"Toaa-Win64-Shipping.exe"
#define STEAM_RUN_URL       L"steam://rungameid/1881700"
#define AUTH_PREFIX         L"intl_auth.txt."
#define AUTH_PREFIX_LEN     14
#define MAX_ACCOUNT_NAME    100

// Identifiants menu (synchronisés avec undawn_reroll.rc)
#define IDM_MENU            100
#define IDM_OPEN_FOLDER     101
#define IDM_BACKUP_PROFILES 102
#define IDM_EXIT            103
#define IDM_ABOUT           104

// Identifiants contrôles fenêtre principale
#define IDC_LABEL_SELECT    200
#define IDC_COMBO_ACCOUNTS  201
#define IDC_BTN_REFRESH     202
#define IDC_STATUS_ACTIVE   203
#define IDC_STATUS_GAME     204
#define IDC_CHK_STEAM       205
#define IDC_BTN_LAUNCH      206
#define IDC_BTN_CREATE      207
#define IDC_BTN_UPDATE      208
#define IDC_BTN_RENAME      209
#define IDC_BTN_DELETE      210

// Identifiants dialogue de saisie
#define IDC_PROMPT_LABEL    301
#define IDC_PROMPT_EDIT     302

#define MAIN_WND_CLASS      L"UndawnRerollMainWindowClass"
#define PROMPT_WND_CLASS    L"UndawnRerollPromptDialogClass"

typedef struct {
    wchar_t *name;
    wchar_t *auth_file;
} Account;

typedef struct {
    Account *items;
    size_t count;
    size_t capacity;
} AccountList;

typedef struct {
    const wchar_t *prompt_label;
    wchar_t *buffer;
    size_t buffer_cap;
    BOOL accepted;
    BOOL done;
    HFONT font;
    HWND edit_hwnd;
} PromptState;

static AccountList g_accounts = {0};
static wchar_t g_auth_dir[1024] = {0};
static HINSTANCE g_hInstance = NULL;
static HFONT g_ui_font = NULL;
static HWND g_main_hwnd = NULL;
static BOOL g_game_is_running = FALSE;

static int scale_dpi(int px, UINT dpi) {
    return MulDiv(px, (int)dpi, 96);
}

static UINT get_window_dpi(HWND hwnd) {
    typedef UINT (WINAPI *GetDpiForWindowFn)(HWND);
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        FARPROC proc = GetProcAddress(user32, "GetDpiForWindow");
        if (proc && hwnd) {
            GetDpiForWindowFn fn = (GetDpiForWindowFn)(void *)proc;
            UINT d = fn(hwnd);
            if (d > 0) return d;
        }
    }
    HDC hdc = GetDC(hwnd);
    UINT dpi = 96;
    if (hdc) {
        int val = GetDeviceCaps(hdc, LOGPIXELSY);
        if (val > 0) dpi = (UINT)val;
        ReleaseDC(hwnd, hdc);
    }
    return dpi;
}

static HFONT create_ui_font(UINT dpi) {
    NONCLIENTMETRICSW ncm;
    ZeroMemory(&ncm, sizeof(ncm));
    ncm.cbSize = sizeof(ncm);
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {
        // Adapter la hauteur de police si le système a renvoyé un DPI différent
        LOGFONTW lf = ncm.lfMessageFont;
        lf.lfHeight = -MulDiv(9, (int)dpi, 72);
        return CreateFontIndirectW(&lf);
    }
    return CreateFontW(-MulDiv(9, (int)dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

static void show_info(HWND hwnd, const wchar_t *msg) {
    MessageBoxW(hwnd, msg, APP_TITLE, MB_OK | MB_ICONINFORMATION);
}

static void show_error(HWND hwnd, const wchar_t *msg) {
    MessageBoxW(hwnd, msg, APP_TITLE, MB_OK | MB_ICONERROR);
}

static void free_accounts(AccountList *list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; ++i) {
        free(list->items[i].name);
        free(list->items[i].auth_file);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static BOOL add_account(AccountList *list, const wchar_t *name, const wchar_t *auth_file) {
    if (list->count == list->capacity) {
        size_t new_cap = (list->capacity == 0) ? 8 : list->capacity * 2;
        Account *new_items = (Account *)realloc(list->items, new_cap * sizeof(Account));
        if (!new_items) return FALSE;
        list->items = new_items;
        list->capacity = new_cap;
    }

    wchar_t *dup_name = _wcsdup(name);
    wchar_t *dup_file = _wcsdup(auth_file);
    if (!dup_name || !dup_file) {
        free(dup_name);
        free(dup_file);
        return FALSE;
    }

    list->items[list->count].name = dup_name;
    list->items[list->count].auth_file = dup_file;
    list->count++;
    return TRUE;
}

static int compare_accounts(const void *a, const void *b) {
    const Account *acc_a = (const Account *)a;
    const Account *acc_b = (const Account *)b;
    return _wcsicmp(acc_a->name, acc_b->name);
}

static BOOL init_auth_dir(void) {
    if (g_auth_dir[0] != L'\0') {
        return TRUE;
    }

    const wchar_t *local_app = _wgetenv(L"LOCALAPPDATA");
    if (local_app && local_app[0] != L'\0') {
        int n = _snwprintf(g_auth_dir, sizeof(g_auth_dir) / sizeof(g_auth_dir[0]),
                           L"%ls\\Toaa\\INTL\\28014\\Toaa-Win64-Shipping", local_app);
        if (n > 0 && n < (int)(sizeof(g_auth_dir) / sizeof(g_auth_dir[0]))) {
            return TRUE;
        }
        g_auth_dir[0] = L'\0';
    }

    const wchar_t *user_profile = _wgetenv(L"USERPROFILE");
    if (user_profile && user_profile[0] != L'\0') {
        int n = _snwprintf(g_auth_dir, sizeof(g_auth_dir) / sizeof(g_auth_dir[0]),
                           L"%ls\\AppData\\Local\\Toaa\\INTL\\28014\\Toaa-Win64-Shipping", user_profile);
        if (n > 0 && n < (int)(sizeof(g_auth_dir) / sizeof(g_auth_dir[0]))) {
            return TRUE;
        }
        g_auth_dir[0] = L'\0';
    }

    return FALSE;
}

static BOOL file_exists(const wchar_t *path) {
    DWORD attr = GetFileAttributesW(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

static BOOL ensure_auth_dir_exists(void) {
    if (!init_auth_dir()) return FALSE;
    DWORD attr = GetFileAttributesW(g_auth_dir);
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        return TRUE;
    }
    int res = SHCreateDirectoryExW(NULL, g_auth_dir, NULL);
    return (res == ERROR_SUCCESS || res == ERROR_ALREADY_EXISTS);
}

static BOOL copy_file_atomic(const wchar_t *src, const wchar_t *dst) {
    wchar_t tmp_path[1100];
    int n = _snwprintf(tmp_path, sizeof(tmp_path) / sizeof(tmp_path[0]),
                       L"%ls.tmp.%lu", dst, GetCurrentProcessId());
    if (n <= 0 || n >= (int)(sizeof(tmp_path) / sizeof(tmp_path[0]))) {
        return FALSE;
    }

    BOOL copied = CopyFileW(src, tmp_path, FALSE);
    if (!copied) {
        // Fallback avec partage en lecture au cas où un autre processus lit le fichier source
        HANDLE hSrc = CreateFileW(src, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hSrc == INVALID_HANDLE_VALUE) {
            return FALSE;
        }

        HANDLE hDst = CreateFileW(tmp_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL, NULL);
        if (hDst == INVALID_HANDLE_VALUE) {
            CloseHandle(hSrc);
            return FALSE;
        }

        BYTE buffer[8192];
        DWORD bytes_read = 0;
        BOOL rw_ok = TRUE;
        while (ReadFile(hSrc, buffer, sizeof(buffer), &bytes_read, NULL)) {
            if (bytes_read == 0) {
                break;
            }
            DWORD total_written = 0;
            while (total_written < bytes_read) {
                DWORD bytes_written = 0;
                if (!WriteFile(hDst, buffer + total_written, bytes_read - total_written, &bytes_written, NULL) ||
                    bytes_written == 0) {
                    rw_ok = FALSE;
                    break;
                }
                total_written += bytes_written;
            }
            if (!rw_ok) break;
        }
        if (GetLastError() != ERROR_SUCCESS && bytes_read != 0) {
            rw_ok = FALSE;
        }

        CloseHandle(hSrc);
        CloseHandle(hDst);

        if (!rw_ok) {
            DeleteFileW(tmp_path);
            return FALSE;
        }
    }

    if (!MoveFileExW(tmp_path, dst, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(tmp_path);
        return FALSE;
    }
    return TRUE;
}

static BOOL files_are_equal(const wchar_t *path_a, const wchar_t *path_b) {
    HANDLE hA = CreateFileW(path_a, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hA == INVALID_HANDLE_VALUE) return FALSE;

    HANDLE hB = CreateFileW(path_b, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hB == INVALID_HANDLE_VALUE) {
        CloseHandle(hA);
        return FALSE;
    }

    LARGE_INTEGER size_a, size_b;
    BOOL equal = FALSE;
    if (GetFileSizeEx(hA, &size_a) && GetFileSizeEx(hB, &size_b) && size_a.QuadPart == size_b.QuadPart) {
        BYTE buf_a[8192];
        BYTE buf_b[8192];
        equal = TRUE;
        for (;;) {
            DWORD ra = 0, rb = 0;
            if (!ReadFile(hA, buf_a, sizeof(buf_a), &ra, NULL) ||
                !ReadFile(hB, buf_b, sizeof(buf_b), &rb, NULL)) {
                equal = FALSE;
                break;
            }
            if (ra != rb) {
                equal = FALSE;
                break;
            }
            if (ra == 0) {
                break;
            }
            if (memcmp(buf_a, buf_b, ra) != 0) {
                equal = FALSE;
                break;
            }
        }
    }

    CloseHandle(hA);
    CloseHandle(hB);
    return equal;
}

static void trim_whitespace(wchar_t *s) {
    if (!s) return;
    size_t len = wcslen(s);
    size_t start = 0;
    while (start < len && iswspace((wint_t)s[start])) {
        start++;
    }
    size_t end = len;
    while (end > start && iswspace((wint_t)s[end - 1])) {
        end--;
    }
    if (start > 0 && end > start) {
        memmove(s, s + start, (end - start) * sizeof(wchar_t));
    }
    s[end - start] = L'\0';
}

static BOOL is_reserved_windows_name(const wchar_t *name) {
    static const wchar_t *const reserved[] = {
        L"CON", L"PRN", L"AUX", L"NUL",
        L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9",
        L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9"
    };
    for (size_t i = 0; i < sizeof(reserved) / sizeof(reserved[0]); ++i) {
        if (_wcsicmp(name, reserved[i]) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL is_valid_account_name(const wchar_t *name, wchar_t *reason, size_t reason_cap) {
    if (!name || name[0] == L'\0') {
        _snwprintf(reason, reason_cap, L"Le nom du compte ne peut pas être vide.");
        return FALSE;
    }

    size_t len = wcslen(name);
    if (len > MAX_ACCOUNT_NAME) {
        _snwprintf(reason, reason_cap, L"Le nom du compte ne doit pas dépasser %d caractères.", MAX_ACCOUNT_NAME);
        return FALSE;
    }

    if (name[len - 1] == L'.' || name[len - 1] == L' ') {
        _snwprintf(reason, reason_cap, L"Le nom du compte ne peut pas se terminer par un point ou un espace.");
        return FALSE;
    }

    const wchar_t *forbidden = L"\\/:*?\"<>|";
    for (size_t i = 0; i < len; ++i) {
        wchar_t c = name[i];
        if (c < 32 || wcschr(forbidden, c) != NULL) {
            _snwprintf(reason, reason_cap, L"Le nom contient un caractère interdit sous Windows (\\ / : * ? \" < > |).");
            return FALSE;
        }
    }

    if (is_reserved_windows_name(name)) {
        _snwprintf(reason, reason_cap, L"Ce nom est réservé par Windows. Choisissez un autre nom.");
        return FALSE;
    }

    return TRUE;
}

static int find_account_index_ci(const wchar_t *name) {
    for (size_t i = 0; i < g_accounts.count; ++i) {
        if (_wcsicmp(g_accounts.items[i].name, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static BOOL is_game_running(void) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    PROCESSENTRY32W pe;
    ZeroMemory(&pe, sizeof(pe));
    pe.dwSize = sizeof(pe);

    BOOL found = FALSE;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, GAME_PROCESS_NAME) == 0) {
                found = TRUE;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);
    return found;
}

static BOOL confirm_if_game_running(HWND hwnd, const wchar_t *action_desc) {
    g_game_is_running = is_game_running();
    if (!g_game_is_running) {
        return TRUE;
    }

    wchar_t msg[512];
    _snwprintf(msg, sizeof(msg) / sizeof(msg[0]),
               L"Attention : Undawn (%ls) semble actuellement en cours d'exécution.\n\n"
               L"%ls alors que le jeu est ouvert peut être ignoré ou écrasé par le jeu.\n\n"
               L"Voulez-vous tout de même continuer ?",
               GAME_PROCESS_NAME, action_desc);

    int res = MessageBoxW(hwnd, msg, APP_TITLE, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    return (res == IDYES);
}

static void update_status_labels(HWND hwnd) {
    HWND lbl_active = GetDlgItem(hwnd, IDC_STATUS_ACTIVE);
    HWND lbl_game = GetDlgItem(hwnd, IDC_STATUS_GAME);

    if (lbl_active) {
        wchar_t text[512] = L"Compte actif : Indisponible";
        if (init_auth_dir()) {
            wchar_t active_path[1024];
            int n = _snwprintf(active_path, sizeof(active_path) / sizeof(active_path[0]),
                               L"%ls\\intl_auth.txt", g_auth_dir);
            if (n > 0 && n < (int)(sizeof(active_path) / sizeof(active_path[0]))) {
                if (!file_exists(active_path)) {
                    _snwprintf(text, sizeof(text) / sizeof(text[0]),
                               L"Compte actif : Aucun (intl_auth.txt absent)");
                } else {
                    const wchar_t *match_name = NULL;
                    int match_count = 0;
                    for (size_t i = 0; i < g_accounts.count; ++i) {
                        if (files_are_equal(active_path, g_accounts.items[i].auth_file)) {
                            if (!match_name) {
                                match_name = g_accounts.items[i].name;
                            }
                            match_count++;
                        }
                    }
                    if (match_name) {
                        if (match_count > 1) {
                            _snwprintf(text, sizeof(text) / sizeof(text[0]),
                                       L"Compte actif : %ls (+ %d identique%ls)",
                                       match_name, match_count - 1, (match_count - 1) > 1 ? L"s" : L"");
                        } else {
                            _snwprintf(text, sizeof(text) / sizeof(text[0]),
                                       L"Compte actif : %ls", match_name);
                        }
                    } else {
                        _snwprintf(text, sizeof(text) / sizeof(text[0]),
                                   L"Compte actif : Non sauvegardé (profil inconnu)");
                    }
                }
            }
        }
        SetWindowTextW(lbl_active, text);
    }

    g_game_is_running = is_game_running();
    if (lbl_game) {
        if (g_game_is_running) {
            SetWindowTextW(lbl_game, L"État du jeu : Undawn est en cours d'exécution");
        } else {
            SetWindowTextW(lbl_game, L"État du jeu : Undawn fermé");
        }
        InvalidateRect(lbl_game, NULL, TRUE);
    }
}

static void update_action_buttons_state(HWND hwnd) {
    BOOL has_sel = FALSE;
    HWND combo = GetDlgItem(hwnd, IDC_COMBO_ACCOUNTS);
    if (combo && SendMessageW(combo, CB_GETCURSEL, 0, 0) != CB_ERR) {
        has_sel = TRUE;
    }

    EnableWindow(GetDlgItem(hwnd, IDC_BTN_LAUNCH), has_sel);
    EnableWindow(GetDlgItem(hwnd, IDC_BTN_UPDATE), has_sel);
    EnableWindow(GetDlgItem(hwnd, IDC_BTN_RENAME), has_sel);
    EnableWindow(GetDlgItem(hwnd, IDC_BTN_DELETE), has_sel);
}

static void load_accounts(HWND hwnd, const wchar_t *prefer_name) {
    HWND combo = GetDlgItem(hwnd, IDC_COMBO_ACCOUNTS);

    // Sauvegarder la sélection actuelle si aucun nom cible n'est demandé
    wchar_t previous_sel[256] = {0};
    if (!prefer_name && combo) {
        LRESULT cur = SendMessageW(combo, CB_GETCURSEL, 0, 0);
        if (cur != CB_ERR && (size_t)cur < g_accounts.count) {
            wcsncpy(previous_sel, g_accounts.items[cur].name, sizeof(previous_sel) / sizeof(previous_sel[0]) - 1);
        }
    }
    const wchar_t *target_name = prefer_name ? prefer_name : (previous_sel[0] ? previous_sel : NULL);

    free_accounts(&g_accounts);
    if (combo) {
        SendMessageW(combo, CB_RESETCONTENT, 0, 0);
    }

    if (init_auth_dir()) {
        wchar_t pattern[1024];
        int n = _snwprintf(pattern, sizeof(pattern) / sizeof(pattern[0]),
                           L"%ls\\intl_auth.txt.*", g_auth_dir);
        if (n > 0 && n < (int)(sizeof(pattern) / sizeof(pattern[0]))) {
            WIN32_FIND_DATAW ffd;
            HANDLE hFind = FindFirstFileW(pattern, &ffd);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        continue;
                    }
                    if (wcsncmp(ffd.cFileName, AUTH_PREFIX, AUTH_PREFIX_LEN) == 0 &&
                        ffd.cFileName[AUTH_PREFIX_LEN] != L'\0') {
                        const wchar_t *name = ffd.cFileName + AUTH_PREFIX_LEN;
                        // Ignorer d'éventuels fichiers temporaires résiduels (.tmp.<pid>)
                        const wchar_t *tmp_suffix = wcsstr(name, L".tmp.");
                        if (tmp_suffix && iswdigit((wint_t)tmp_suffix[5])) {
                            continue;
                        }
                        wchar_t full_path[1024];
                        int fn = _snwprintf(full_path, sizeof(full_path) / sizeof(full_path[0]),
                                            L"%ls\\%ls", g_auth_dir, ffd.cFileName);
                        if (fn > 0 && fn < (int)(sizeof(full_path) / sizeof(full_path[0]))) {
                            add_account(&g_accounts, name, full_path);
                        }
                    }
                } while (FindNextFileW(hFind, &ffd));
                FindClose(hFind);
            }
        }
    }

    if (g_accounts.count > 1) {
        qsort(g_accounts.items, g_accounts.count, sizeof(Account), compare_accounts);
    }

    if (combo && g_accounts.count > 0) {
        int sel_index = 0;
        for (size_t i = 0; i < g_accounts.count; ++i) {
            SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)g_accounts.items[i].name);
            if (target_name && _wcsicmp(g_accounts.items[i].name, target_name) == 0) {
                sel_index = (int)i;
            }
        }
        SendMessageW(combo, CB_SETCURSEL, (WPARAM)sel_index, 0);
    }

    update_action_buttons_state(hwnd);
    update_status_labels(hwnd);
}

static const Account *get_selected_account(HWND hwnd) {
    HWND combo = GetDlgItem(hwnd, IDC_COMBO_ACCOUNTS);
    if (!combo) return NULL;
    LRESULT idx = SendMessageW(combo, CB_GETCURSEL, 0, 0);
    if (idx == CB_ERR || (size_t)idx >= g_accounts.count) {
        return NULL;
    }
    return &g_accounts.items[idx];
}

static LRESULT CALLBACK prompt_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PromptState *state = (PromptState *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTW *cs = (CREATESTRUCTW *)lParam;
            state = (PromptState *)cs->lpCreateParams;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)state);

            UINT dpi = get_window_dpi(hwnd);
            int pad = scale_dpi(12, dpi);
            int client_w = scale_dpi(340, dpi);
            int lbl_h = scale_dpi(18, dpi);
            int edit_h = scale_dpi(24, dpi);
            int btn_w = scale_dpi(88, dpi);
            int btn_h = scale_dpi(26, dpi);
            int gap = scale_dpi(8, dpi);

            HWND lbl = CreateWindowExW(0, L"STATIC", state->prompt_label,
                                       WS_CHILD | WS_VISIBLE,
                                       pad, pad, client_w - 2 * pad, lbl_h,
                                       hwnd, (HMENU)IDC_PROMPT_LABEL, g_hInstance, NULL);
            SendMessageW(lbl, WM_SETFONT, (WPARAM)state->font, TRUE);

            state->edit_hwnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", state->buffer,
                                               WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                               pad, pad + lbl_h + scale_dpi(6, dpi),
                                               client_w - 2 * pad, edit_h,
                                               hwnd, (HMENU)IDC_PROMPT_EDIT, g_hInstance, NULL);
            SendMessageW(state->edit_hwnd, WM_SETFONT, (WPARAM)state->font, TRUE);
            SendMessageW(state->edit_hwnd, EM_SETLIMITTEXT, (WPARAM)(state->buffer_cap - 1), 0);
            SendMessageW(state->edit_hwnd, EM_SETSEL, 0, -1);

            int btn_y = pad + lbl_h + scale_dpi(6, dpi) + edit_h + pad;
            int cancel_x = client_w - pad - btn_w;
            int ok_x = cancel_x - gap - btn_w;

            HWND btn_ok = CreateWindowExW(0, L"BUTTON", L"Valider",
                                          WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                          ok_x, btn_y, btn_w, btn_h,
                                          hwnd, (HMENU)(INT_PTR)IDOK, g_hInstance, NULL);
            SendMessageW(btn_ok, WM_SETFONT, (WPARAM)state->font, TRUE);

            HWND btn_cancel = CreateWindowExW(0, L"BUTTON", L"Annuler",
                                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                              cancel_x, btn_y, btn_w, btn_h,
                                              hwnd, (HMENU)(INT_PTR)IDCANCEL, g_hInstance, NULL);
            SendMessageW(btn_cancel, WM_SETFONT, (WPARAM)state->font, TRUE);

            SetFocus(state->edit_hwnd);
            return 0;
        }
        case WM_COMMAND: {
            WORD id = LOWORD(wParam);
            if (id == IDOK && state) {
                GetWindowTextW(state->edit_hwnd, state->buffer, (int)state->buffer_cap);
                trim_whitespace(state->buffer);
                wchar_t reason[256];
                if (!is_valid_account_name(state->buffer, reason, sizeof(reason) / sizeof(reason[0]))) {
                    show_error(hwnd, reason);
                    SetFocus(state->edit_hwnd);
                    SendMessageW(state->edit_hwnd, EM_SETSEL, 0, -1);
                    return 0;
                }
                state->accepted = TRUE;
                state->done = TRUE;
                return 0;
            }
            if (id == IDCANCEL && state) {
                state->accepted = FALSE;
                state->done = TRUE;
                return 0;
            }
            break;
        }
        case WM_CLOSE:
            if (state) {
                state->accepted = FALSE;
                state->done = TRUE;
            }
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static BOOL prompt_account_name(HWND parent, const wchar_t *title, const wchar_t *label,
                                const wchar_t *initial, wchar_t *out_buf, size_t out_cap) {
    PromptState state;
    ZeroMemory(&state, sizeof(state));
    state.prompt_label = label;
    state.buffer = out_buf;
    state.buffer_cap = out_cap;
    state.font = g_ui_font;

    if (initial) {
        wcsncpy(out_buf, initial, out_cap - 1);
        out_buf[out_cap - 1] = L'\0';
    } else {
        out_buf[0] = L'\0';
    }

    UINT dpi = get_window_dpi(parent);
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    DWORD ex_style = WS_EX_DLGMODALFRAME;
    RECT rc = {0, 0, scale_dpi(340, dpi), scale_dpi(114, dpi)};
    AdjustWindowRectEx(&rc, style, FALSE, ex_style);

    int dlg_w = rc.right - rc.left;
    int dlg_h = rc.bottom - rc.top;

    RECT prc;
    GetWindowRect(parent, &prc);
    int x = prc.left + ((prc.right - prc.left) - dlg_w) / 2;
    int y = prc.top + ((prc.bottom - prc.top) - dlg_h) / 2;

    HWND dlg = CreateWindowExW(ex_style, PROMPT_WND_CLASS, title, style,
                               x, y, dlg_w, dlg_h, parent, NULL, g_hInstance, &state);
    if (!dlg) return FALSE;

    EnableWindow(parent, FALSE);
    ShowWindow(dlg, SW_SHOW);
    UpdateWindow(dlg);

    MSG msg;
    while (!state.done && GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessageW(dlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    EnableWindow(parent, TRUE);
    SetForegroundWindow(parent);
    DestroyWindow(dlg);

    return state.accepted;
}

static BOOL launch_steam_game(BOOL *out_steam_installed) {
    HKEY hKey = NULL;
    BOOL steam_installed = FALSE;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        steam_installed = TRUE;
        RegCloseKey(hKey);
    }
    if (out_steam_installed) {
        *out_steam_installed = steam_installed;
    }

    HINSTANCE res = ShellExecuteW(NULL, L"open", STEAM_RUN_URL, NULL, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)res > 32) {
        return TRUE;
    }

    // Fallbacks si l'association de protocole échoue via ShellExecuteW
    wchar_t cmd[512];
    _snwprintf(cmd, sizeof(cmd) / sizeof(cmd[0]), L"rundll32.exe url.dll,FileProtocolHandler %ls", STEAM_RUN_URL);
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;

    if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return TRUE;
    }

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;
    _snwprintf(cmd, sizeof(cmd) / sizeof(cmd[0]), L"explorer.exe %ls", STEAM_RUN_URL);
    if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return TRUE;
    }

    return FALSE;
}

static void on_launch_account(HWND hwnd) {
    const Account *account = get_selected_account(hwnd);
    if (!account) {
        show_error(hwnd, L"Aucun compte sélectionné.");
        return;
    }

    if (!file_exists(account->auth_file)) {
        show_error(hwnd, L"Le fichier de ce profil est introuvable sur le disque. La liste va être actualisée.");
        load_accounts(hwnd, NULL);
        return;
    }

    if (!confirm_if_game_running(hwnd, L"Activer un autre compte")) {
        return;
    }

    if (!ensure_auth_dir_exists()) {
        show_error(hwnd, L"Erreur : Impossible d'accéder au répertoire d'authentification.");
        return;
    }

    wchar_t dest_path[1024];
    int n = _snwprintf(dest_path, sizeof(dest_path) / sizeof(dest_path[0]), L"%ls\\intl_auth.txt", g_auth_dir);
    if (n <= 0 || n >= (int)(sizeof(dest_path) / sizeof(dest_path[0]))) {
        show_error(hwnd, L"Erreur : Chemin de destination trop long.");
        return;
    }

    if (!copy_file_atomic(account->auth_file, dest_path) || !files_are_equal(account->auth_file, dest_path)) {
        show_error(hwnd, L"Erreur : Impossible de copier le fichier d'authentification actif.");
        return;
    }

    update_status_labels(hwnd);

    BOOL launch_steam = (SendMessageW(GetDlgItem(hwnd, IDC_CHK_STEAM), BM_GETCHECK, 0, 0) == BST_CHECKED);
    if (launch_steam) {
        BOOL steam_installed = FALSE;
        if (!launch_steam_game(&steam_installed)) {
            if (!steam_installed) {
                show_error(hwnd, L"Compte activé, mais Steam ne semble pas installé sur votre système.");
            } else {
                show_error(hwnd, L"Compte activé, mais erreur lors du lancement du jeu via Steam.");
            }
            return;
        }
        wchar_t msg[384];
        _snwprintf(msg, sizeof(msg) / sizeof(msg[0]),
                   L"Compte \"%ls\" activé.\nLe jeu est en cours de lancement via Steam.", account->name);
        show_info(hwnd, msg);
    } else {
        wchar_t msg[384];
        _snwprintf(msg, sizeof(msg) / sizeof(msg[0]),
                   L"Compte \"%ls\" activé sans lancer Steam.", account->name);
        show_info(hwnd, msg);
    }
}

static void on_create_account(HWND hwnd) {
    if (!ensure_auth_dir_exists()) {
        show_error(hwnd, L"Erreur : Impossible d'accéder au dossier d'authentification.");
        return;
    }

    wchar_t name_buf[MAX_ACCOUNT_NAME + 1] = {0};
    if (!prompt_account_name(hwnd, L"Créer un nouveau compte", L"Nom du compte :", NULL,
                             name_buf, sizeof(name_buf) / sizeof(name_buf[0]))) {
        return;
    }

    if (find_account_index_ci(name_buf) >= 0) {
        show_error(hwnd, L"Un compte portant ce nom existe déjà. Utilisez « Mettre à jour » pour l'écraser.");
        return;
    }

    wchar_t source_path[1024];
    int sn = _snwprintf(source_path, sizeof(source_path) / sizeof(source_path[0]), L"%ls\\intl_auth.txt", g_auth_dir);
    wchar_t new_path[1024];
    int dn = _snwprintf(new_path, sizeof(new_path) / sizeof(new_path[0]), L"%ls\\%ls%ls", g_auth_dir, AUTH_PREFIX, name_buf);
    if (sn <= 0 || sn >= (int)(sizeof(source_path) / sizeof(source_path[0])) ||
        dn <= 0 || dn >= (int)(sizeof(new_path) / sizeof(new_path[0]))) {
        show_error(hwnd, L"Erreur : Chemin trop long.");
        return;
    }

    BOOL has_active = file_exists(source_path);
    if (has_active) {
        if (!confirm_if_game_running(hwnd, L"Sauvegarder la session actuelle")) {
            return;
        }
        if (!copy_file_atomic(source_path, new_path)) {
            show_error(hwnd, L"Erreur : Impossible de copier le fichier d'authentification actuel.");
            return;
        }
    } else {
        int res = MessageBoxW(hwnd,
                              L"Aucun fichier intl_auth.txt actif n'a été trouvé.\n"
                              L"Voulez-vous quand même créer un fichier de profil vide ?",
                              APP_TITLE, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
        if (res != IDYES) {
            return;
        }
        HANDLE hFile = CreateFileW(new_path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            show_error(hwnd, L"Erreur : Impossible de créer le fichier de profil.");
            return;
        }
        CloseHandle(hFile);
    }

    load_accounts(hwnd, name_buf);

    wchar_t msg[384];
    _snwprintf(msg, sizeof(msg) / sizeof(msg[0]), L"Compte \"%ls\" créé avec succès.", name_buf);
    show_info(hwnd, msg);
}

static void on_update_account(HWND hwnd) {
    const Account *account = get_selected_account(hwnd);
    if (!account) {
        show_error(hwnd, L"Aucun compte sélectionné.");
        return;
    }

    if (!init_auth_dir()) {
        show_error(hwnd, L"Erreur : Impossible de déterminer le dossier d'authentification.");
        return;
    }

    wchar_t src_path[1024];
    int n = _snwprintf(src_path, sizeof(src_path) / sizeof(src_path[0]), L"%ls\\intl_auth.txt", g_auth_dir);
    if (n <= 0 || n >= (int)(sizeof(src_path) / sizeof(src_path[0])) || !file_exists(src_path)) {
        show_error(hwnd, L"Erreur : Le fichier d'authentification actif (intl_auth.txt) est introuvable.");
        return;
    }

    if (!confirm_if_game_running(hwnd, L"Mettre à jour ce profil depuis la session active")) {
        return;
    }

    wchar_t selected_name[256];
    wcsncpy(selected_name, account->name, sizeof(selected_name) / sizeof(selected_name[0]) - 1);
    selected_name[sizeof(selected_name) / sizeof(selected_name[0]) - 1] = L'\0';

    if (!copy_file_atomic(src_path, account->auth_file)) {
        show_error(hwnd, L"Erreur : Impossible de mettre à jour le profil (l'ancienne sauvegarde a été conservée).");
        return;
    }

    load_accounts(hwnd, selected_name);

    wchar_t msg[384];
    _snwprintf(msg, sizeof(msg) / sizeof(msg[0]), L"Compte \"%ls\" mis à jour avec succès.", selected_name);
    show_info(hwnd, msg);
}

static void on_rename_account(HWND hwnd) {
    const Account *account = get_selected_account(hwnd);
    if (!account) {
        show_error(hwnd, L"Aucun compte sélectionné.");
        return;
    }

    if (!file_exists(account->auth_file)) {
        show_error(hwnd, L"Le fichier du profil est introuvable sur le disque.");
        load_accounts(hwnd, NULL);
        return;
    }

    wchar_t new_name[MAX_ACCOUNT_NAME + 1] = {0};
    if (!prompt_account_name(hwnd, L"Renommer le compte", L"Nouveau nom :", account->name,
                             new_name, sizeof(new_name) / sizeof(new_name[0]))) {
        return;
    }

    if (wcscmp(new_name, account->name) == 0) {
        return;
    }

    int existing_idx = find_account_index_ci(new_name);
    if (existing_idx >= 0 && &g_accounts.items[existing_idx] != account) {
        show_error(hwnd, L"Un autre compte porte déjà ce nom.");
        return;
    }

    wchar_t new_path[1024];
    int dn = _snwprintf(new_path, sizeof(new_path) / sizeof(new_path[0]),
                        L"%ls\\%ls%ls", g_auth_dir, AUTH_PREFIX, new_name);
    if (dn <= 0 || dn >= (int)(sizeof(new_path) / sizeof(new_path[0]))) {
        show_error(hwnd, L"Erreur : Chemin trop long.");
        return;
    }

    if (!MoveFileW(account->auth_file, new_path)) {
        show_error(hwnd, L"Erreur : Impossible de renommer le fichier du profil.");
        return;
    }

    load_accounts(hwnd, new_name);

    wchar_t msg[384];
    _snwprintf(msg, sizeof(msg) / sizeof(msg[0]), L"Compte renommé en \"%ls\" avec succès.", new_name);
    show_info(hwnd, msg);
}

static void on_delete_account(HWND hwnd) {
    const Account *account = get_selected_account(hwnd);
    if (!account) {
        show_error(hwnd, L"Aucun compte sélectionné.");
        return;
    }

    wchar_t confirm_msg[384];
    _snwprintf(confirm_msg, sizeof(confirm_msg) / sizeof(confirm_msg[0]),
               L"Êtes-vous sûr de vouloir supprimer le compte \"%ls\" ?", account->name);
    if (MessageBoxW(hwnd, confirm_msg, APP_TITLE, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) {
        return;
    }

    wchar_t deleted_name[256];
    wcsncpy(deleted_name, account->name, sizeof(deleted_name) / sizeof(deleted_name[0]) - 1);
    deleted_name[sizeof(deleted_name) / sizeof(deleted_name[0]) - 1] = L'\0';

    if (file_exists(account->auth_file)) {
        if (!DeleteFileW(account->auth_file)) {
            show_error(hwnd, L"Erreur : Impossible de supprimer le fichier du profil sur le disque.");
            return;
        }
    } else {
        show_error(hwnd, L"Le fichier du profil n'existait déjà plus sur le disque.");
        load_accounts(hwnd, NULL);
        return;
    }

    load_accounts(hwnd, NULL);

    wchar_t msg[384];
    _snwprintf(msg, sizeof(msg) / sizeof(msg[0]), L"Compte \"%ls\" supprimé avec succès.", deleted_name);
    show_info(hwnd, msg);
}

static void on_open_folder(HWND hwnd) {
    if (!ensure_auth_dir_exists()) {
        show_error(hwnd, L"Erreur : Impossible d'accéder au dossier des profils.");
        return;
    }
    HINSTANCE res = ShellExecuteW(hwnd, L"open", g_auth_dir, NULL, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)res <= 32) {
        show_error(hwnd, L"Erreur : Impossible d'ouvrir le dossier dans l'Explorateur.");
    }
}

static void on_backup_profiles(HWND hwnd) {
    if (!init_auth_dir()) {
        show_error(hwnd, L"Erreur : Dossier des profils introuvable.");
        return;
    }

    wchar_t active_path[1024];
    _snwprintf(active_path, sizeof(active_path) / sizeof(active_path[0]), L"%ls\\intl_auth.txt", g_auth_dir);
    BOOL has_active = file_exists(active_path);

    if (g_accounts.count == 0 && !has_active) {
        show_info(hwnd, L"Aucun profil ni fichier d'authentification à sauvegarder.");
        return;
    }

    BROWSEINFOW bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.hwndOwner = hwnd;
    bi.lpszTitle = L"Choisissez le dossier où créer la sauvegarde des profils :";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (!pidl) {
        return;
    }

    wchar_t dest_root[MAX_PATH] = {0};
    BOOL got_path = SHGetPathFromIDListW(pidl, dest_root);
    CoTaskMemFree(pidl);
    if (!got_path || dest_root[0] == L'\0') {
        show_error(hwnd, L"Erreur : Dossier sélectionné invalide.");
        return;
    }

    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t backup_dir[1024];
    int bn = _snwprintf(backup_dir, sizeof(backup_dir) / sizeof(backup_dir[0]),
                        L"%ls\\UndawnReroll_Backup_%04u%02u%02u_%02u%02u%02u",
                        dest_root, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    if (bn <= 0 || bn >= (int)(sizeof(backup_dir) / sizeof(backup_dir[0]))) {
        show_error(hwnd, L"Erreur : Chemin de sauvegarde trop long.");
        return;
    }

    int mk = SHCreateDirectoryExW(hwnd, backup_dir, NULL);
    if (mk != ERROR_SUCCESS && mk != ERROR_ALREADY_EXISTS) {
        show_error(hwnd, L"Erreur : Impossible de créer le dossier de sauvegarde.");
        return;
    }

    size_t copied = 0;
    size_t failed = 0;

    if (has_active) {
        wchar_t dst_active[1100];
        _snwprintf(dst_active, sizeof(dst_active) / sizeof(dst_active[0]), L"%ls\\intl_auth.txt", backup_dir);
        if (copy_file_atomic(active_path, dst_active)) {
            copied++;
        } else {
            failed++;
        }
    }

    for (size_t i = 0; i < g_accounts.count; ++i) {
        wchar_t dst_prof[1100];
        int pn = _snwprintf(dst_prof, sizeof(dst_prof) / sizeof(dst_prof[0]),
                            L"%ls\\%ls%ls", backup_dir, AUTH_PREFIX, g_accounts.items[i].name);
        if (pn > 0 && pn < (int)(sizeof(dst_prof) / sizeof(dst_prof[0])) &&
            copy_file_atomic(g_accounts.items[i].auth_file, dst_prof)) {
            copied++;
        } else {
            failed++;
        }
    }

    wchar_t summary[1280];
    if (failed == 0) {
        _snwprintf(summary, sizeof(summary) / sizeof(summary[0]),
                   L"Sauvegarde terminée : %zu fichier(s) copié(s) dans :\n%ls", copied, backup_dir);
        show_info(hwnd, summary);
    } else {
        _snwprintf(summary, sizeof(summary) / sizeof(summary[0]),
                   L"Sauvegarde partielle : %zu fichier(s) copié(s), %zu échec(s) dans :\n%ls",
                   copied, failed, backup_dir);
        show_error(hwnd, summary);
    }
}

static LRESULT CALLBACK main_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            UINT dpi = get_window_dpi(hwnd);
            g_ui_font = create_ui_font(dpi);

            int pad = scale_dpi(14, dpi);
            int client_w = scale_dpi(460, dpi);
            int inner_w = client_w - 2 * pad;
            int y = pad;

            int lbl_h = scale_dpi(18, dpi);
            HWND lbl_sel = CreateWindowExW(0, L"STATIC", L"Compte sélectionné :",
                                           WS_CHILD | WS_VISIBLE,
                                           pad, y, inner_w, lbl_h,
                                           hwnd, (HMENU)IDC_LABEL_SELECT, g_hInstance, NULL);
            SendMessageW(lbl_sel, WM_SETFONT, (WPARAM)g_ui_font, TRUE);
            y += lbl_h + scale_dpi(4, dpi);

            int refresh_w = scale_dpi(96, dpi);
            int gap = scale_dpi(8, dpi);
            int combo_w = inner_w - refresh_w - gap;
            int combo_h = scale_dpi(24, dpi);

            HWND combo = CreateWindowExW(0, L"COMBOBOX", NULL,
                                         WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
                                         CBS_DROPDOWNLIST | CBS_HASSTRINGS,
                                         pad, y, combo_w, scale_dpi(220, dpi),
                                         hwnd, (HMENU)IDC_COMBO_ACCOUNTS, g_hInstance, NULL);
            SendMessageW(combo, WM_SETFONT, (WPARAM)g_ui_font, TRUE);

            HWND btn_refresh = CreateWindowExW(0, L"BUTTON", L"Rafraîchir",
                                               WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                               pad + combo_w + gap, y - scale_dpi(1, dpi), refresh_w, combo_h + scale_dpi(2, dpi),
                                               hwnd, (HMENU)IDC_BTN_REFRESH, g_hInstance, NULL);
            SendMessageW(btn_refresh, WM_SETFONT, (WPARAM)g_ui_font, TRUE);
            y += combo_h + scale_dpi(12, dpi);

            HWND lbl_active = CreateWindowExW(0, L"STATIC", L"Compte actif : ...",
                                              WS_CHILD | WS_VISIBLE | SS_ENDELLIPSIS,
                                              pad, y, inner_w, lbl_h,
                                              hwnd, (HMENU)IDC_STATUS_ACTIVE, g_hInstance, NULL);
            SendMessageW(lbl_active, WM_SETFONT, (WPARAM)g_ui_font, TRUE);
            y += lbl_h + scale_dpi(4, dpi);

            HWND lbl_game = CreateWindowExW(0, L"STATIC", L"État du jeu : ...",
                                            WS_CHILD | WS_VISIBLE | SS_ENDELLIPSIS,
                                            pad, y, inner_w, lbl_h,
                                            hwnd, (HMENU)IDC_STATUS_GAME, g_hInstance, NULL);
            SendMessageW(lbl_game, WM_SETFONT, (WPARAM)g_ui_font, TRUE);
            y += lbl_h + scale_dpi(10, dpi);

            HWND chk_steam = CreateWindowExW(0, L"BUTTON", L"Lancer le jeu via Steam",
                                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                             pad, y, inner_w, scale_dpi(20, dpi),
                                             hwnd, (HMENU)IDC_CHK_STEAM, g_hInstance, NULL);
            SendMessageW(chk_steam, WM_SETFONT, (WPARAM)g_ui_font, TRUE);
            SendMessageW(chk_steam, BM_SETCHECK, BST_CHECKED, 0);
            y += scale_dpi(20, dpi) + scale_dpi(12, dpi);

            int btn3_w = (inner_w - 2 * gap) / 3;
            int btn_h = scale_dpi(28, dpi);

            HWND btn_launch = CreateWindowExW(0, L"BUTTON", L"Lancer",
                                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                              pad, y, btn3_w, btn_h,
                                              hwnd, (HMENU)IDC_BTN_LAUNCH, g_hInstance, NULL);
            SendMessageW(btn_launch, WM_SETFONT, (WPARAM)g_ui_font, TRUE);

            HWND btn_create = CreateWindowExW(0, L"BUTTON", L"Créer",
                                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                              pad + btn3_w + gap, y, btn3_w, btn_h,
                                              hwnd, (HMENU)IDC_BTN_CREATE, g_hInstance, NULL);
            SendMessageW(btn_create, WM_SETFONT, (WPARAM)g_ui_font, TRUE);

            HWND btn_update = CreateWindowExW(0, L"BUTTON", L"Mettre à jour",
                                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                              pad + 2 * (btn3_w + gap), y, inner_w - 2 * (btn3_w + gap), btn_h,
                                              hwnd, (HMENU)IDC_BTN_UPDATE, g_hInstance, NULL);
            SendMessageW(btn_update, WM_SETFONT, (WPARAM)g_ui_font, TRUE);
            y += btn_h + gap;

            int btn2_w = (inner_w - gap) / 2;
            HWND btn_rename = CreateWindowExW(0, L"BUTTON", L"Renommer",
                                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                              pad, y, btn2_w, btn_h,
                                              hwnd, (HMENU)IDC_BTN_RENAME, g_hInstance, NULL);
            SendMessageW(btn_rename, WM_SETFONT, (WPARAM)g_ui_font, TRUE);

            HWND btn_delete = CreateWindowExW(0, L"BUTTON", L"Supprimer",
                                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                              pad + btn2_w + gap, y, inner_w - btn2_w - gap, btn_h,
                                              hwnd, (HMENU)IDC_BTN_DELETE, g_hInstance, NULL);
            SendMessageW(btn_delete, WM_SETFONT, (WPARAM)g_ui_font, TRUE);

            load_accounts(hwnd, NULL);
            return 0;
        }
        case WM_ACTIVATE:
            if (LOWORD(wParam) != WA_INACTIVE) {
                update_status_labels(hwnd);
            }
            return 0;
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            HWND ctrl = (HWND)lParam;
            SetBkMode(hdc, TRANSPARENT);
            if (GetDlgCtrlID(ctrl) == IDC_STATUS_GAME) {
                SetTextColor(hdc, g_game_is_running ? RGB(180, 30, 30) : RGB(20, 120, 40));
            } else {
                SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
            }
            return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
        }
        case WM_COMMAND: {
            WORD id = LOWORD(wParam);
            WORD code = HIWORD(wParam);
            switch (id) {
                case IDC_COMBO_ACCOUNTS:
                    if (code == CBN_SELCHANGE) {
                        update_action_buttons_state(hwnd);
                    }
                    break;
                case IDC_BTN_REFRESH:
                    load_accounts(hwnd, NULL);
                    break;
                case IDC_BTN_LAUNCH:
                    on_launch_account(hwnd);
                    break;
                case IDC_BTN_CREATE:
                    on_create_account(hwnd);
                    break;
                case IDC_BTN_UPDATE:
                    on_update_account(hwnd);
                    break;
                case IDC_BTN_RENAME:
                    on_rename_account(hwnd);
                    break;
                case IDC_BTN_DELETE:
                    on_delete_account(hwnd);
                    break;
                case IDM_OPEN_FOLDER:
                    on_open_folder(hwnd);
                    break;
                case IDM_BACKUP_PROFILES:
                    on_backup_profiles(hwnd);
                    break;
                case IDM_EXIT:
                    DestroyWindow(hwnd);
                    break;
                case IDM_ABOUT:
                    MessageBoxW(hwnd, L"Made by Th0r", L"À propos", MB_OK | MB_ICONINFORMATION);
                    break;
            }
            return 0;
        }
        case WM_DESTROY:
            free_accounts(&g_accounts);
            if (g_ui_font) {
                DeleteObject(g_ui_font);
                g_ui_font = NULL;
            }
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)pCmdLine;

    g_hInstance = hInstance;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    HICON hIconBig = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON,
                                       GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    HICON hIconSmall = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON,
                                         GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = main_wnd_proc;
    wc.hInstance = hInstance;
    wc.hIcon = hIconBig;
    wc.hIconSm = hIconSmall;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = MAKEINTRESOURCEW(IDM_MENU);
    wc.lpszClassName = MAIN_WND_CLASS;

    if (!RegisterClassExW(&wc)) {
        if (SUCCEEDED(hr)) CoUninitialize();
        return 1;
    }

    WNDCLASSEXW pwc;
    ZeroMemory(&pwc, sizeof(pwc));
    pwc.cbSize = sizeof(pwc);
    pwc.lpfnWndProc = prompt_wnd_proc;
    pwc.hInstance = hInstance;
    pwc.hIcon = hIconBig;
    pwc.hIconSm = hIconSmall;
    pwc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    pwc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    pwc.lpszClassName = PROMPT_WND_CLASS;

    if (!RegisterClassExW(&pwc)) {
        if (SUCCEEDED(hr)) CoUninitialize();
        return 1;
    }

    UINT dpi = get_window_dpi(NULL);
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT rc = {0, 0, scale_dpi(460, dpi), scale_dpi(218, dpi)};
    AdjustWindowRectEx(&rc, style, TRUE, 0);

    int win_w = rc.right - rc.left;
    int win_h = rc.bottom - rc.top;

    RECT work_area;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0);
    int pos_x = work_area.left + ((work_area.right - work_area.left) - win_w) / 2;
    int pos_y = work_area.top + ((work_area.bottom - work_area.top) - win_h) / 2;

    g_main_hwnd = CreateWindowExW(0, MAIN_WND_CLASS, APP_TITLE, style,
                                  pos_x, pos_y, win_w, win_h,
                                  NULL, NULL, hInstance, NULL);
    if (!g_main_hwnd) {
        if (SUCCEEDED(hr)) CoUninitialize();
        return 1;
    }

    ShowWindow(g_main_hwnd, nCmdShow);
    UpdateWindow(g_main_hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessageW(g_main_hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (SUCCEEDED(hr)) {
        CoUninitialize();
    }
    return (int)msg.wParam;
}
