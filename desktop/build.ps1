# SPDX-License-Identifier: GPL-3.0-only
$ErrorActionPreference = 'Stop'
Push-Location (Join-Path $PSScriptRoot '..')
try {
    $windowsRoot = (Get-Location).Path
    $windowsDir = [Environment]::GetFolderPath("Windows")
    $wsl = Join-Path $windowsDir "System32\wsl.exe"
    $linuxRoot = (& $wsl -e wslpath -u $windowsRoot | Out-String).Trim()
    if (-not $linuxRoot) { throw "Could not resolve repository path in WSL" }
    & $wsl -e sh -c 'cd "$1" && sh desktop/build-engine.sh' sh $linuxRoot
    if ($LASTEXITCODE -ne 0) { throw 'Native engine build failed' }
    $compiler = Join-Path $windowsDir 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
    & $compiler /nologo /target:winexe /platform:x64 /optimize+ /out:build\nightclaw-pc\NightClaw.exe /win32manifest:desktop\app.manifest /reference:System.Windows.Forms.dll /reference:System.Drawing.dll desktop\Native.cs desktop\NightClaw.cs desktop\Tests.cs
    if ($LASTEXITCODE -ne 0) { throw 'Desktop interface build failed' }
    Copy-Item -LiteralPath desktop/NightClaw.exe.config -Destination build/nightclaw-pc/NightClaw.exe.config
    Write-Host 'Built build/nightclaw-pc/NightClaw.exe'
} finally { Pop-Location }
