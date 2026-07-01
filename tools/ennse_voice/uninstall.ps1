# Fortune Summoners EN-SE — Japanese Voice Patch uninstaller.
# Removes the patch files; leaves the game otherwise untouched.
$ErrorActionPreference = 'SilentlyContinue'
function Info($m){ Write-Host $m -ForegroundColor Cyan }
function Ok($m){ Write-Host $m -ForegroundColor Green }
function Warn($m){ Write-Host $m -ForegroundColor Yellow }

Info "=== Uninstall: Fortune Summoners EN-SE Japanese Voice Patch ==="

function Get-SteamLibraries {
  $libs = @()
  $sp = (Get-ItemProperty 'HKCU:\Software\Valve\Steam' -EA SilentlyContinue).SteamPath
  if (-not $sp) { $sp = "${env:ProgramFiles(x86)}\Steam" }
  if ($sp) {
    $sp = $sp -replace '/','\'; $libs += $sp
    $vdf = Join-Path $sp 'steamapps\libraryfolders.vdf'
    if (Test-Path $vdf) {
      foreach ($m in [regex]::Matches((Get-Content -Raw $vdf), '"path"\s*"([^"]+)"')) {
        $libs += ($m.Groups[1].Value -replace '\\\\','\')
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
  Warn "Couldn't auto-find the game folder."
  Add-Type -AssemblyName System.Windows.Forms | Out-Null
  $d = New-Object System.Windows.Forms.FolderBrowserDialog
  $d.Description = "Select the Fortune Summoners 'sotes' folder"
  if ($d.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) { $game = $d.SelectedPath }
}
if (-not $game) { Warn "No folder selected — nothing removed."; Read-Host "Press Enter to exit"; exit }

foreach ($f in 'version.dll','realver.dll','sotesx_s.dll','oss_voice.log') {
  $p = Join-Path $game $f
  if (Test-Path $p) { Remove-Item $p -Force; Ok "  removed $f" }
}
Ok "Uninstalled. The game is back to vanilla (English text, no voice)."
Read-Host "Press Enter to exit"
