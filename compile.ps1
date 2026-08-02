<#
.SYNOPSIS
    Compila aMule su Windows tramite la toolchain MSYS2 MINGW64.

.DESCRIPTION
    Equivalente PowerShell di scripts/compile.sh, adattato a Windows.

    aMule su Windows si compila con MSYS2/MINGW64 (e' la via usata dalla CI,
    vedi .github/workflows/ccpp.yml): le dipendenze -- wxWidgets, Crypto++,
    Boost, zlib -- arrivano dai pacchetti MSYS2. Questo script fa da
    ponte: rileva MSYS2, verifica il toolchain, e instrada cmake/ninja nella
    shell MINGW64 corretta.

    Di default compila il solo eseguibile monolitico amule.exe. Usa -All per
    costruire anche amuled, amulegui, amulecmd, amuleweb e le utility.

.PARAMETER Debug
    Compila in Debug invece che Release.

.PARAMETER Jobs
    Job paralleli. Default: numero di CPU logiche.

.PARAMETER Clean
    Elimina la directory di build e riconfigura da zero.

.PARAMETER All
    Abilita tutti i target (daemon, remote GUI, webserver, utility, test).

.PARAMETER Bootstrap
    Installa/aggiorna i pacchetti MSYS2 necessari prima di compilare.
    Da usare al primo giro o dopo un aggiornamento delle dipendenze.

.PARAMETER Run
    Avvia l'eseguibile prodotto al termine della compilazione.

.EXAMPLE
    .\compile.ps1
    Build Release di amule.exe.

.EXAMPLE
    .\compile.ps1 -Bootstrap
    Installa le dipendenze MSYS2, poi compila.

.EXAMPLE
    .\compile.ps1 -Debug -Clean -Jobs 8
    Riconfigura da zero e compila in Debug con 8 job.
#>

# Niente [CmdletBinding()]: aggiungerebbe i parametri comuni di PowerShell,
# fra cui -Debug, che andrebbe in conflitto con il nostro.
param(
    [switch]$Debug,
    [int]$Jobs = 0,
    [switch]$Clean,
    [switch]$All,
    [switch]$Bootstrap,
    [switch]$Run
)

$ErrorActionPreference = 'Stop'

# --------------------------------------------------------------------
# Contesto
# --------------------------------------------------------------------

# Come scripts/compile.sh, lo script si ancora alla root del repo. A
# differenza di quello non pretende che ci si trovi gia' dentro: si
# posiziona da solo, cosi' e' invocabile da qualunque directory.
$RepoRoot = $PSScriptRoot
if (-not (Test-Path (Join-Path $RepoRoot 'CMakeLists.txt'))) {
    throw "CMakeLists.txt non trovato in '$RepoRoot' - lo script deve stare nella root del repo aMule."
}

$BuildType = if ($Debug) { 'Debug' } else { 'Release' }
$BuildDir  = 'build'
if ($Jobs -le 0) { $Jobs = [int]$env:NUMBER_OF_PROCESSORS }
if ($Jobs -le 0) { $Jobs = 4 }

# --------------------------------------------------------------------
# Individuazione di MSYS2
# --------------------------------------------------------------------

$MsysRoot = $null
$candidates = @()
if ($env:MSYS2_ROOT) { $candidates += $env:MSYS2_ROOT }
$candidates += @('C:\msys64', 'C:\msys2', 'D:\msys64', "$env:LOCALAPPDATA\Programs\msys64")

foreach ($c in $candidates) {
    if ($c -and (Test-Path (Join-Path $c 'usr\bin\bash.exe'))) { $MsysRoot = $c; break }
}

if (-not $MsysRoot) {
    Write-Host ''
    Write-Host 'MSYS2 non trovato.' -ForegroundColor Red
    Write-Host ''
    Write-Host 'Installalo con:'
    Write-Host '    winget install --id MSYS2.MSYS2 --exact' -ForegroundColor Cyan
    Write-Host ''
    Write-Host 'Poi rilancia questo script con -Bootstrap per installare le dipendenze:'
    Write-Host '    .\compile.ps1 -Bootstrap' -ForegroundColor Cyan
    Write-Host ''
    Write-Host 'Se MSYS2 e'' installato altrove, indicalo con $env:MSYS2_ROOT.'
    exit 1
}

$Bash = Join-Path $MsysRoot 'usr\bin\bash.exe'
Write-Host "MSYS2:      $MsysRoot"
Write-Host "Build type: $BuildType"
Write-Host "Jobs:       $Jobs"

# --------------------------------------------------------------------
# Esecuzione comandi nella shell MINGW64
# --------------------------------------------------------------------

