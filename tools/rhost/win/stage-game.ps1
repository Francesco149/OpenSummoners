# One-time: pristine copies of the game data for rhost (never the user's saves/config).
#   C:\oss-rhost\game\<edition>\  ← the Steam install (user\, temp\, logs, traces excluded)
param([string]$Edition = 'enold')
$ErrorActionPreference = 'Stop'
$src = @{ 'enold' = 'C:\Program Files (x86)\Steam\steamapps\common\Fortune Summoners';
          'ense'  = 'C:\Program Files (x86)\Steam\steamapps\common\sotes' }[$Edition]
$dst = "C:\oss-rhost\game\$Edition"
New-Item -ItemType Directory -Force $dst | Out-Null
$keep = @('*.exe','*.dll','*.spl','*.sin','*.htm','_sotes2.ini')
Get-ChildItem $src -File | Where-Object { $n = $_.Name; ($keep | Where-Object { $n -like $_ }) -and $n -notmatch '^(oss_|imgui|version\.dll|realver)' } | ForEach-Object {
    $d = Join-Path $dst $_.Name
    if (!(Test-Path $d) -or (Get-Item $d).Length -ne $_.Length) { Copy-Item $_.FullName $d }
}
if ($Edition -eq 'enold' -and !(Test-Path "$dst\sotes.unpacked.exe")) { Copy-Item 'C:\oss-ida\enold\sotes.unpacked.exe' "$dst\sotes.unpacked.exe" }
if ($Edition -eq 'ense') {
    Copy-Item 'C:\oss-ida\ense-en\sotes_en.exe.unpacked.exe' "$dst\sotes_en.unpacked.exe" -Force
    Copy-Item 'C:\oss-ida\ense-jp\sotes.exe.unpacked.exe' "$dst\sotes.unpacked.exe" -Force
}
Get-ChildItem $dst | Select Name,Length | ft -AutoSize
