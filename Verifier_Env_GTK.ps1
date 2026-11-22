param(
    [switch]$CreerRaccourci,
    [switch]$NoPause
)

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

function Normalize-DirPath {
    param([string]$PathValue)
    if ([string]::IsNullOrWhiteSpace($PathValue)) { return '' }
    $p = $PathValue.Trim().Trim('"').Replace('/', '\')
    while ($p.Length -gt 3 -and $p.EndsWith('\')) {
        $p = $p.Substring(0, $p.Length - 1)
    }
    return $p
}

function Split-PathEntries {
    param([string]$RawPath)
    if ([string]::IsNullOrWhiteSpace($RawPath)) { return @() }
    return @($RawPath -split ';' | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne '' })
}

function Test-GtkDllDir {
    param([string]$DirPath)
    if ([string]::IsNullOrWhiteSpace($DirPath)) { return $false }
    try {
        $dll = [System.IO.Path]::Combine($DirPath, 'libgtk-3-0.dll')
        return [System.IO.File]::Exists($dll)
    } catch {
        return $false
    }
}

function Test-IsSubPathOf {
    param([string]$ChildPath, [string]$ParentPath)
    $c = (Normalize-DirPath $ChildPath).ToLowerInvariant()
    $p = (Normalize-DirPath $ParentPath).ToLowerInvariant()
    if ([string]::IsNullOrEmpty($c) -or [string]::IsNullOrEmpty($p)) { return $false }
    return ($c -eq $p) -or ($c.StartsWith($p + '\'))
}

function Get-RegistryPathInfo {
    param([ValidateSet('User', 'Machine')][string]$Scope)
    $key = $null
    try {
        if ($Scope -eq 'User') {
            $key = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey('Environment', $false)
        } else {
            $key = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey('SYSTEM\CurrentControlSet\Control\Session Manager\Environment', $false)
        }
        if ($null -eq $key) {
            return [pscustomobject]@{ Exists = $false; Raw = ''; Kind = [Microsoft.Win32.RegistryValueKind]::ExpandString }
        }
        $val = $key.GetValue('Path', $null, [Microsoft.Win32.RegistryValueOptions]::DoNotExpandEnvironmentNames)
        if ($null -eq $val) {
            return [pscustomobject]@{ Exists = $false; Raw = ''; Kind = [Microsoft.Win32.RegistryValueKind]::ExpandString }
        }
        $kind = $key.GetValueKind('Path')
        return [pscustomobject]@{ Exists = $true; Raw = [string]$val; Kind = $kind }
    } finally {
        if ($null -ne $key) { $key.Dispose() }
    }
}

function Set-UserPathRaw {
    param(
        [string]$NewRawPath,
        [Microsoft.Win32.RegistryValueKind]$PreferredKind = [Microsoft.Win32.RegistryValueKind]::ExpandString
    )
    $key = $null
    try {
        $key = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey('Environment', $true)
        if ($null -eq $key) {
            $key = [Microsoft.Win32.Registry]::CurrentUser.CreateSubKey('Environment')
        }
        $kind = $PreferredKind
        if ($NewRawPath -match '%[^%]+%') {
            $kind = [Microsoft.Win32.RegistryValueKind]::ExpandString
        }
        $key.SetValue('Path', $NewRawPath, $kind)
    } finally {
        if ($null -ne $key) { $key.Dispose() }
    }
}

function Broadcast-EnvironmentChange {
    try {
        if (-not ('Win32.EnvBroadcast' -as [type])) {
            Add-Type -Namespace 'Win32' -Name 'EnvBroadcast' -MemberDefinition @'
[System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
public static extern System.IntPtr SendMessageTimeoutW(
    System.IntPtr hWnd,
    uint Msg,
    System.UIntPtr wParam,
    string lParam,
    uint fuFlags,
    uint uTimeout,
    out System.UIntPtr lpdwResult);
'@
        }
        $result = [System.UIntPtr]::Zero
        [void][Win32.EnvBroadcast]::SendMessageTimeoutW(
            [IntPtr]0xffff,
            0x001A,
            [System.UIntPtr]::Zero,
            'Environment',
            0x0002,
            5000,
            [ref]$result
        )
        return $true
    } catch {
        return $false
    }
}

function Test-CleanPathContains {
    param([string]$RawPath, [string]$TargetDir)
    $targetNorm = Normalize-DirPath $TargetDir
    foreach ($entry in (Split-PathEntries $RawPath)) {
        # Une entree propre ne doit pas contenir de guillemets ni de slash '/'
        if ($entry -match '["/]') { continue }
        $expanded = [Environment]::ExpandEnvironmentVariables($entry)
        if ((Normalize-DirPath $expanded) -ieq $targetNorm) {
            return $true
        }
    }
    return $false
}

function Find-MalformedEntries {
    param([string]$RawPath, [string]$TargetDir)
    $targetNorm = Normalize-DirPath $TargetDir
    $bad = @()
    foreach ($entry in (Split-PathEntries $RawPath)) {
        $expanded = [Environment]::ExpandEnvironmentVariables($entry)
        if (((Normalize-DirPath $expanded) -ieq $targetNorm) -and ($entry -match '["/]')) {
            $bad += $entry
        }
    }
    return $bad
}

function New-DesktopShortcut {
    param([string]$ProjectDir)
    $desktopDir = [Environment]::GetFolderPath('Desktop')
    $batPath = Join-Path $ProjectDir 'Verifier_Env_GTK.bat'
    $icoPath = Join-Path $ProjectDir 'unnamed.ico'
    $lnkPath = Join-Path $desktopDir 'Verifier Env GTK (Undawn Reroll).lnk'

    $shell = New-Object -ComObject WScript.Shell
    $shortcut = $shell.CreateShortcut($lnkPath)
    $shortcut.TargetPath = $batPath
    $shortcut.WorkingDirectory = $ProjectDir
    $shortcut.Description = 'Vérifie et configure le PATH Windows pour GTK3 (Undawn Reroll)'
    if (Test-Path -LiteralPath $icoPath -PathType Leaf) {
        $shortcut.IconLocation = "$icoPath,0"
    }
    $shortcut.Save()
    Write-Host "[OK] Raccourci créé sur le Bureau : $lnkPath" -ForegroundColor Green
}

$projectRoot = Normalize-DirPath $PSScriptRoot

Write-Host '====================================================================' -ForegroundColor Cyan
Write-Host '  Vérification & Configuration GTK3 (PATH) - Undawn Reroll' -ForegroundColor Cyan
Write-Host '====================================================================' -ForegroundColor Cyan
Write-Host ''

if ($CreerRaccourci) {
    New-DesktopShortcut -ProjectDir $projectRoot
    Write-Host ''
}

# 1. Lire les variables PATH actuelles (sans corrompre les %VAR%)
$userPathInfo = Get-RegistryPathInfo -Scope 'User'
$machinePathInfo = Get-RegistryPathInfo -Scope 'Machine'

# 2. Rechercher les installations GTK3/MSYS2 par defaut (hors dossier du projet)
$defaultCandidates = @(
    'C:\msys64\mingw64\bin',
    'C:\msys64\ucrt64\bin',
    'C:\msys64\clang64\bin',
    'C:\msys64\mingw32\bin',
    'D:\msys64\mingw64\bin',
    'D:\msys64\ucrt64\bin',
    'E:\msys64\mingw64\bin',
    'F:\msys64\mingw64\bin',
    (Join-Path $env:USERPROFILE 'msys64\mingw64\bin'),
    (Join-Path $env:USERPROFILE 'msys64\ucrt64\bin'),
    'C:\Program Files\GTK3-Runtime Win64\bin',
    'C:\gtk\bin',
    'C:\gtk3\bin'
)

# Ajouter aussi tout dossier deja present dans PATH (hors projet) contenant libgtk-3-0.dll
$existingPathDirs = @()
foreach ($entry in ((Split-PathEntries $userPathInfo.Raw) + (Split-PathEntries $machinePathInfo.Raw))) {
    $expanded = Normalize-DirPath ([Environment]::ExpandEnvironmentVariables($entry))
    if (-not [string]::IsNullOrWhiteSpace($expanded)) {
        $existingPathDirs += $expanded
    }
}

$foundSystemDirs = New-Object System.Collections.Generic.List[string]
$seen = @{}

foreach ($cand in ($defaultCandidates + $existingPathDirs)) {
    $norm = Normalize-DirPath $cand
    if ([string]::IsNullOrWhiteSpace($norm)) { continue }
    $key = $norm.ToLowerInvariant()
    if ($seen.ContainsKey($key)) { continue }
    $seen[$key] = $true

    if (Test-IsSubPathOf -ChildPath $norm -ParentPath $projectRoot) { continue }
    if (Test-GtkDllDir -DirPath $norm) {
        $foundSystemDirs.Add($norm)
    }
}

Write-Host '[1/4] Recherche de libgtk-3-0.dll (installation système par défaut)...' -ForegroundColor White

$selectedGtkDir = $null
if ($foundSystemDirs.Count -gt 0) {
    $selectedGtkDir = $foundSystemDirs[0]
    foreach ($d in $foundSystemDirs) {
        $marker = if ($d -ieq $selectedGtkDir) { ' (sélectionné)' } else { '' }
        Write-Host "  [TROUVÉ] $d$marker" -ForegroundColor Green
    }
} else {
    Write-Host '  [AVERTISSEMENT] Aucune installation MSYS2/GTK3 par défaut détectée.' -ForegroundColor Yellow
    $releaseDir = Join-Path $projectRoot 'Release\UndawnReroll'
    if (Test-GtkDllDir -DirPath $releaseDir) {
        Write-Host "  Un dossier portable complet existe dans le projet : $releaseDir" -ForegroundColor Yellow
        if (-not $NoPause) {
            $choice = Read-Host '  Voulez-vous saisir un chemin personnalisé, taper P pour utiliser Release\UndawnReroll, ou Entrée pour annuler'
            if ($choice -ieq 'P') {
                $selectedGtkDir = Normalize-DirPath $releaseDir
            } elseif (-not [string]::IsNullOrWhiteSpace($choice)) {
                $customNorm = Normalize-DirPath $choice
                if (Test-GtkDllDir -DirPath $customNorm) {
                    $selectedGtkDir = $customNorm
                } else {
                    Write-Host "  [ERREUR] libgtk-3-0.dll introuvable dans : $customNorm" -ForegroundColor Red
                }
            }
        }
    } else {
        Write-Host '  Installez MSYS2 (C:\msys64) puis le paquet mingw-w64-x86_64-gtk3.' -ForegroundColor Yellow
    }
}

if ([string]::IsNullOrWhiteSpace($selectedGtkDir)) {
    Write-Host ''
    Write-Host '[ARRÊT] Aucun dossier GTK3 valide sélectionné. Aucune variable modifiée.' -ForegroundColor Red
    if (-not $NoPause) {
        Write-Host ''
        Read-Host 'Appuyez sur Entrée pour fermer'
    }
    exit 1
}

# 3. Vérification des DLLs principales dans le dossier retenu
Write-Host ''
Write-Host "[2/4] Vérification des DLLs GTK3 essentielles dans : $selectedGtkDir" -ForegroundColor White
$coreDlls = @(
    'libgtk-3-0.dll',
    'libgdk-3-0.dll',
    'libglib-2.0-0.dll',
    'libgobject-2.0-0.dll',
    'libcairo-2.dll',
    'libpango-1.0-0.dll',
    'libgdk_pixbuf-2.0-0.dll'
)
$missingCore = @($coreDlls | Where-Object { -not (Test-Path -LiteralPath (Join-Path $selectedGtkDir $_) -PathType Leaf) })
if ($missingCore.Count -eq 0) {
    Write-Host '  [OK] Toutes les DLLs GTK3 essentielles sont présentes.' -ForegroundColor Green
} else {
    Write-Host ("  [AVERTISSEMENT] DLLs manquantes : " + ($missingCore -join ', ')) -ForegroundColor Yellow
}

# 4. Diagnostic et mise à jour du PATH
Write-Host ''
Write-Host '[3/4] Vérification des variables d''environnement PATH...' -ForegroundColor White

$inUserClean = Test-CleanPathContains -RawPath $userPathInfo.Raw -TargetDir $selectedGtkDir
$inMachineClean = Test-CleanPathContains -RawPath $machinePathInfo.Raw -TargetDir $selectedGtkDir
$badUserEntries = Find-MalformedEntries -RawPath $userPathInfo.Raw -TargetDir $selectedGtkDir

$modified = $false

if ($inMachineClean) {
    Write-Host "  [OK] Déjà présent dans le PATH Système : $selectedGtkDir" -ForegroundColor Green
}
if ($inUserClean) {
    Write-Host "  [OK] Déjà présent dans le PATH Utilisateur : $selectedGtkDir" -ForegroundColor Green
}

if ($badUserEntries.Count -gt 0) {
    Write-Host ("  [ATTENTION] Entrée(s) mal formatée(s) détectée(s) dans le PATH Utilisateur : " + ($badUserEntries -join ', ')) -ForegroundColor Yellow
}

if ((-not $inUserClean -and -not $inMachineClean) -or ($badUserEntries.Count -gt 0)) {
    # Sauvegarde de securite de l'ancien PATH Utilisateur
    $stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
    $backupFile = Join-Path $env:TEMP "backup_user_path_undawn_reroll_$stamp.txt"
    Set-Content -LiteralPath $backupFile -Value $userPathInfo.Raw -Encoding UTF8
    Write-Host "  [INFO] Sauvegarde de l'ancien PATH Utilisateur : $backupFile" -ForegroundColor DarkGray

    $entries = New-Object System.Collections.Generic.List[string]
    foreach ($e in (Split-PathEntries $userPathInfo.Raw)) {
        $expanded = Normalize-DirPath ([Environment]::ExpandEnvironmentVariables($e))
        if (($expanded -ieq $selectedGtkDir) -and ($e -match '["/]')) {
            continue
        }
        $entries.Add($e)
    }

    if (-not $inUserClean -and -not $inMachineClean) {
        $entries.Add($selectedGtkDir)
    }

    $newUserPath = ($entries -join ';')
    Set-UserPathRaw -NewRawPath $newUserPath -PreferredKind $userPathInfo.Kind
    $modified = $true
    Write-Host "  [CORRIGÉ] Ajout/nettoyage effectué dans le PATH Utilisateur : $selectedGtkDir" -ForegroundColor Green
}

# Mettre a jour la session en cours et notifier Windows (Explorer, nouveaux processus)
if (-not (Test-CleanPathContains -RawPath $env:Path -TargetDir $selectedGtkDir)) {
    $env:Path = ($env:Path.TrimEnd(';') + ';' + $selectedGtkDir)
}

$notified = Broadcast-EnvironmentChange
if ($notified) {
    if ($modified) {
        Write-Host '  [OK] Windows a été notifié (WM_SETTINGCHANGE) : actif immédiatement sans redémarrer.' -ForegroundColor Green
    } else {
        Write-Host '  [OK] Rafraîchissement Windows (WM_SETTINGCHANGE) envoyé.' -ForegroundColor DarkGray
    }
} else {
    Write-Host '  [INFO] Fermez puis rouvrez votre session Windows si le changement ne prend pas effet tout de suite.' -ForegroundColor Yellow
}

# 5. Diagnostic bonus du dossier projet (alerte DLLs locales incompletes)
Write-Host ''
Write-Host '[4/4] Vérification de cohérence du dossier du projet...' -ForegroundColor White

$rootDllCount = @(Get-ChildItem -LiteralPath $projectRoot -Filter '*.dll' -File -ErrorAction SilentlyContinue).Count
$releaseDirCheck = Join-Path $projectRoot 'Release\UndawnReroll'
$releaseDllCount = 0
if (Test-Path -LiteralPath $releaseDirCheck -PathType Container) {
    $releaseDllCount = @(Get-ChildItem -LiteralPath $releaseDirCheck -Filter '*.dll' -File -ErrorAction SilentlyContinue).Count
}

if ($rootDllCount -gt 0 -and $releaseDllCount -gt $rootDllCount) {
    Write-Host "  [INFO] La racine du projet contient $rootDllCount DLLs alors que Release\UndawnReroll en contient $releaseDllCount." -ForegroundColor Yellow
    Write-Host '         Si vous lancez undawn_reroll.exe depuis la racine, Windows charge d''abord les DLLs locales' -ForegroundColor Yellow
    Write-Host '         (potentiellement incomplètes ou d''une autre version que MSYS2).' -ForegroundColor Yellow
} else {
    Write-Host "  [OK] Dossier projet vérifié ($rootDllCount DLL(s) à la racine, $releaseDllCount dans Release\UndawnReroll)." -ForegroundColor Green
}

# Test final de resolution via PATH
$whereHit = $null
foreach ($pEntry in (Split-PathEntries $env:Path)) {
    $exp = Normalize-DirPath ([Environment]::ExpandEnvironmentVariables($pEntry))
    if (Test-GtkDllDir -DirPath $exp) {
        $whereHit = Join-Path $exp 'libgtk-3-0.dll'
        break
    }
}

Write-Host ''
if ($whereHit) {
    Write-Host '====================================================================' -ForegroundColor Green
    Write-Host "  RÉSUMÉ : OK - libgtk-3-0.dll résolue via PATH :" -ForegroundColor Green
    Write-Host "  $whereHit" -ForegroundColor Green
    Write-Host '====================================================================' -ForegroundColor Green
} else {
    Write-Host '====================================================================' -ForegroundColor Red
    Write-Host '  RÉSUMÉ : ATTENTION - libgtk-3-0.dll non résolue dans le PATH.' -ForegroundColor Red
    Write-Host '====================================================================' -ForegroundColor Red
}

if (-not $NoPause) {
    Write-Host ''
    Read-Host 'Appuyez sur Entrée pour fermer cette fenêtre'
}
