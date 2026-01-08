#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include <io.h>
#include <direct.h>
#include <winreg.h>
#include <shlobj.h>

// Définition des identifiants de menu
#define IDM_ABOUT 101

// Début du programme

void create_auth_file(const wchar_t *account_name);
void delete_auth_file(const wchar_t *account_name);
void on_delete_account(GtkWidget *widget, gpointer data);
void on_update_account(GtkWidget *widget, gpointer data);
void on_create_account(GtkWidget *widget, gpointer data);
void on_select_account(GtkWidget *widget, gpointer data);

// Suppression des attributs unused pour éviter les warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

typedef struct {
    wchar_t *name;
    wchar_t *auth_file;
} Account;

// Variables globales
GList *accounts = NULL;
GtkWidget *account_combo = NULL;
wchar_t g_auth_dir[1024] = {0}; // Cache du chemin d'authentification

// Initialise le chemin du dossier d'authentification s'il n'est pas déjà défini
static void init_auth_dir(void) {
    if (g_auth_dir[0] == 0) {
        wchar_t *user_profile = _wgetenv(L"USERPROFILE");
        if (user_profile) {
            swprintf(g_auth_dir, sizeof(g_auth_dir)/sizeof(g_auth_dir[0]), 
                    L"%ls\\AppData\\Local\\Toaa\\INTL\\28014\\Toaa-Win64-Shipping", user_profile);
        }
    }
}

void load_accounts(void) {
    // Initialiser le chemin d'authentification
    init_auth_dir();
    if (g_auth_dir[0] == 0) {
        fprintf(stderr, "Erreur: Impossible d'obtenir le chemin du profil utilisateur\n");
        return;
    }
    
    // Nettoyer la liste existante
    if (accounts) {
        for(GList *iter = accounts; iter != NULL; iter = iter->next) {
            Account *account = iter->data;
            free(account->name);
            free(account->auth_file);
            free(account);
        }
        g_list_free(accounts);
        accounts = NULL;
    }
    
    // Rechercher tous les fichiers intl_auth.txt.* dans le dossier
    wchar_t search_pattern[1024];
    swprintf(search_pattern, sizeof(search_pattern)/sizeof(search_pattern[0]), L"%ls\\intl_auth.txt.*", g_auth_dir);
    
    WIN32_FIND_DATAW find_data;
    HANDLE find_handle = FindFirstFileW(search_pattern, &find_data);
    
    if(find_handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Aucun compte trouvé dans %ls\n", g_auth_dir);
        return;
    }

    int count = 0;
    
    do {
        // Ignorer les entrées . et ..
        if(wcscmp(find_data.cFileName, L".") == 0 || wcscmp(find_data.cFileName, L"..") == 0)
            continue;
        
        // Vérifier que le fichier commence bien par "intl_auth.txt."
        if(wcsncmp(find_data.cFileName, L"intl_auth.txt.", 14) == 0 && wcslen(find_data.cFileName) > 14) {
            // Extraire le nom du compte à partir du nom du fichier (après le dernier point)
            WCHAR *account_name = wcsrchr(find_data.cFileName, L'.');
            if(account_name) {
                account_name++; // Sauter le point
                
                // Créer le chemin complet du fichier
                wchar_t full_path[1024];
                swprintf(full_path, sizeof(full_path)/sizeof(full_path[0]), L"%ls\\%ls", g_auth_dir, find_data.cFileName);
                
                // Créer et ajouter le compte à la liste
                Account *account = malloc(sizeof(Account));
                if (!account) continue; // Vérification d'allocation
                
                // Dupliquer directement les chaînes sans conversion inutile
                account->name = _wcsdup(account_name);
                account->auth_file = _wcsdup(full_path);
                
                if (!account->name || !account->auth_file) {
                    // Gestion d'erreur d'allocation
                    if (account->name) free(account->name);
                    if (account->auth_file) free(account->auth_file);
                    free(account);
                    continue;
                }
                
                accounts = g_list_append(accounts, account);
                count++;
            }
        }
    } while(FindNextFileW(find_handle, &find_data));
    
    FindClose(find_handle);
    
    fprintf(stderr, "Info: %d compte(s) trouvé(s)\n", count);
    
    // Mettre à jour le combo box seulement s'il existe
    if(count > 0 && account_combo != NULL) {
        gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(account_combo));
        for(GList *iter = accounts; iter != NULL; iter = iter->next) {
            Account *account = iter->data;
            gchar *name_utf8 = g_utf16_to_utf8((gunichar2*)account->name, -1, NULL, NULL, NULL);
            if (name_utf8) {
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(account_combo), name_utf8);
                g_free(name_utf8);
            }
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(account_combo), 0);
    }
}

// Fonction pour afficher la boîte de dialogue "À propos"
void show_about_dialog(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_application_get_active_window(NULL)),
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_INFO,
                                             GTK_BUTTONS_OK,
                                             "Made by Th0r");
    gtk_window_set_title(GTK_WINDOW(dialog), "À propos");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

