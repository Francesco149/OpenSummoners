# Fortune Summoners EN-SE — Japanese Voice Patch installer.
# Auto-detects the Steam game folder + your JP sotesx_s.dll; folder-picker fallback.
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path

function Info($m){ Write-Host $m -ForegroundColor Cyan }
function Ok($m){ Write-Host $m -ForegroundColor Green }
function Warn($m){ Write-Host $m -ForegroundColor Yellow }
function Err($m){ Write-Host $m -ForegroundColor Red }

Info "=== Fortune Summoners (English SE) — Japanese Voice Patch ==="
Write-Host ""

# --- locate Steam libraries -------------------------------------------------
function Get-SteamLibraries {
  $libs = @()
  $sp = (Get-ItemProperty 'HKCU:\Software\Valve\Steam' -EA SilentlyContinue).SteamPath
  if (-not $sp) { $sp = "${env:ProgramFiles(x86)}\Steam" }
  if ($sp) {
    $sp = $sp -replace '/','\'
    $libs += $sp
    $vdf = Join-Path $sp 'steamapps\libraryfolders.vdf'
    if (Test-Path $vdf) {
      foreach ($m in [regex]::Matches((Get-Content -Raw $vdf), '"path"\s*"([^"]+)"')) {
        $libs += ($m.Groups[1].Value -replace '\\\\','\')
      }
    }
  }
  $libs | Where-Object { $_ } | Select-Object -Unique
}

function Pick-Folder($desc) {
  Add-Type -AssemblyName System.Windows.Forms | Out-Null
  $d = New-Object System.Windows.Forms.FolderBrowserDialog
  $d.Description = $desc
  if ($d.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) { return $d.SelectedPath }
  return $null
}

# --- 1. English SE game folder (has sotes_en.exe) ---------------------------
$game = $null
foreach ($lib in Get-SteamLibraries) {
  $g = Join-Path $lib 'steamapps\common\sotes'
  if (Test-Path (Join-Path $g 'sotes_en.exe')) { $game = $g; break }
}
if (-not $game) {
  Warn "Could not auto-find the English special edition."
  Warn "Pick your game folder (the one containing sotes_en.exe)."
  $game = Pick-Folder "Select the Fortune Summoners 'sotes' folder (contains sotes_en.exe)"
}
if (-not $game -or -not (Test-Path (Join-Path $game 'sotes_en.exe'))) {
  Err "sotes_en.exe not found — aborting."; Read-Host "Press Enter to exit"; exit 1
}
Ok "English SE game folder: $game"

# --- 2. JP voice bank (sotesx_s.dll) ----------------------------------------
$jp = $null
$cands = @(
  "${env:ProgramFiles(x86)}\lizsoft\FortuneSummoners\sotesx_s.dll",
  "${env:ProgramFiles}\lizsoft\FortuneSummoners\sotesx_s.dll"
)
foreach ($lib in Get-SteamLibraries) { $cands += (Join-Path $lib 'steamapps\common\Fortune Summoners SE\sotesx_s.dll') }
foreach ($c in $cands) { if (Test-Path $c) { $jp = $c; break } }
if (-not $jp) {
  Warn "Could not auto-find sotesx_s.dll (the Japanese voice bank)."
  Warn "Pick your JAPANESE version's folder (the one containing sotesx_s.dll)."
  $f = Pick-Folder "Select your Japanese Fortune Summoners folder (contains sotesx_s.dll)"
  if ($f) { $jp = Join-Path $f 'sotesx_s.dll' }
}
if (-not $jp -or -not (Test-Path $jp)) {
  Err "sotesx_s.dll not found. You need the Japanese version to supply it — aborting."
  Read-Host "Press Enter to exit"; exit 1
}
Ok "Japanese voice bank: $jp"
Write-Host ""

# --- 3. install -------------------------------------------------------------
$realver = Join-Path $env:WINDIR 'SysWOW64\version.dll'
if (-not (Test-Path $realver)) { $realver = Join-Path $env:WINDIR 'System32\version.dll' }

Info "Installing…"
Copy-Item $realver               (Join-Path $game 'realver.dll') -Force; Ok "  realver.dll  (proxy target = your system version.dll)"
Copy-Item (Join-Path $here 'version.dll') (Join-Path $game 'version.dll') -Force; Ok "  version.dll  (the patch)"
$dstVoice = Join-Path $game 'sotesx_s.dll'
if ((Resolve-Path $jp).Path -ieq $dstVoice) {
  Ok "  sotesx_s.dll (already in place)"
} else {
  Info "  Copying sotesx_s.dll (~253 MB, one moment)…"
  Copy-Item $jp $dstVoice -Force; Ok "  sotesx_s.dll (the voice bank)"
}

Write-Host ""
Ok "Done!  Launch the game normally."
Info "The first line of the game (Arche's dad) should now be spoken in Japanese."
Info "If anything's off, check oss_voice.log in the game folder, or run Uninstall.bat."
Read-Host "Press Enter to exit"
