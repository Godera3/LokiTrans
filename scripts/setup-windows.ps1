param(
    [switch]$SkipWinget,
    [switch]$SkipMsysPackages
)

$ErrorActionPreference = "Stop"

function Test-CommandExists {
    param([string]$Name)
    return $null -ne (Get-Command $Name -ErrorAction SilentlyContinue)
}

function Install-WithWinget {
    param(
        [string]$Id,
        [string]$Label
    )

    Write-Host "Installing $Label ($Id)..."
    winget install --id $Id --exact --accept-package-agreements --accept-source-agreements --silent
}

Write-Host "=== Svanipp Windows Setup ==="

if (-not $SkipWinget) {
    if (-not (Test-CommandExists "winget")) {
        throw "winget was not found. Install App Installer from Microsoft Store, then run this script again."
    }

    Install-WithWinget -Id "Kitware.CMake" -Label "CMake"
    Install-WithWinget -Id "MSYS2.MSYS2" -Label "MSYS2"
}

$msysBash = "C:\msys64\usr\bin\bash.exe"
if (-not (Test-Path $msysBash)) {
    throw "MSYS2 bash not found at C:\msys64\usr\bin\bash.exe. Install MSYS2 first."
}

if (-not $SkipMsysPackages) {
    Write-Host "Installing MSYS2 UCRT64 build tools..."
    & $msysBash -lc "pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-cmake"
}

Write-Host ""
Write-Host "Setup complete."
Write-Host "Next step: .\scripts\build-windows.ps1"