int WINAPI wWinMain(HINSTANCE hInstance __attribute__((unused)), HINSTANCE hPrevInstance __attribute__((unused)), PWSTR pCmdLine __attribute__((unused)), int nCmdShow __attribute__((unused))) {
    // Initialiser GTK
    gtk_init(NULL, NULL);
    
    // Créer la fenêtre principale
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Undawn Reroll");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
    // Fonction de fermeture complète du programme
    void exit_program(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
        // Libérer les ressources
        g_list_free_full(accounts, (GDestroyNotify)free);
        gtk_widget_destroy(account_combo);
        gtk_main_quit();
        ExitProcess(0); // Force la fermeture complète du processus
    }
    
    // Gestion des événements de fermeture
    g_signal_connect(window, "delete-event", G_CALLBACK(exit_program), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(exit_program), NULL);
    
    // Créer la barre de menu
    GtkWidget *menubar = gtk_menu_bar_new();
    
    // Créer le menu "À propos"
    GtkWidget *about_menu = gtk_menu_new();
    GtkWidget *about_item = gtk_menu_item_new_with_label("À propos");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(about_item), about_menu);
    
    // Créer l'élément de menu "Made by Th0r"
    GtkWidget *made_by_item = gtk_menu_item_new_with_label("Made by Th0r");
    g_signal_connect(made_by_item, "activate", G_CALLBACK(show_about_dialog), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(about_menu), made_by_item);
    
    // Ajouter le menu à la barre de menu
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), about_item);
    
    // Créer un conteneur principal
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(box), menubar, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), box);
    
    // Créer un sélecteur de compte
    account_combo = gtk_combo_box_text_new();
    gtk_box_pack_start(GTK_BOX(box), account_combo, FALSE, FALSE, 0);
    
    // Créer un conteneur pour les boutons
    GtkWidget *button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(box), button_box, FALSE, FALSE, 0);
    
    // Bouton pour lancer un compte
    GtkWidget *select_button = gtk_button_new_with_label("Lancer");
    g_signal_connect(select_button, "clicked", G_CALLBACK(on_select_account), NULL);
    gtk_container_add(GTK_CONTAINER(button_box), select_button);
    
    // Bouton pour créer un nouveau compte
    GtkWidget *create_button = gtk_button_new_with_label("Créer");
    g_signal_connect(create_button, "clicked", G_CALLBACK(on_create_account), NULL);
    gtk_container_add(GTK_CONTAINER(button_box), create_button);
    
    // Bouton pour supprimer un compte
    GtkWidget *delete_button = gtk_button_new_with_label("Supprimer");
    g_signal_connect(delete_button, "clicked", G_CALLBACK(on_delete_account), NULL);
    gtk_container_add(GTK_CONTAINER(button_box), delete_button);
    
    // Bouton pour mettre à jour un compte
    GtkWidget *update_button = gtk_button_new_with_label("Mettre à jour");
    g_signal_connect(update_button, "clicked", G_CALLBACK(on_update_account), NULL);
    gtk_container_add(GTK_CONTAINER(button_box), update_button);
    
    // Charger les comptes disponibles
    load_accounts();
    
    // Afficher tous les widgets
    gtk_widget_show_all(window);
    
    // Lancer la boucle principale GTK
    gtk_main();
    return 0;
}

