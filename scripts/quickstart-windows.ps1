$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

Write-Host "Running dependency setup..."
& .\scripts\setup-windows.ps1

Write-Host ""
Write-Host "Building project..."
& .\scripts\build-windows.ps1 -BuildType Release

Write-Host ""
Write-Host "Running readiness check..."
& .\build\svanipp.exe check
