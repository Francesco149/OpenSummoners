# oss-rhost job runner — lives in the INTERACTIVE logon session (scheduled task at logon).
# Windows OpenSSH runs commands in session 0 (no desktop ⇒ no DirectDraw device), so the
# ssh side only drops jobs here: C:\oss-rhost\queue\<id>.job.ps1 → runs it →
# <id>.out (stdout+stderr) and <id>.done (exit code). FIFO; a job may fan out internally.
# Separate from openrecet's C:\orc-rhost runner (own dir, own task, own queue): both can
# run at the same time without seeing each other's jobs.
$q = 'C:\oss-rhost\queue'
New-Item -ItemType Directory -Force $q | Out-Null
Set-Content -Path "$q\runner.pid" -Value $PID
while ($true) {
    $jobs = Get-ChildItem $q -Filter '*.job.ps1' -ErrorAction SilentlyContinue | Sort-Object Name
    foreach ($j in $jobs) {
        $id = $j.Name -replace '\.job\.ps1$', ''
        $run = Join-Path $q "$id.running.ps1"
        try { Move-Item $j.FullName $run -ErrorAction Stop } catch { continue }
        $out = Join-Path $q "$id.out"
        $p = Start-Process -FilePath powershell.exe -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',$run) `
             -NoNewWindow -Wait -PassThru -RedirectStandardOutput $out -RedirectStandardError "$out.err"
        if (Test-Path "$out.err") { Get-Content "$out.err" | Add-Content $out; Remove-Item "$out.err" }
        Set-Content -Path (Join-Path $q "$id.done") -Value $p.ExitCode
        Remove-Item $run -ErrorAction SilentlyContinue
    }
    Start-Sleep -Milliseconds 100
}