void on_select_account(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    // Initialiser le chemin d'authentification
    init_auth_dir();
    if (g_auth_dir[0] == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
            "Erreur: Impossible d'obtenir le chemin du profil utilisateur");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    if(account_combo == NULL || accounts == NULL) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Aucun compte disponible");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(account_combo));
    if(active < 0) return;

    GList *item = g_list_nth(accounts, active);
    if(item == NULL) return;

    Account *account = item->data;
    
    // Vérifier que le fichier source existe
    if(_waccess(account->auth_file, 0) != 0) {
        // Vérifier si le fichier existe dans un autre emplacement
        wchar_t alt_path[1024];
        swprintf(alt_path, sizeof(alt_path)/sizeof(alt_path[0]), L"%ls\\intl_auth.txt.%ls", g_auth_dir, account->name);
        
        if(_waccess(alt_path, 0) == 0) {
            // Mettre à jour le chemin du fichier dans la structure Account
            free(account->auth_file);
            account->auth_file = _wcsdup(alt_path);
            fprintf(stderr, "Info: Fichier d'authentification trouvé à l'emplacement alternatif: %ls\n", alt_path);
        } else {
            // Essayer de créer un nouveau fichier d'authentification vide
            create_auth_file(account->name);
            if(_waccess(account->auth_file, 0) != 0) {
                GtkWidget *dialog = gtk_message_dialog_new(NULL, 
                    GTK_DIALOG_MODAL, 
                    GTK_MESSAGE_ERROR, 
                    GTK_BUTTONS_OK, 
                    "Erreur: Impossible de créer le fichier d'authentification");
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                return;
            }
        }
    }
    
    // Chemin du fichier de destination
    wchar_t dest_path[1024];
    swprintf(dest_path, sizeof(dest_path)/sizeof(dest_path[0]), L"%ls\\intl_auth.txt", g_auth_dir);
    
    // Supprimer le fichier de destination s'il existe déjà
    if(_waccess(dest_path, 0) == 0) {
        if(_wremove(dest_path) != 0) {
            fprintf(stderr, "Erreur: Impossible de supprimer le fichier d'authentification existant: %ls\n", dest_path);
            GtkWidget *dialog = gtk_message_dialog_new(NULL, 
                GTK_DIALOG_MODAL, 
                GTK_MESSAGE_ERROR, 
                GTK_BUTTONS_OK, 
                "Erreur: Impossible de supprimer le fichier d'authentification existant");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return;
        }
        fprintf(stderr, "Info: Fichier d'authentification existant supprimé: %ls\n", dest_path);
    }
    
    // Le répertoire g_auth_dir devrait déjà exister grâce à init_auth_dir
    // Vérifier simplement si le répertoire existe, sinon le créer rapidement
    if(_waccess(g_auth_dir, 0) != 0) {
        // Créer le répertoire en une seule opération
        int result = SHCreateDirectoryExW(NULL, g_auth_dir, NULL);
        if (result != ERROR_SUCCESS && result != ERROR_ALREADY_EXISTS) {
            fprintf(stderr, "Erreur: Impossible de créer le répertoire: %ls\n", g_auth_dir);
            GtkWidget *dialog = gtk_message_dialog_new(NULL, 
                GTK_DIALOG_MODAL, 
                GTK_MESSAGE_ERROR, 
                GTK_BUTTONS_OK, 
                "Erreur: Impossible de créer le répertoire de destination.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return;
        }
    }
    
    // Copier le fichier d'authentification - Optimisé
    fprintf(stderr, "Info: Copie du fichier d'authentification\n");
    BOOL copy_success = FALSE;
    
    // Première tentative avec CopyFileW (plus rapide)
    if(CopyFileW(account->auth_file, dest_path, FALSE)) {
        copy_success = TRUE;
        fprintf(stderr, "Info: Fichier copié avec succès\n");
    } else {
        DWORD error = GetLastError();
        fprintf(stderr, "Erreur: Échec de la copie du fichier. Code d'erreur: %ld\n", error);
        
        // Essayer une autre méthode de copie si la première échoue
        FILE *source = _wfopen(account->auth_file, L"rb");
        if (!source) {
            fprintf(stderr, "Erreur: Impossible d'ouvrir le fichier source: %ls (Erreur: %d)\n", account->auth_file, errno);
            GtkWidget *dialog = gtk_message_dialog_new(NULL, 
                GTK_DIALOG_MODAL, 
                GTK_MESSAGE_ERROR, 
                GTK_BUTTONS_OK, 
                "Erreur: Impossible d'ouvrir le fichier source. Vérifiez les permissions.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return;
        }
        
        // Vérifier que le répertoire de destination existe
        if (_waccess(g_auth_dir, 0) != 0) {
            fprintf(stderr, "Info: Création du répertoire de destination: %ls\n", g_auth_dir);
            int result = SHCreateDirectoryExW(NULL, g_auth_dir, NULL);
            if (result != ERROR_SUCCESS && result != ERROR_ALREADY_EXISTS) {
                fprintf(stderr, "Erreur: Impossible de créer le répertoire de destination: %ls (Code: %d)\n", g_auth_dir, result);
                fclose(source);
                GtkWidget *dialog = gtk_message_dialog_new(NULL, 
                    GTK_DIALOG_MODAL, 
                    GTK_MESSAGE_ERROR, 
                    GTK_BUTTONS_OK, 
                    "Erreur: Impossible de créer le répertoire de destination.");
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                return;
            }
        }
        
        FILE *dest = _wfopen(dest_path, L"wb");
        if (!dest) {
            fprintf(stderr, "Erreur: Impossible d'ouvrir le fichier de destination: %ls (Erreur: %d)\n", dest_path, errno);
            fclose(source);
            GtkWidget *dialog = gtk_message_dialog_new(NULL, 
                GTK_DIALOG_MODAL, 
                GTK_MESSAGE_ERROR, 
                GTK_BUTTONS_OK, 
                "Erreur: Impossible de créer le fichier de destination. Vérifiez les permissions.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return;
        }
        
        // Copier le contenu du fichier
        // Utiliser un buffer plus grand pour améliorer les performances
        char buffer[8192];
        size_t bytes;
        BOOL write_error = FALSE;
        
        while((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
            if (fwrite(buffer, 1, bytes, dest) != bytes) {
                write_error = TRUE;
                break;
            }
        }
        
        fclose(source);
        fclose(dest);
        
        if (write_error) {
            fprintf(stderr, "Erreur: Problème d'écriture lors de la copie du fichier\n");
            GtkWidget *dialog = gtk_message_dialog_new(NULL, 
                GTK_DIALOG_MODAL, 
                GTK_MESSAGE_ERROR, 
                GTK_BUTTONS_OK, 
                "Erreur: Problème d'écriture lors de la copie du fichier.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return;
        }
        
        copy_success = TRUE;
        fprintf(stderr, "Info: Fichier copié avec succès en utilisant la méthode alternative\n");
    }
    
    // Vérifier que la copie a réussi
    if(!copy_success) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, 
            GTK_DIALOG_MODAL, 
            GTK_MESSAGE_ERROR, 
            GTK_BUTTONS_OK, 
            "Erreur: Le fichier d'authentification n'a pas pu être copié");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    // Attendre un peu pour s'assurer que le fichier est bien copié
    Sleep(500);

    // Vérifier que le fichier d'authentification a bien été copié
    if(_waccess(dest_path, 0) != 0) {
        GtkWidget *error_dialog = gtk_message_dialog_new(NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Erreur: Le fichier d'authentification n'a pas été trouvé après la copie");
        gtk_dialog_run(GTK_DIALOG(error_dialog));
        gtk_widget_destroy(error_dialog);
        return;
    }

    // Lancer le jeu via Steam avec ShellExecute
    fprintf(stderr, "Info: Lancement du jeu via Steam avec l'URL: steam://rungameid/1881700\n");
    
    // Vérifier que le fichier d'authentification est bien en place avant de lancer le jeu
    if(_waccess(dest_path, 0) != 0) {
        GtkWidget *error_dialog = gtk_message_dialog_new(NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Erreur: Le fichier d'authentification n'est pas disponible. Impossible de lancer le jeu.");
        gtk_dialog_run(GTK_DIALOG(error_dialog));
        gtk_widget_destroy(error_dialog);
        return;
    }
    
    // Lancer Steam avec l'ID du jeu en utilisant plusieurs méthodes pour assurer la compatibilité
    wchar_t steam_url[256] = L"steam://rungameid/1881700";
    BOOL launch_success = FALSE;
    
    // Vérifier si Steam est installé et disponible
    HKEY hKey;
    BOOL steam_installed = FALSE;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        steam_installed = TRUE;
        RegCloseKey(hKey);
        fprintf(stderr, "Info: Steam est installé sur le système\n");
    } else {
        fprintf(stderr, "Avertissement: Steam ne semble pas être installé\n");
    }
    
    // Méthode 1: Utiliser ShellExecute directement avec verb "open" explicite
    fprintf(stderr, "Info: Tentative de lancement via ShellExecuteW avec l'URL: %ls\n", steam_url);
    HINSTANCE result = ShellExecuteW(NULL, L"open", steam_url, NULL, NULL, SW_SHOWNORMAL);
    
    // Vérifier le résultat de ShellExecute
    if((INT_PTR)result > 32) {
        fprintf(stderr, "Info: Jeu lancé via ShellExecute\n");
        launch_success = TRUE;
    } else {
        fprintf(stderr, "Info: ShellExecute a échoué avec le code %d, essai avec d'autres méthodes...\n", (int)(INT_PTR)result);
        
        // Méthode 2: Essayer avec rundll32
        wchar_t command[512];
        swprintf(command, sizeof(command)/sizeof(command[0]), L"rundll32 url.dll,FileProtocolHandler %ls", steam_url);
        fprintf(stderr, "Info: Tentative de lancement via rundll32 avec la commande: %ls\n", command);
        
        STARTUPINFOW si = {0};
        PROCESS_INFORMATION pi = {0};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOWNORMAL;
        
        // Créer le processus pour lancer Steam via rundll32
        BOOL process_created = CreateProcessW(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        
        if(process_created) {
            // Fermer les handles du processus et du thread
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            fprintf(stderr, "Info: Jeu lancé via rundll32\n");
            launch_success = TRUE;
        } else {
            fprintf(stderr, "Info: Échec du lancement via rundll32, erreur: %ld\n", GetLastError());
            
            // Méthode 3: Essayer avec explorer.exe
            memset(&si, 0, sizeof(si));
            memset(&pi, 0, sizeof(pi));
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_SHOWNORMAL;
            
            swprintf(command, sizeof(command)/sizeof(command[0]), L"explorer.exe %ls", steam_url);
            fprintf(stderr, "Info: Tentative de lancement via explorer.exe avec la commande: %ls\n", command);
            
            process_created = CreateProcessW(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
            
            if(process_created) {
                // Fermer les handles du processus et du thread
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                fprintf(stderr, "Info: Jeu lancé via explorer.exe\n");
                launch_success = TRUE;
            } else {
                fprintf(stderr, "Info: Échec du lancement via explorer.exe, erreur: %ld\n", GetLastError());
            }
        }
    }
    
    if(!launch_success) {
        // Toutes les méthodes ont échoué
        GtkWidget *error_dialog;
        if (!steam_installed) {
            error_dialog = gtk_message_dialog_new(NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Erreur: Steam ne semble pas être installé sur votre système. Veuillez installer Steam pour pouvoir lancer le jeu.");
        } else {
            error_dialog = gtk_message_dialog_new(NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Erreur lors du lancement du jeu via Steam. Vérifiez que Steam est installé et en cours d'exécution.");
        }
        gtk_dialog_run(GTK_DIALOG(error_dialog));
        gtk_widget_destroy(error_dialog);
        return;
    }
    
    // Attendre plus longtemps pour s'assurer que Steam a le temps de démarrer
    Sleep(3000);
    
    // Vérifier une dernière fois que le fichier d'authentification est toujours présent
    if(_waccess(dest_path, 0) == 0) {
        // Vérifier que le fichier d'authentification correspond bien au compte sélectionné
        wchar_t expected_auth_file[1024];
        swprintf(expected_auth_file, sizeof(expected_auth_file)/sizeof(expected_auth_file[0]), L"%s\\AppData\\Local\\Toaa\\INTL\\28014\\Toaa-Win64-Shipping\\intl_auth.txt.%ls", _wgetenv(L"USERPROFILE"), account->name);
        
        // Comparer les contenus des fichiers pour s'assurer qu'il s'agit du bon compte
        FILE *src = _wfopen(account->auth_file, L"rb");
        FILE *dst = _wfopen(dest_path, L"rb");
        
        if(src && dst) {
            char src_buffer[1024] = {0};
            char dst_buffer[1024] = {0};
            size_t src_size = fread(src_buffer, 1, sizeof(src_buffer) - 1, src);
            size_t dst_size = fread(dst_buffer, 1, sizeof(dst_buffer) - 1, dst);
            
            fclose(src);
            fclose(dst);
            
            if(src_size == dst_size && memcmp(src_buffer, dst_buffer, src_size) == 0) {
                fprintf(stderr, "Info: Vérification réussie - Le fichier d'authentification correspond au compte sélectionné\n");
            } else {
                fprintf(stderr, "Avertissement: Le fichier d'authentification ne correspond pas au compte sélectionné\n");
                // Essayer de copier à nouveau le fichier
                if(CopyFileW(account->auth_file, dest_path, FALSE)) {
                    fprintf(stderr, "Info: Fichier d'authentification recopié avec succès\n");
                    // Attendre un peu pour s'assurer que le fichier est bien copié
                    Sleep(500);
                }
            }
        }
        
        fprintf(stderr, "Info: Fichier d'authentification toujours présent après lancement de Steam\n");
    } else {
        fprintf(stderr, "Avertissement: Le fichier d'authentification n'est plus présent après lancement de Steam\n");
        // Essayer de recréer le fichier
        if(CopyFileW(account->auth_file, dest_path, FALSE)) {
            fprintf(stderr, "Info: Fichier d'authentification recréé avec succès\n");
            // Attendre un peu pour s'assurer que le fichier est bien copié
            Sleep(500);
        }
    }
    
    // Vérifier une dernière fois que le fichier d'authentification est correct
    if(_waccess(dest_path, 0) == 0) {
        FILE *src = _wfopen(account->auth_file, L"rb");
        FILE *dst = _wfopen(dest_path, L"rb");
        
        if(src && dst) {
            char src_buffer[1024] = {0};
            char dst_buffer[1024] = {0};
            size_t src_size = fread(src_buffer, 1, sizeof(src_buffer) - 1, src);
            size_t dst_size = fread(dst_buffer, 1, sizeof(dst_buffer) - 1, dst);
            
            fclose(src);
            fclose(dst);
            
            if(src_size != dst_size || memcmp(src_buffer, dst_buffer, src_size) != 0) {
                // Forcer une dernière copie si les fichiers ne correspondent toujours pas
                if(CopyFileW(account->auth_file, dest_path, FALSE)) {
                    fprintf(stderr, "Info: Fichier d'authentification recopié une dernière fois\n");
                    Sleep(500);
                }
            }
        }
    }
    
    // Obtenir le nom du compte pour l'affichage
    gchar *name_utf8 = g_utf16_to_utf8((gunichar2*)account->name, -1, NULL, NULL, NULL);
    
    // Afficher un message de confirmation sans bloquer l'interface
    // Cela permet à Steam de continuer à se charger pendant que l'utilisateur lit le message
    GtkWidget *success_dialog = gtk_message_dialog_new(NULL,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "Compte %s sélectionné. Le jeu est en cours de lancement via Steam.",
        name_utf8 ? name_utf8 : "inconnu");
    gtk_dialog_run(GTK_DIALOG(success_dialog));
    gtk_widget_destroy(success_dialog);
    
    if(name_utf8) g_free(name_utf8);
}

// Rétablir les warnings
#pragma GCC diagnostic pop

void create_auth_file(const wchar_t *account_name) {
    // Initialiser le chemin d'authentification
    init_auth_dir();
    if (g_auth_dir[0] == 0) {
        fprintf(stderr, "Erreur: Impossible d'obtenir le chemin du profil utilisateur\n");
        return;
    }
    
    // Créer tous les répertoires parents nécessaires
    int result = SHCreateDirectoryExW(NULL, g_auth_dir, NULL);
    if (result != ERROR_SUCCESS && result != ERROR_ALREADY_EXISTS) {
        wchar_t error_msg[256];
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, result, 0, error_msg, 256, NULL);
        fprintf(stderr, "Erreur: Impossible de créer le répertoire avec SHCreateDirectoryExW: %ls (Code: %d - %ls)\n", g_auth_dir, result, error_msg);
        
        // Méthode alternative: créer les répertoires un par un
        fprintf(stderr, "Info: Tentative de création des répertoires un par un\n");
        
        // Extraire le chemin du profil utilisateur
        wchar_t *user_profile = _wgetenv(L"USERPROFILE");
        if (!user_profile) {
            fprintf(stderr, "Erreur: Impossible d'obtenir le chemin du profil utilisateur\n");
            return;
        }
        
        // Créer le chemin complet manuellement
        wchar_t base_path[1024];
        swprintf(base_path, sizeof(base_path)/sizeof(base_path[0]), L"%ls\\AppData", user_profile);
        _wmkdir(base_path);
        
        swprintf(base_path, sizeof(base_path)/sizeof(base_path[0]), L"%ls\\AppData\\Local", user_profile);
        _wmkdir(base_path);
        
        swprintf(base_path, sizeof(base_path)/sizeof(base_path[0]), L"%ls\\AppData\\Local\\Toaa", user_profile);
        _wmkdir(base_path);
        
        swprintf(base_path, sizeof(base_path)/sizeof(base_path[0]), L"%ls\\AppData\\Local\\Toaa\\INTL", user_profile);
        _wmkdir(base_path);
        
        swprintf(base_path, sizeof(base_path)/sizeof(base_path[0]), L"%ls\\AppData\\Local\\Toaa\\INTL\\28014", user_profile);
        _wmkdir(base_path);
        
        swprintf(base_path, sizeof(base_path)/sizeof(base_path[0]), L"%ls\\AppData\\Local\\Toaa\\INTL\\28014\\Toaa-Win64-Shipping", user_profile);
        if (_wmkdir(base_path) != 0 && errno != EEXIST) {
            wchar_t error_msg[256];
            _wcserror_s(error_msg, 256, errno);
            fprintf(stderr, "Erreur: Impossible de créer le répertoire final: %ls (Erreur: %ls)\n", base_path, error_msg);
            return;  // Sortir si on ne peut pas créer le répertoire
        } else {
            fprintf(stderr, "Info: Répertoire créé ou déjà existant: %ls\n", base_path);
        }
    } else {
        fprintf(stderr, "Info: Répertoire créé ou déjà existant: %ls\n", g_auth_dir);
    }
    
    // Vérifier que le répertoire a bien été créé
    if(_waccess(g_auth_dir, 0) != 0) {
        fprintf(stderr, "Erreur: Le répertoire n'a pas pu être créé malgré les tentatives: %ls\n", g_auth_dir);
        return;
    }
    
    // Chemin du fichier d'authentification source (intl_auth.txt)
    wchar_t source_path[1024];
    swprintf(source_path, sizeof(source_path)/sizeof(source_path[0]), L"%ls\\intl_auth.txt", g_auth_dir);
    
    // Chemin du fichier d'authentification pour le nouveau compte
    wchar_t new_auth_path[1024];
    swprintf(new_auth_path, sizeof(new_auth_path)/sizeof(new_auth_path[0]), L"%ls\\intl_auth.txt.%ls", g_auth_dir, account_name);
    
    // Vérifier que le répertoire parent du fichier existe
    wchar_t dir_path[1024];
    if (wcsrchr(new_auth_path, L'\\') != NULL) {
        wcsncpy(dir_path, new_auth_path, wcsrchr(new_auth_path, L'\\') - new_auth_path);
        dir_path[wcsrchr(new_auth_path, L'\\') - new_auth_path] = L'\0';
        
        // Vérifier si le répertoire existe, sinon le créer
        if (_waccess(dir_path, 0) != 0) {
            result = SHCreateDirectoryExW(NULL, dir_path, NULL);
            if (result != ERROR_SUCCESS && result != ERROR_ALREADY_EXISTS) {
                wchar_t error_msg[256];
                FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, result, 0, error_msg, 256, NULL);
                fprintf(stderr, "Erreur: Impossible de créer le répertoire pour le fichier: %ls (Code: %d - %ls)\n", dir_path, result, error_msg);
                return;
            } else {
                fprintf(stderr, "Info: Répertoire pour le fichier créé ou déjà existant: %ls\n", dir_path);
            }
        }
    }
    
    // Vérifier si le fichier source existe
    BOOL copy_success = FALSE;
    if (_waccess(source_path, 0) == 0) {
        // Copier le fichier source vers le nouveau fichier
        fprintf(stderr, "Info: Copie du fichier d'authentification existant\n");
        
        // Première tentative avec CopyFileW (plus rapide)
        if (CopyFileW(source_path, new_auth_path, FALSE)) {
            copy_success = TRUE;
            fprintf(stderr, "Info: Fichier copié avec succès\n");
        } else {
            DWORD error = GetLastError();
            fprintf(stderr, "Erreur: Échec de la copie du fichier. Code d'erreur: %ld\n", error);
            
            // Essayer une autre méthode de copie si la première échoue
            FILE *source = _wfopen(source_path, L"rb");
            if (!source) {
                fprintf(stderr, "Erreur: Impossible d'ouvrir le fichier source: %ls (Erreur: %d)\n", source_path, errno);
            } else {
                FILE *dest = _wfopen(new_auth_path, L"wb");
                if (!dest) {
                    fprintf(stderr, "Erreur: Impossible d'ouvrir le fichier de destination: %ls (Erreur: %d)\n", new_auth_path, errno);
                    fclose(source);
                } else {
                    // Copier le contenu du fichier
                    char buffer[8192];
                    size_t bytes;
                    BOOL write_error = FALSE;
                    
                    while((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
                        if (fwrite(buffer, 1, bytes, dest) != bytes) {
                            write_error = TRUE;
                            break;
                        }
                    }
                    
                    fclose(source);
                    fclose(dest);
                    
                    if (!write_error) {
                        copy_success = TRUE;
                        fprintf(stderr, "Info: Fichier copié avec succès en utilisant la méthode alternative\n");
                    } else {
                        fprintf(stderr, "Erreur: Problème d'écriture lors de la copie du fichier\n");
                    }
                }
            }
        }
    }
    
    // Si la copie a échoué ou si le fichier source n'existe pas, créer un fichier vide
    if (!copy_success) {
        fprintf(stderr, "Info: Création d'un nouveau fichier d'authentification vide\n");
        FILE *fp = _wfopen(new_auth_path, L"wb");
        if (fp) {
            fclose(fp);
            copy_success = TRUE;
            fprintf(stderr, "Info: Création d'un nouveau fichier d'authentification: %ls\n", new_auth_path);
        } else {
            wchar_t error_msg[256];
            _wcserror_s(error_msg, 256, errno);
            fprintf(stderr, "Erreur: Impossible de créer le fichier d'authentification: %ls (Erreur: %d - %ls)\n", new_auth_path, errno, error_msg);
            return;
        }
    }
    
    // Mettre à jour la structure Account correspondante si elle existe
    if (copy_success) {
        for(GList *iter = accounts; iter != NULL; iter = iter->next) {
            Account *account = iter->data;
            if (wcscmp(account->name, account_name) == 0) {
                // Libérer l'ancien chemin et mettre à jour avec le nouveau
                if (account->auth_file) {
                    free(account->auth_file);
                }
                account->auth_file = _wcsdup(new_auth_path);
                fprintf(stderr, "Info: Mise à jour du chemin du fichier d'authentification dans la structure Account\n");
                break;
            }
        }
    } else {
        fprintf(stderr, "Erreur: Impossible de créer ou copier le fichier d'authentification\n");
    }
}

void delete_auth_file(const wchar_t *account_name) {
    // Initialiser le chemin d'authentification
    init_auth_dir();
    if (g_auth_dir[0] == 0) {
        fprintf(stderr, "Erreur: Impossible d'obtenir le chemin du profil utilisateur\n");
        return;
    }
    
    wchar_t auth_path[1024];
    swprintf(auth_path, sizeof(auth_path)/sizeof(auth_path[0]), L"%ls\\intl_auth.txt.%ls", g_auth_dir, account_name);
    
    if(_waccess(auth_path, 0) == 0) {
        if(_wremove(auth_path) == 0) {
            fprintf(stderr, "Info: Suppression du fichier d'authentification: %ls\n", auth_path);
        } else {
            fprintf(stderr, "Erreur: Impossible de supprimer le fichier d'authentification: %ls\n", auth_path);
        }
    } else {
        fprintf(stderr, "Erreur: Le fichier d'authentification n'existe pas: %ls\n", auth_path);
    }
}

void on_delete_account(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    if(account_combo == NULL || accounts == NULL) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Aucun compte disponible");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(account_combo));
    if(active >= 0) {
        GList *item = g_list_nth(accounts, active);
        if(item != NULL) {
            Account *account = item->data;
            
            // Demander confirmation
            gchar *name_utf8 = g_utf16_to_utf8((gunichar2*)account->name, -1, NULL, NULL, NULL);
            GtkWidget *confirm_dialog = gtk_message_dialog_new(NULL, 
                GTK_DIALOG_MODAL, 
                GTK_MESSAGE_QUESTION, 
                GTK_BUTTONS_YES_NO, 
                "Êtes-vous sûr de vouloir supprimer le compte %s ?", 
                name_utf8);
            
            gint response = gtk_dialog_run(GTK_DIALOG(confirm_dialog));
            gtk_widget_destroy(confirm_dialog);
            
            if(response == GTK_RESPONSE_YES) {
                // Supprimer le fichier d'authentification
                delete_auth_file(account->name);
                
                // Supprimer le compte de la liste
                accounts = g_list_remove(accounts, account);
                free(account->name);
                free(account->auth_file);
                free(account);
                
                // Mettre à jour le combo box
                gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(account_combo), active);
                
                // Sélectionner le premier compte s'il y en a
                if(g_list_length(accounts) > 0) {
                    gtk_combo_box_set_active(GTK_COMBO_BOX(account_combo), 0);
                }
                
                GtkWidget *success_dialog = gtk_message_dialog_new(NULL, 
                    GTK_DIALOG_MODAL, 
                    GTK_MESSAGE_INFO, 
                    GTK_BUTTONS_OK, 
                    "Compte %s supprimé avec succès", 
                    name_utf8);
                gtk_dialog_run(GTK_DIALOG(success_dialog));
                gtk_widget_destroy(success_dialog);
            }
            
            g_free(name_utf8);
        }
    }
}



void on_update_account(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    // Initialiser le chemin d'authentification
    init_auth_dir();
    if (g_auth_dir[0] == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
            "Erreur: Impossible d'obtenir le chemin du profil utilisateur");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    if(account_combo == NULL || accounts == NULL) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Aucun compte disponible");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(account_combo));
    if(active < 0) return;

    GList *item = g_list_nth(accounts, active);
    if(item == NULL) return;

    Account *account = item->data;
    
    // Chemin du fichier source (intl_auth.txt)
    wchar_t src_path[1024];
    swprintf(src_path, sizeof(src_path)/sizeof(src_path[0]), L"%ls\\intl_auth.txt", g_auth_dir);
    
    // Vérifier que le fichier source existe
    if(_waccess(src_path, 0) != 0) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, 
            GTK_DIALOG_MODAL, 
            GTK_MESSAGE_ERROR, 
            GTK_BUTTONS_OK, 
            "Erreur: Le fichier d'authentification n'existe pas");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    // Chemin du fichier de destination (intl_auth.txt.NOM_DU_COMPTE)
    wchar_t dest_path[1024];
    swprintf(dest_path, sizeof(dest_path)/sizeof(dest_path[0]), L"%ls\\intl_auth.txt.%ls", g_auth_dir, account->name);
    
    // Supprimer le fichier de destination s'il existe déjà
    if(_waccess(dest_path, 0) == 0) {
        if(_wremove(dest_path) != 0) {
            fprintf(stderr, "Erreur: Impossible de supprimer le fichier d'authentification existant: %ls\n", dest_path);
            GtkWidget *dialog = gtk_message_dialog_new(NULL, 
                GTK_DIALOG_MODAL, 
                GTK_MESSAGE_ERROR, 
                GTK_BUTTONS_OK, 
                "Erreur: Impossible de supprimer le fichier d'authentification existant");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return;
        }
        fprintf(stderr, "Info: Fichier d'authentification existant supprimé: %ls\n", dest_path);
    }
    
    // Copier le fichier d'authentification
    fprintf(stderr, "Info: Copie du fichier d'authentification\n");
    BOOL copy_success = FALSE;
    
    // Première tentative avec CopyFileW (plus rapide)
    if(CopyFileW(src_path, dest_path, FALSE)) {
        copy_success = TRUE;
        fprintf(stderr, "Info: Fichier copié avec succès\n");
    } else {
        DWORD error = GetLastError();
        fprintf(stderr, "Erreur: Échec de la copie du fichier. Code d'erreur: %ld\n", error);
        
        // Essayer une autre méthode de copie si la première échoue
        FILE *source = _wfopen(src_path, L"rb");
        if (!source) {
            fprintf(stderr, "Erreur: Impossible d'ouvrir le fichier source: %ls (Erreur: %d)\n", src_path, errno);
            GtkWidget *dialog = gtk_message_dialog_new(NULL, 
                GTK_DIALOG_MODAL, 
                GTK_MESSAGE_ERROR, 
                GTK_BUTTONS_OK, 
                "Erreur: Impossible d'ouvrir le fichier source. Vérifiez les permissions.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return;
        }
        
        FILE *dest = _wfopen(dest_path, L"wb");
        if (!dest) {
            fprintf(stderr, "Erreur: Impossible d'ouvrir le fichier de destination: %ls (Erreur: %d)\n", dest_path, errno);
            fclose(source);
            GtkWidget *dialog = gtk_message_dialog_new(NULL, 
                GTK_DIALOG_MODAL, 
                GTK_MESSAGE_ERROR, 
                GTK_BUTTONS_OK, 
                "Erreur: Impossible de créer le fichier de destination. Vérifiez les permissions.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return;
        }
        
        // Copier le contenu du fichier
        char buffer[8192];
        size_t bytes;
        BOOL write_error = FALSE;
        
        while((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
            if (fwrite(buffer, 1, bytes, dest) != bytes) {
                write_error = TRUE;
                break;
            }
        }
        
        fclose(source);
        fclose(dest);
        
        if (write_error) {
            fprintf(stderr, "Erreur: Problème d'écriture lors de la copie du fichier\n");
            GtkWidget *dialog = gtk_message_dialog_new(NULL, 
                GTK_DIALOG_MODAL, 
                GTK_MESSAGE_ERROR, 
                GTK_BUTTONS_OK, 
                "Erreur: Problème d'écriture lors de la copie du fichier.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return;
        }
        
        copy_success = TRUE;
        fprintf(stderr, "Info: Fichier copié avec succès en utilisant la méthode alternative\n");
    }
    
    // Vérifier que la copie a réussi
    if(!copy_success) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, 
            GTK_DIALOG_MODAL, 
            GTK_MESSAGE_ERROR, 
            GTK_BUTTONS_OK, 
            "Erreur: Le fichier d'authentification n'a pas pu être copié");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    // Mettre à jour le chemin du fichier dans la structure Account
    free(account->auth_file);
    account->auth_file = _wcsdup(dest_path);
    
    // Obtenir le nom du compte pour l'affichage
    gchar *name_utf8 = g_utf16_to_utf8((gunichar2*)account->name, -1, NULL, NULL, NULL);
    
    // Afficher un message de confirmation
    GtkWidget *success_dialog = gtk_message_dialog_new(NULL,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "Compte %s mis à jour avec succès",
        name_utf8 ? name_utf8 : "inconnu");
    gtk_dialog_run(GTK_DIALOG(success_dialog));
    gtk_widget_destroy(success_dialog);
    
    if(name_utf8) g_free(name_utf8);
    
    // Recharger la liste des comptes
    load_accounts();
}

void on_create_account(GtkWidget *widget __attribute__((unused)), gpointer data __attribute__((unused))) {
    // Créer une boîte de dialogue pour saisir le nom du compte
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Créer un nouveau compte", 
        NULL, 
        GTK_DIALOG_MODAL, 
        "Annuler", GTK_RESPONSE_CANCEL, 
        "Créer", GTK_RESPONSE_ACCEPT, 
        NULL);
    
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    
    GtkWidget *label = gtk_label_new("Nom du compte :");
    gtk_container_add(GTK_CONTAINER(content_area), label);
    
    GtkWidget *entry = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(content_area), entry);
    
    gtk_widget_show_all(dialog);
    
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if(response == GTK_RESPONSE_ACCEPT) {
        const gchar *account_name = gtk_entry_get_text(GTK_ENTRY(entry));
        
        if(strlen(account_name) > 0) {
            // Convertir le nom du compte en wchar_t
            gunichar2 *account_name_utf16 = g_utf8_to_utf16(account_name, -1, NULL, NULL, NULL);
            
            // Créer le fichier d'authentification
            create_auth_file((wchar_t*)account_name_utf16);
            
            // Recharger la liste des comptes
            load_accounts();
            
            // Sélectionner le nouveau compte
            for(guint i = 0; i < g_list_length(accounts); i++) {
                GList *item = g_list_nth(accounts, i);
                if(item != NULL) {
                    Account *account = item->data;
                    gchar *name_utf8 = g_utf16_to_utf8((gunichar2*)account->name, -1, NULL, NULL, NULL);
                    
                    if(strcmp(name_utf8, account_name) == 0) {
                        gtk_combo_box_set_active(GTK_COMBO_BOX(account_combo), i);
                        g_free(name_utf8);
                        break;
                    }
                    
                    g_free(name_utf8);
                }
            }
            
            g_free(account_name_utf16);
            
            GtkWidget *success_dialog = gtk_message_dialog_new(NULL, 
                GTK_DIALOG_MODAL, 
                GTK_MESSAGE_INFO, 
                GTK_BUTTONS_OK, 
                "Compte %s créé avec succès", 
                account_name);
            gtk_dialog_run(GTK_DIALOG(success_dialog));
            gtk_widget_destroy(success_dialog);
        } else {
            GtkWidget *error_dialog = gtk_message_dialog_new(NULL, 
                GTK_DIALOG_MODAL, 
                GTK_MESSAGE_ERROR, 
                GTK_BUTTONS_OK, 
                "Le nom du compte ne peut pas être vide");
            gtk_dialog_run(GTK_DIALOG(error_dialog));
            gtk_widget_destroy(error_dialog);
        }
    }
    
    gtk_widget_destroy(dialog);
}

// Rétablir les warnings
#pragma GCC diagnostic pop