param(
    [string]$OutputDir = "build"
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$packageRoot = Join-Path $env:LOCALAPPDATA "Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe"
$toolchainDir = Get-ChildItem $packageRoot -Directory -Filter "llvm-mingw-*-ucrt-x86_64" | Sort-Object Name -Descending | Select-Object -First 1
if ($null -eq $toolchainDir) {
    throw "LLVM-MinGW package directory not found. Install package MartinStorsjo.LLVM-MinGW.UCRT first."
}

$toolRoot = Join-Path $toolchainDir.FullName "bin"
$clang = Join-Path $toolRoot "clang++.exe"
$rc = Join-Path $toolRoot "llvm-rc.exe"
$strip = Join-Path $toolRoot "llvm-strip.exe"

if (-not (Test-Path $clang)) {
    throw "LLVM-MinGW toolchain not found. Install package MartinStorsjo.LLVM-MinGW.UCRT first."
}

$buildDir = Join-Path $projectRoot $OutputDir
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

& $rc "/fo" (Join-Path $buildDir "app.res") (Join-Path $projectRoot "resources\app.rc")

& $clang `
    "-std=c++17" `
    "-O2" `
    "-municode" `
    "-mwindows" `
    "-static-libstdc++" `
    "-static-libgcc" `
    "-DUNICODE" `
    "-D_UNICODE" `
    "-DWIN32_LEAN_AND_MEAN" `
    "-DNOMINMAX" `
    "-finput-charset=UTF-8" `
    "-o" (Join-Path $buildDir "EasySW.exe") `
    (Join-Path $projectRoot "src\main.cpp") `
    (Join-Path $projectRoot "src\App.cpp") `
    (Join-Path $projectRoot "src\SettingsStore.cpp") `
    (Join-Path $projectRoot "src\StartupManager.cpp") `
    (Join-Path $projectRoot "src\TextInteractor.cpp") `
    (Join-Path $projectRoot "src\TextTransform.cpp") `
    (Join-Path $projectRoot "src\UiAutomationTextProvider.cpp") `
    (Join-Path $buildDir "app.res") `
    "-ladvapi32" `
    "-lole32" `
    "-loleaut32" `
    "-lshell32" `
    "-lcomctl32" `
    "-luiautomationcore" `
    "-luuid"

& $strip (Join-Path $buildDir "EasySW.exe")
Get-Item (Join-Path $buildDir "EasySW.exe")
