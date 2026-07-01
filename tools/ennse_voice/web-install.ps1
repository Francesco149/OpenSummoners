# Fortune Summoners (English SE) - Japanese Voice Patch : web installer.
#
# Paste-and-run one-liner (downloads the latest nightly and installs it):
#
#   [Net.ServicePointManager]::SecurityProtocol='Tls12'; irm https://raw.githubusercontent.com/Francesco149/OpenSummoners/master/tools/ennse_voice/web-install.ps1 | iex
#
# Idempotent: if the patch is already installed it offers to uninstall.
# Auto-detects the Steam game folder + your Japanese sotesx_s.dll (file-picker
# fallback). Ships no game assets: you supply sotesx_s.dll from your own JP copy.
#
# ASCII-ONLY on purpose: Windows PowerShell 5.1 reads scripts as the system
# ANSI codepage, so any non-ASCII byte (em-dash, ellipsis) corrupts parsing.

$ErrorActionPreference = 'Stop'
try { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 } catch {}

$BASE = 'https://github.com/Francesco149/OpenSummoners/releases/download/nightly'

# ---- pretty output --------------------------------------------------------
function Step($m){ Write-Host "[*] $m" -ForegroundColor Cyan }
function Good($m){ Write-Host "[+] $m" -ForegroundColor Green }
function Note($m){ Write-Host "    $m" -ForegroundColor DarkGray }
function Warn($m){ Write-Host "[!] $m" -ForegroundColor Yellow }
function Bad ($m){ Write-Host "[x] $m" -ForegroundColor Red }

Write-Host ""
Write-Host "  Fortune Summoners (English SE) - Japanese Voice Patch" -ForegroundColor Magenta
Write-Host "  =====================================================" -ForegroundColor Magenta
Write-Host "  English text + Japanese dialogue voice. Exe untouched." -ForegroundColor DarkGray
Write-Host ""

