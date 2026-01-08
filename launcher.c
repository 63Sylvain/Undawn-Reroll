#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define ID_ZIPFILE 101

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // 1. Préparer le dossier temporaire unique
    char tempPath[MAX_PATH];
    GetTempPath(MAX_PATH, tempPath);
    
    char workDir[MAX_PATH];
    // Utiliser GetTickCount pour un nom unique simple
    sprintf(workDir, "%sUndawnReroll_%lu", tempPath, GetTickCount());
    
    // Créer le dossier
    CreateDirectory(workDir, NULL);

    char zipPath[MAX_PATH];
    sprintf(zipPath, "%s\\app.zip", workDir);

    // 2. Extraire le ZIP depuis les ressources
    HRSRC hRes = FindResource(hInstance, MAKEINTRESOURCE(ID_ZIPFILE), RT_RCDATA);
    if (!hRes) {
        MessageBox(NULL, "Erreur: Impossible de trouver la ressource ZIP.", "Erreur", MB_ICONERROR);
        return 1;
    }
    
    HGLOBAL hData = LoadResource(hInstance, hRes);
    if (!hData) {
        MessageBox(NULL, "Erreur: Impossible de charger la ressource ZIP.", "Erreur", MB_ICONERROR);
        return 1;
    }
    
    DWORD dataSize = SizeofResource(hInstance, hRes);
    void* pData = LockResource(hData);

    FILE* fp = fopen(zipPath, "wb");
    if (!fp) {
        MessageBox(NULL, "Erreur: Impossible d'écrire le fichier ZIP temporaire.", "Erreur", MB_ICONERROR);
        return 1;
    }
    fwrite(pData, 1, dataSize, fp);
    fclose(fp);

    // 3. Décompresser avec PowerShell (discret)
    char cmd[4096];
    // On utilise Expand-Archive. WindowStyle Hidden pour cacher la fenêtre PowerShell.
    sprintf(cmd, "powershell -WindowStyle Hidden -Command \"Expand-Archive -Path '%s' -DestinationPath '%s' -Force\"", zipPath, workDir);
    
    // system() affiche une fenêtre cmd, on préfère CreateProcess ou ShellExecute pour cacher, 
    // mais system est simple. Pour cacher la fenêtre CMD de system(), c'est dur.
    // On va utiliser ShellExecuteEx ou CreateProcess pour lancer powershell proprement.
    
    STARTUPINFO si_ps;
    PROCESS_INFORMATION pi_ps;
    ZeroMemory(&si_ps, sizeof(si_ps));
    si_ps.cb = sizeof(si_ps);
    si_ps.dwFlags = STARTF_USESHOWWINDOW;
    si_ps.wShowWindow = SW_HIDE; // Cacher la fenêtre
    ZeroMemory(&pi_ps, sizeof(pi_ps));

    // Commande complète pour CreateProcess
    char psCmd[4096];
    sprintf(psCmd, "powershell -WindowStyle Hidden -Command \"Expand-Archive -Path '%s' -DestinationPath '%s' -Force\"", zipPath, workDir);
    
    if (CreateProcess(NULL, psCmd, NULL, NULL, FALSE, 0, NULL, NULL, &si_ps, &pi_ps)) {
        WaitForSingleObject(pi_ps.hProcess, INFINITE);
        CloseHandle(pi_ps.hProcess);
        CloseHandle(pi_ps.hThread);
    } else {
        MessageBox(NULL, "Erreur lors de la décompression.", "Erreur", MB_ICONERROR);
        return 1;
    }

    // 4. Lancer l'application extraite
    char exePath[MAX_PATH];
    sprintf(exePath, "%s\\undawn_reroll.exe", workDir);
    
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcess(exePath, NULL, NULL, NULL, FALSE, 0, NULL, workDir, &si, &pi)) {
        // Attendre que l'application se termine (optionnel, mais utile pour le nettoyage)
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        MessageBox(NULL, "Impossible de lancer undawn_reroll.exe.", "Erreur", MB_ICONERROR);
    }

    // 5. Nettoyage (supprimer le dossier temporaire)
    // On relance une commande powershell pour supprimer récursivement
    sprintf(psCmd, "powershell -WindowStyle Hidden -Command \"Remove-Item -Path '%s' -Recurse -Force\"", workDir);
    
    ZeroMemory(&si_ps, sizeof(si_ps));
    si_ps.cb = sizeof(si_ps);
    si_ps.dwFlags = STARTF_USESHOWWINDOW;
    si_ps.wShowWindow = SW_HIDE;
    ZeroMemory(&pi_ps, sizeof(pi_ps));
    
    if (CreateProcess(NULL, psCmd, NULL, NULL, FALSE, 0, NULL, NULL, &si_ps, &pi_ps)) {
        WaitForSingleObject(pi_ps.hProcess, INFINITE); // Attendre un peu que ça finisse
        CloseHandle(pi_ps.hProcess);
        CloseHandle(pi_ps.hThread);
    }

    return 0;
}
