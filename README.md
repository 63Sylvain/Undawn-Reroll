# Undawn Reroll Manager

Cet outil permet de gérer facilement plusieurs comptes pour le jeu **Undawn** (Steam) sur PC. Il bascule rapidement entre les fichiers d'authentification sans avoir à se reconnecter manuellement à chaque fois.

## 🚀 Fonctionnalités

*   **Multi-comptes** : Basculez entre vos comptes en un clic.
*   **100 % Autonome (Win32 natif)** : Un seul exécutable `undawn_reroll.exe` sans aucune DLL externe requise (plus besoin de GTK3).
*   **Détection en direct** : Affiche le profil actuellement actif (`intl_auth.txt`) et avertit si le jeu (`Toaa-Win64-Shipping.exe`) est en cours d'exécution.
*   **Gestion complète** : Créer, mettre à jour (copie atomique sécurisée), renommer, supprimer et rafraîchir la liste des profils.
*   **Lancement Steam optionnel** : Case à cocher « Lancer le jeu via Steam » pour choisir de basculer avec ou sans démarrage immédiat du jeu.
*   **Menu Outils** : Accès direct au dossier des profils dans l'Explorateur et sauvegarde complète (Backup horodaté) de tous vos comptes.

## 📥 Installation

Téléchargez simplement `undawn_reroll.exe` (ou `UndawnReroll_OneFile.exe`) depuis l'onglet [Releases](https://github.com/63Sylvain/Undawn-Reroll/releases). Aucune installation ni bibliothèque externe n'est nécessaire.

## 📖 Utilisation

### 1. Lancement
Double-cliquez sur `undawn_reroll.exe`.

### 2. Détection
Au lancement, le programme analyse le dossier d'authentification du jeu (`%LOCALAPPDATA%\Toaa\INTL\28014\Toaa-Win64-Shipping`).
Vos comptes sauvegardés (`intl_auth.txt.*`) apparaissent triés dans le menu déroulant, et l'état du compte actif ainsi que du jeu s'affichent automatiquement.

### 3. Gestion des comptes

*   **Lancer** : Copie le profil sélectionné vers `intl_auth.txt` et lance le jeu via Steam (AppID 1881700) si la case « Lancer le jeu via Steam » est cochée.
*   **Créer** : Sauvegarde le fichier `intl_auth.txt` actuel sous un nouveau nom de profil.
*   **Mettre à jour** : Met à jour le profil sélectionné à partir du fichier `intl_auth.txt` actuel (de façon atomique pour ne jamais perdre l'ancienne sauvegarde en cas d'erreur).
*   **Renommer / Supprimer / Rafraîchir** : Renomme un profil existant, le supprime après confirmation, ou actualise la liste.
*   **Menu Outils** : Permet d'ouvrir directement le dossier des profils ou d'exporter une sauvegarde complète de tous les profils dans un dossier choisi.

## 🛠️ Compilation (Pour les développeurs)

Ce projet est écrit en C99 et utilise uniquement l'API Win32 standard.

### Prérequis
*   MSYS2 (avec MinGW 64-bit)
*   Paquets : `mingw-w64-x86_64-toolchain`, `make`

Dans un terminal MSYS2 (MINGW64) :

```
pacman -S --needed --noconfirm mingw-w64-x86_64-toolchain make
```

### Build
Pour compiler l'exécutable autonome :

```
make all
```

Pour générer également l'archive portable et `UndawnReroll_OneFile.exe` :

```
make onefile
```

---
**Note** : Ce logiciel n'est pas affilié à Level Infinite ou LightSpeed Studios. Utilisez-le à vos propres risques.
