# Fortune Summoners (English SE) - Japanese Voice Patch : uninstaller.
# Removes the voice mod; keeps the mod loader if other mods remain. ASCII-only.
$ErrorActionPreference = 'SilentlyContinue'
function Step($m){ Write-Host "[*] $m" -ForegroundColor Cyan }
function Good($m){ Write-Host "[+] $m" -ForegroundColor Green }
function Warn($m){ Write-Host "[!] $m" -ForegroundColor Yellow }

Write-Host ""
Write-Host "  Uninstall: Fortune Summoners EN-SE Japanese Voice Patch" -ForegroundColor Magenta
Write-Host ""

function Get-SteamLibraries {
  $libs = @()
  $sp = (Get-ItemProperty 'HKCU:\Software\Valve\Steam' -EA SilentlyContinue).SteamPath
  if (-not $sp) { $sp = "${env:ProgramFiles(x86)}\Steam" }
  if ($sp) {
    $sp = $sp -replace '/','\'; $libs += $sp
    $vdf = Join-Path $sp 'steamapps\libraryfolders.vdf'
    if (Test-Path $vdf) {
      Get-Content $vdf | Select-String '"path"\s+"(.+?)"' | ForEach-Object {
        $libs += ($_.Matches[0].Groups[1].Value -replace '\\\\','\')
      }
    }
  }
  $libs | Where-Object { $_ } | Select-Object -Unique
}

$game = $null
foreach ($lib in Get-SteamLibraries) {
  $g = Join-Path $lib 'steamapps\common\sotes'
  if (Test-Path (Join-Path $g 'sotes_en.exe')) { $game = $g; break }
}
if (-not $game) {
  Warn "Could not auto-detect the game folder - please pick it."
  Add-Type -AssemblyName System.Windows.Forms | Out-Null
  $d = New-Object System.Windows.Forms.FolderBrowserDialog
  $d.Description = "Select the Fortune Summoners sotes folder"
  if ($d.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) { $game = $d.SelectedPath }
}
if (-not $game) { Warn "No folder selected - nothing removed."; Read-Host "Press Enter to exit"; exit }

$modsDir = Join-Path $game 'mods'
Step "Removing the voice patch from: $game"
foreach ($f in (Join-Path $modsDir 'ennse_voice.dll'),(Join-Path $game 'sotesx_s.dll'),(Join-Path $game 'oss_voice.log')) {
  if (Test-Path $f) { Remove-Item $f -Force; Good ("removed " + (Split-Path $f -Leaf)) }
}
# Remove the loader too, but ONLY if no other mods remain (keeps it for the trainer etc.)
$others = @(Get-ChildItem $modsDir -Filter *.dll -EA SilentlyContinue)
if ($others.Count -eq 0) {
  foreach ($f in (Join-Path $game 'version.dll'),(Join-Path $game 'realver.dll'),(Join-Path $game 'oss_modloader.log')) {
    if (Test-Path $f) { Remove-Item $f -Force; Good ("removed " + (Split-Path $f -Leaf)) }
  }
  if (Test-Path $modsDir) { Remove-Item $modsDir -Recurse -Force -EA SilentlyContinue }
  Write-Host ""; Good "Uninstalled - the game is back to vanilla (English text, no voice)."
} else {
  Write-Host ""; Good "Voice patch removed. Kept the mod loader (other mods present in mods\)."
}
Read-Host "Press Enter to exit"
