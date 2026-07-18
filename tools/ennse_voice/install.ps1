# Fortune Summoners (English SE) - Japanese Voice Patch : offline installer.
# (Bundled in ennse-voice-patch.zip.)  Installs the generic mod loader (version.dll)
# plus the voice mod (mods\ennse_voice.dll).  The paste-and-run one-liner
# (web-install.ps1) is the easy path - see README.
#
# ASCII-ONLY on purpose: Windows PowerShell 5.1 reads scripts as the system ANSI
# codepage, so any non-ASCII byte (em-dash, ellipsis) corrupts parsing.
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path

function Step($m){ Write-Host "[*] $m" -ForegroundColor Cyan }
function Good($m){ Write-Host "[+] $m" -ForegroundColor Green }
function Note($m){ Write-Host "    $m" -ForegroundColor DarkGray }
function Warn($m){ Write-Host "[!] $m" -ForegroundColor Yellow }
function Bad ($m){ Write-Host "[x] $m" -ForegroundColor Red }

Write-Host ""
Write-Host "  Fortune Summoners (English SE) - Japanese Voice Patch" -ForegroundColor Magenta
Write-Host "  =====================================================" -ForegroundColor Magenta
Write-Host ""

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

# 1. English SE game folder (has sotes_en.exe)
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
  Bad "sotes_en.exe not found. Aborting."; Read-Host "Press Enter to exit"; exit 1
}
Good "Game folder: $game"

# 1b. already installed? compare against THIS package + confirm a replace.
$verDll   = Join-Path $game 'version.dll'
$realDll  = Join-Path $game 'realver.dll'
$mods     = Join-Path $game 'mods'
$voiceMod = Join-Path $mods 'ennse_voice.dll'
$srcVer   = Join-Path $here 'version.dll'
$srcVoice = Join-Path $here 'ennse_voice.dll'
function Same($a,$b){ (Test-Path $a) -and (Test-Path $b) -and ((Get-FileHash $a).Hash -eq (Get-FileHash $b).Hash) }
if (Test-Path $realDll) {
  $verOk = Same $verDll $srcVer; $voiceOk = Same $voiceMod $srcVoice
  Write-Host ""
  if ($verOk -and $voiceOk) { Good "Already installed and UP TO DATE (matches this package)." }
  else { Warn "Already installed but OLD or MISMATCHED vs this package:" }
  Note ("version.dll         : " + $(if (-not (Test-Path $verDll))   {'MISSING'} elseif ($verOk)   {'up to date'} else {'DIFFERS - would be replaced'}))
  Note ("mods\ennse_voice.dll: " + $(if (-not (Test-Path $voiceMod)) {'MISSING'} elseif ($voiceOk) {'up to date'} else {'DIFFERS - would be replaced'}))
  Write-Host ""
  Warn "Reinstall will REPLACE:  version.dll, realver.dll, mods\ennse_voice.dll   (in $game)"
  Note "It will NOT touch:  sotesx_s.dll (your voice bank), sotes_en.exe, or any other mods."
  $c = Read-Host "Type Y to replace those files, anything else to cancel"
  if ($c -notmatch '^[Yy]') { Note "Cancelled - nothing changed."; Read-Host "Press Enter to exit"; exit 0 }
}

# 2. JP voice bank (sotesx_s.dll)
Step "Locating the Japanese voice bank (sotesx_s.dll)"
$jp = $null
$cands = @(
  (Join-Path $here 'sotesx_s.dll'),
  (Join-Path $game 'sotesx_s.dll'),
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
  Bad "sotesx_s.dll not found. You need the Japanese version to supply it. Aborting."
  Read-Host "Press Enter to exit"; exit 1
}
Good "found: $jp"
Write-Host ""

# 3. install : mod loader (version.dll) + realver.dll + mods\ennse_voice.dll + sotesx_s.dll
$realver = Join-Path $env:WINDIR 'SysWOW64\version.dll'
if (-not (Test-Path $realver)) { $realver = Join-Path $env:WINDIR 'System32\version.dll' }
$mods = Join-Path $game 'mods'
Step "Installing"
Copy-Item $realver (Join-Path $game 'realver.dll') -Force; Good "realver.dll  (forwards the real version.dll)"
Copy-Item (Join-Path $here 'version.dll') (Join-Path $game 'version.dll') -Force; Good "version.dll  (the mod loader)"
if (-not (Test-Path $mods)) { New-Item -ItemType Directory -Path $mods | Out-Null }
Copy-Item (Join-Path $here 'ennse_voice.dll') (Join-Path $mods 'ennse_voice.dll') -Force; Good "mods\ennse_voice.dll  (the voice patch)"
$dst = Join-Path $game 'sotesx_s.dll'
$sameFile = (Test-Path $dst) -and ((Resolve-Path $jp).Path -ieq (Resolve-Path $dst).Path)
if ($sameFile) {
  Good "sotesx_s.dll (already in place)"
} else {
  Step "Copying sotesx_s.dll (~253 MB, a few seconds)"
  Copy-Item $jp $dst -Force; Good "sotesx_s.dll (the voice bank)"
}

Write-Host ""
Good "Done. Launch the game normally."
Note "The first line (Arche's dad) is now spoken in Japanese. Run Uninstall.bat to remove."
Read-Host "Press Enter to exit"