# I comandi passano da un file .sh temporaneo anziche' da `bash -lc "..."`:
# PowerShell e bash si contendono virgolette, backslash e '$', e le
# stringhe lunghe si corrompono in silenzio. Un file evita del tutto il
# problema.
function Invoke-Mingw {
    param(
        [Parameter(Mandatory)][string]$Script,
        [Parameter(Mandatory)][string]$Activity
    )

    $tmp = Join-Path ([System.IO.Path]::GetTempPath()) ("amule-build-{0}.sh" -f [guid]::NewGuid())
    # LF e UTF8 senza BOM: bash non digerisce ne' i CRLF ne' il BOM.
    $body = "set -euo pipefail`n" + $Script
    [System.IO.File]::WriteAllText($tmp, ($body -replace "`r`n", "`n"), (New-Object System.Text.UTF8Encoding $false))

    try {
        $prevMsystem = $env:MSYSTEM
        $prevChere   = $env:CHERE_INVOKING
        $env:MSYSTEM = 'MINGW64'
        $env:CHERE_INVOKING = '1'

        $msysTmp = ConvertTo-MsysPath $tmp
        & $Bash -l $msysTmp
        $rc = $LASTEXITCODE
    }
    finally {
        $env:MSYSTEM = $prevMsystem
        $env:CHERE_INVOKING = $prevChere
        Remove-Item $tmp -Force -ErrorAction SilentlyContinue
    }

    if ($rc -ne 0) {
        Write-Host ''
        Write-Host "$Activity - FALLITO (exit $rc)" -ForegroundColor Red
        exit $rc
    }
}

