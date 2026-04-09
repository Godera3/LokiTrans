param(
    [ValidateSet("Debug", "Release")]
    [string]$BuildType = "Release"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

# Ensure build exists
if (-not (Test-Path ".\build\svanipp.exe")) {
    Write-Host "Build not found; building..."
    & .\scripts\build-windows.ps1 -BuildType $BuildType
}

# Create distribution folder
$dist = Join-Path $repoRoot "dist"
if (Test-Path $dist) { Remove-Item $dist -Recurse -Force }
New-Item -ItemType Directory -Path $dist | Out-Null

Copy-Item -Path ".\build\svanipp.exe" -Destination (Join-Path $dist "svanipp.exe") -Force

# Optionally include README and LICENSE if presentnif (Test-Path ".\README.md") { Copy-Item .\README.md -Destination $dist -Force }
if (Test-Path ".\LICENSE") { Copy-Item .\LICENSE -Destination $dist -Force }

# Zip distributionn$zipPath = Join-Path $repoRoot "dist\svanipp-windows.zip"
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path "$dist\*" -DestinationPath $zipPath

Write-Host "Created: $zipPath"
Write-Host "Also available: $dist\svanipp.exe"
