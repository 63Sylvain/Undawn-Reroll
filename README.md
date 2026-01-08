# Undawn Reroll Manager

Cet outil permet de gérer facilement plusieurs comptes pour le jeu **Undawn** (Steam) sur PC. Il bascule rapidement entre les fichiers d'authentification sans avoir à se reconnecter manuellement à chaque fois.

## 🚀 Fonctionnalités

*   **Multi-comptes** : Basculez entre vos comptes en un clic.
*   **Lancement automatique** : Lance le jeu via Steam après le changement de compte.
*   **Portable** : Fonctionne sans installation (version OneFile).

## 📥 Installation

Téléchargez simplement la dernière version (`UndawnReroll_OneFile.exe`) depuis l'onglet [Releases](https://github.com/63Sylvain/Undawn-Reroll/releases).

## 📖 Utilisation

### 1. Lancement
Double-cliquez sur `UndawnReroll_OneFile.exe`.

### 2. Détection
Au lancement, le programme analyse le dossier d'authentification du jeu (généralement `C:\Users\[User]\AppData\Local\Toaa\INTL\28014\Toaa-Win64-Shipping`).
Vos comptes sauvegardés apparaîtront dans le menu déroulant.

### 3. Gestion des comptes

#### ➤ Lancer un compte
1.  Sélectionnez un compte dans la liste.
2.  Cliquez sur **Lancer**.
    *   *L'outil remplace le fichier d'auth actuel et lance le jeu via Steam (AppID 1881700).*

#### ➤ Créer un nouveau profil
1.  Connectez-vous à un nouveau compte dans le jeu, puis fermez le jeu.
2.  Dans l'outil, cliquez sur **Créer**.
3.  Donnez un nom (ex: "MonSmurf", "ComptePrincipal").
    *   *L'outil sauvegarde le fichier d'auth actuel sous ce nom.*

#### ➤ Mettre à jour un profil
Si vous avez joué sur un compte et que vous voulez mettre à jour ses données sauvegardées :
1.  Sélectionnez le nom du compte dans la liste.
2.  Cliquez sur **Mettre à jour**.
    *   *L'outil écrase la sauvegarde de ce profil avec les données actuelles du jeu.*

#### ➤ Supprimer
Sélectionnez un compte et cliquez sur **Supprimer** pour l'effacer de la liste.

## 🛠️ Compilation (Pour les développeurs)

Ce projet est écrit en C et utilise la bibliothèque GTK+ 3.

### Prérequis
*   MSYS2 (avec MinGW 64-bit)
*   Paquets : `mingw-w64-x86_64-gtk3`, `mingw-w64-x86_64-toolchain`, `make`

Dans un terminal MSYS2 (MINGW64), installez/mettez à jour:

```
pacman -Syu --noconfirm
pacman -S --needed --noconfirm mingw-w64-x86_64-toolchain mingw-w64-x86_64-gtk3 make
```

### Build
Pour générer l'exécutable unique avec MSYS2 :
1. Ouvrez un terminal MSYS2 (MINGW64).
2. Exécutez :

```
make onefile
```

Cela compile le binaire, copie les DLLs, crée l'archive ZIP et construit l'exécutable autonome.

---
**Note** : Ce logiciel n'est pas affilié à Level Infinite ou LightSpeed Studios. Utilisez-le à vos propres risques.