function ConvertTo-MsysPath {
    param([Parameter(Mandatory)][string]$Path)
    $full = [System.IO.Path]::GetFullPath($Path)
    $drive = $full.Substring(0, 1).ToLower()
    $rest = $full.Substring(2).Replace('\', '/')
    return "/$drive$rest"
}

$RepoRootMsys = ConvertTo-MsysPath $RepoRoot

# --------------------------------------------------------------------
# Bootstrap dipendenze (opzionale)
# --------------------------------------------------------------------

if ($Bootstrap) {
    Write-Host ''
    Write-Host '==> Installazione dipendenze MSYS2 (puo'' richiedere parecchi minuti)' -ForegroundColor Yellow

    # Set minimo per amule.exe, derivato da cmake_mingw_w64_deps in
    # .github/workflows/ccpp.yml. libgd / readline servono solo a cas e a
    # amulecmd/amuleweb: inclusi solo con -All per non scaricare
    # inutilmente.
    $packages = @(
        'mingw-w64-x86_64-toolchain'
        'mingw-w64-x86_64-cmake'
        'mingw-w64-x86_64-ninja'
        'mingw-w64-x86_64-make'
        'mingw-w64-x86_64-pkgconf'
        'mingw-w64-x86_64-boost'
        'mingw-w64-x86_64-crypto++'
        'mingw-w64-x86_64-wxwidgets3.2-msw'
        'mingw-w64-x86_64-zlib'
        'mingw-w64-x86_64-gettext'
        'mingw-w64-x86_64-python'
    )
    if ($All) {
        $packages += @(
            'mingw-w64-x86_64-libgd'
            'mingw-w64-x86_64-readline'
        )
    }

    # Il primo -Syuu puo' terminare la shell dopo aver aggiornato il core
    # di MSYS2: e' il comportamento documentato di pacman. Si rilancia una
    # seconda volta, e solo il secondo esito conta.
    Write-Host '--> aggiornamento sistema MSYS2 (pass 1/2)'
    $env:MSYSTEM = 'MINGW64'; $env:CHERE_INVOKING = '1'
    & $Bash -lc 'pacman -Syuu --noconfirm' | Out-Null

    Write-Host '--> aggiornamento sistema MSYS2 (pass 2/2)'
    Invoke-Mingw -Activity 'Aggiornamento MSYS2' -Script 'pacman -Syuu --noconfirm'

    Write-Host '--> installazione pacchetti'
    Invoke-Mingw -Activity 'Installazione dipendenze' -Script (
        "pacman -S --needed --noconfirm " + ($packages -join ' ')
    )
}

# --------------------------------------------------------------------
# Verifica toolchain
# --------------------------------------------------------------------

Write-Host ''
Write-Host '==> Verifica toolchain' -ForegroundColor Cyan
Invoke-Mingw -Activity 'Verifica toolchain' -Script @'
missing=""
for t in gcc g++ cmake ninja wx-config; do
    command -v "$t" >/dev/null || missing="$missing $t"
done
if [ -n "$missing" ]; then
    echo "Strumenti mancanti:$missing" >&2
    echo "Rilancia con -Bootstrap per installare le dipendenze." >&2
    exit 1
fi
printf '    %-10s %s\n' gcc "$(gcc -dumpversion)" cmake "$(cmake --version | head -1 | awk '{print $3}')" wx "$(wx-config --version)"
'@

# --------------------------------------------------------------------
# Configure
# --------------------------------------------------------------------

if ($Clean) {
    $abs = Join-Path $RepoRoot $BuildDir
    if (Test-Path $abs) {
        Write-Host ''
        Write-Host "==> Rimozione $BuildDir" -ForegroundColor Cyan
        Remove-Item $abs -Recurse -Force
    }
}

$cmakeFlags = @(
    "-DCMAKE_BUILD_TYPE=$BuildType"
    '-DBUILD_MONOLITHIC=YES'
)
if ($All) {
    $cmakeFlags += @(
        '-DBUILD_DAEMON=YES'
        '-DBUILD_REMOTEGUI=YES'
        '-DBUILD_AMULECMD=YES'
        '-DBUILD_WEBSERVER=YES'
        '-DBUILD_ED2K=YES'
        '-DBUILD_ALC=YES'
        '-DBUILD_ALCC=YES'
        '-DBUILD_WXCAS=YES'
        '-DBUILD_TESTING=YES'
    )
} else {
    $cmakeFlags += '-DBUILD_TESTING=NO'
}

$needConfigure = -not (Test-Path (Join-Path $RepoRoot "$BuildDir\build.ninja"))
if ($needConfigure) {
    Write-Host ''
    Write-Host '==> Configurazione CMake' -ForegroundColor Cyan
    # Generatore Ninja: e' quello che usa la CI su MINGW64. Nota che
    # packaging/windows/build.sh usa "MinGW Makefiles", ma per un motivo
    # specifico di CLANGARM64 (crash di ninja in try_compile), non
    # applicabile qui.
    Invoke-Mingw -Activity 'Configurazione CMake' -Script (
        "cd '$RepoRootMsys'`ncmake -B $BuildDir -G Ninja " + ($cmakeFlags -join ' ')
    )
} else {
    Write-Host ''
    Write-Host "==> Build gia' configurata in $BuildDir/ (usa -Clean per riconfigurare)" -ForegroundColor DarkGray
}

# --------------------------------------------------------------------
# Build
# --------------------------------------------------------------------

Write-Host ''
Write-Host '==> Compilazione' -ForegroundColor Cyan

# Un aMule in esecuzione tiene lockato il .exe e il link fallisce con
# "Permission denied". Capita di continuo iterando sulla stessa macchina.
#
# Solo i processi avviati DA questa build dir tengono lockato l'output del
# linker: una copia installata altrove (o un aMule di sistema) non c'entra
# nulla e chiuderla e' un effetto collaterale che nessuno ha chiesto. Il
# match e' quindi sul path dell'eseguibile, non sul nome del processo.
$BuildDirAbs = (Join-Path $RepoRoot $BuildDir).TrimEnd('\') + '\'
Get-Process amule, amuled, amulegui -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -and $_.Path.StartsWith($BuildDirAbs, [StringComparison]::OrdinalIgnoreCase) } |
    ForEach-Object {
        Write-Host "    chiudo $($_.ProcessName) (PID $($_.Id)) - gira da $BuildDir e terrebbe lockato l'exe" -ForegroundColor DarkGray
        Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue
    }

$sw = [Diagnostics.Stopwatch]::StartNew()
Invoke-Mingw -Activity 'Compilazione' -Script "cd '$RepoRootMsys'`ncmake --build $BuildDir -j$Jobs"
$sw.Stop()

# --------------------------------------------------------------------
# Esito
# --------------------------------------------------------------------

$exe = Join-Path $RepoRoot "$BuildDir\src\amule.exe"
if (-not (Test-Path $exe)) {
    Write-Host ''
    Write-Host "Build terminata ma $exe non esiste." -ForegroundColor Red
    exit 1
}

$sizeMb = [math]::Round((Get-Item $exe).Length / 1MB, 1)
Write-Host ''
Write-Host ("OK - {0} ({1} MB) in {2:mm\:ss}" -f $exe, $sizeMb, $sw.Elapsed) -ForegroundColor Green

Get-ChildItem (Join-Path $RepoRoot "$BuildDir\src") -Filter *.exe |
    Where-Object { $_.Name -ne 'amule.exe' } |
    ForEach-Object { Write-Host "     inoltre: $($_.FullName)" -ForegroundColor DarkGray }

# L'eseguibile linka le DLL di MINGW64, che non sono nel PATH di Windows:
# va avviato da una shell MINGW64, oppure da un albero installato in cui
# cmake ha copiato le DLL accanto all'exe (vedi src/CMakeLists.txt,
# blocco GET_RUNTIME_DEPENDENCIES).
Write-Host ''
Write-Host 'Per avviarlo:' -ForegroundColor Yellow
Write-Host "    .\compile.ps1 -Run" -ForegroundColor Cyan
Write-Host '  oppure, per un albero autonomo con le DLL incluse:'
Write-Host "    cmake --install $BuildDir --prefix .\amule-portable-x64" -ForegroundColor Cyan

if ($Run) {
    Write-Host ''
    Write-Host '==> Avvio amule.exe' -ForegroundColor Cyan
    $env:MSYSTEM = 'MINGW64'; $env:CHERE_INVOKING = '1'
    & $Bash -lc "cd '$RepoRootMsys' && ./$BuildDir/src/amule.exe &" | Out-Null
}