# ---- helpers --------------------------------------------------------------
function Get-SteamLibraries {
  $libs = @()
  $sp = (Get-ItemProperty 'HKCU:\Software\Valve\Steam' -EA SilentlyContinue).SteamPath
  if (-not $sp) { $sp = "${env:ProgramFiles(x86)}\Steam" }
  if ($sp) {
    $sp = $sp -replace '/','\'
    $libs += $sp
    $vdf = Join-Path $sp 'steamapps\libraryfolders.vdf'
    if (Test-Path $vdf) {
      Get-Content $vdf | Select-String '"path"\s+"(.+?)"' | ForEach-Object {
        $libs += ($_.Matches[0].Groups[1].Value -replace '\\\\','\')
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

function Pick-File($title) {
  Add-Type -AssemblyName System.Windows.Forms | Out-Null
  $d = New-Object System.Windows.Forms.OpenFileDialog
  $d.Title  = $title
  $d.Filter = 'sotesx_s.dll|sotesx_s.dll|All files (*.*)|*.*'
  if ($d.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) { return $d.FileName }
  return $null
}

# Fetch version.dll: prefer the loose asset, fall back to extracting the zip
# (older releases only shipped the zip).
function Get-VersionDll($dest) {
  try {
    Invoke-WebRequest "$BASE/version.dll" -OutFile $dest -UseBasicParsing
  } catch {
    $zip = Join-Path $env:TEMP 'ennse-voice-patch.zip'
    $ex  = Join-Path $env:TEMP 'ennse-voice-patch-x'
    Invoke-WebRequest "$BASE/ennse-voice-patch.zip" -OutFile $zip -UseBasicParsing
    if (Test-Path $ex) { Remove-Item $ex -Recurse -Force }
    Expand-Archive $zip $ex -Force
    $hit = Get-ChildItem $ex -Recurse -Filter version.dll | Select-Object -First 1
    if (-not $hit) { throw "version.dll not found in the release zip" }
    Copy-Item $hit.FullName $dest -Force
  }
}

# ---- 1. locate the game ---------------------------------------------------
Step "Locating the English special edition (sotes_en.exe)"
$game = $null
foreach ($lib in Get-SteamLibraries) {
  $g = Join-Path $lib 'steamapps\common\sotes'
  if (Test-Path (Join-Path $g 'sotes_en.exe')) { $game = $g; break }
}
if (-not $game) {
  Warn "Could not auto-detect it - please pick the folder with sotes_en.exe."
  $game = Pick-Folder 'Select the Fortune Summoners sotes folder (contains sotes_en.exe)'
}
if (-not $game -or -not (Test-Path (Join-Path $game 'sotes_en.exe'))) {
  Bad "sotes_en.exe not found. Aborting."; Read-Host "Press Enter to exit"; return
}
Good "Game folder: $game"

$verDll   = Join-Path $game 'version.dll'
$realDll  = Join-Path $game 'realver.dll'
$voiceDll = Join-Path $game 'sotesx_s.dll'

# ---- 2. idempotency: already installed? offer uninstall -------------------
if (Test-Path $realDll) {   # realver.dll is unique to this patch
  Write-Host ""
  Warn "The patch is ALREADY INSTALLED here:"
  Note ("version.dll  : " + $(if (Test-Path $verDll)   {'present'} else {'MISSING'}))
  Note ("realver.dll  : " + $(if (Test-Path $realDll)  {'present'} else {'MISSING'}))
  Note ("sotesx_s.dll : " + $(if (Test-Path $voiceDll) {'present'} else {'MISSING'}))
  Write-Host ""
  $ans = Read-Host "Type U to uninstall, R to reinstall/repair, anything else to cancel"
  if ($ans -match '^[Uu]') {
    Write-Host ""; Step "Uninstalling"
    foreach ($f in 'version.dll','realver.dll','sotesx_s.dll','oss_voice.log') {
      $p = Join-Path $game $f
      if (Test-Path $p) { Remove-Item $p -Force; Good "removed $f" }
    }
    Write-Host ""; Good "Uninstalled - the game is back to vanilla (English text, no voice)."
    Read-Host "Press Enter to exit"; return
  }
  elseif ($ans -notmatch '^[Rr]') {
    Note "Cancelled - nothing changed."; Read-Host "Press Enter to exit"; return
  }
  Write-Host ""; Step "Reinstalling / repairing"
}

# ---- 3. install -----------------------------------------------------------
Write-Host ""
Step "Copying your system version.dll -> realver.dll"
$sys = Join-Path $env:WINDIR 'SysWOW64\version.dll'
if (-not (Test-Path $sys)) { $sys = Join-Path $env:WINDIR 'System32\version.dll' }
Copy-Item $sys $realDll -Force
Good "realver.dll  (forwards the real version.dll)"

Step "Downloading the patch from the nightly release"
Get-VersionDll $verDll
Good ("version.dll  (" + [math]::Round((Get-Item $verDll).Length/1KB) + " KB)")

if (Test-Path $voiceDll) {
  Good "sotesx_s.dll (already present)"
} else {
  Step "Locating the Japanese voice bank (sotesx_s.dll)"
  $jp = $null
  $cands = @(
    "${env:ProgramFiles(x86)}\lizsoft\FortuneSummoners\sotesx_s.dll",
    "${env:ProgramFiles}\lizsoft\FortuneSummoners\sotesx_s.dll"
  )
  foreach ($lib in Get-SteamLibraries) {
    $cands += (Join-Path $lib 'steamapps\common\Fortune Summoners SE\sotesx_s.dll')
    $cands += (Join-Path $lib 'steamapps\common\FortuneSummoners\sotesx_s.dll')
  }
  foreach ($c in $cands) { if ($c -and (Test-Path $c)) { $jp = $c; break } }
  if (-not $jp) {
    Warn "Could not auto-detect it - please pick sotesx_s.dll from your Japanese copy."
    $jp = Pick-File 'Select sotesx_s.dll from your Japanese Fortune Summoners'
  }
  if (-not $jp -or -not (Test-Path $jp)) {
    Bad "No sotesx_s.dll - the patch needs it. Rolling back."
    Remove-Item $verDll,$realDll -Force -EA SilentlyContinue
    Read-Host "Press Enter to exit"; return
  }
  Good ("found: " + $jp)
  Step "Copying sotesx_s.dll (~253 MB, a few seconds)"
  Copy-Item $jp $voiceDll -Force
  Good "sotesx_s.dll (the voice bank)"
}

# ---- done -----------------------------------------------------------------
Write-Host ""
Good "Installed to: $game"
Note "version.dll + realver.dll + sotesx_s.dll"
Write-Host ""
Write-Host "  Launch the game normally - the first line (Arche's dad) is now" -ForegroundColor Green
Write-Host "  spoken in Japanese. Re-run this one-liner to uninstall." -ForegroundColor Green
Write-Host ""
Read-Host "Press Enter to exit"
