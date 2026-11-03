# Creates release zip file from a build directory and checks that the packaged application starts
param(
    [Parameter(Mandatory)][string]$BuildDir,
    # Name of the zip file and of the root directory inside it
    [Parameter(Mandatory)][string]$Name,
    [Parameter(Mandatory)][string]$OutDir
)
$ErrorActionPreference = 'Stop'

$stagingRoot = Join-Path ([IO.Path]::GetTempPath()) "fssm-package-$([guid]::NewGuid())"
$staging = Join-Path $stagingRoot $Name
New-Item -ItemType Directory -Force $staging | Out-Null
try {
    Copy-Item (Join-Path $BuildDir '*.dll') $staging
    Copy-Item (Join-Path $BuildDir 'FromSoftSaveManager.exe') $staging
    Copy-Item (Join-Path $BuildDir 'FromSoftSaveManagerUpdater.exe') $staging
    Copy-Item -Recurse (Join-Path $BuildDir 'plugins') $staging

    $required = @(
        'FromSoftSaveManager.exe',
        'FromSoftSaveManagerUpdater.exe',
        'Qt6Core.dll',
        'Qt6Gui.dll',
        'Qt6Network.dll',
        'Qt6Sql.dll',
        'Qt6Widgets.dll',
        'libgcc_s_seh-1.dll',
        'libstdc++-6.dll',
        'libwinpthread-1.dll',
        'plugins/platforms/qwindows.dll',
        'plugins/sqldrivers/qsqlite.dll',
        'plugins/tls/qschannelbackend.dll'
    )
    foreach ($file in $required) {
        if (-not (Test-Path (Join-Path $staging $file))) { throw "Missing from package: $file" }
    }

    # Start the application with PATH without Qt so a dll missing in package is noticed,
    #   and with separate data directories so it does not touch existing config.
    $dataDir = Join-Path $stagingRoot 'data'
    New-Item -ItemType Directory $dataDir | Out-Null
    $oldPath, $oldLocal, $oldRoaming = $env:PATH, $env:LOCALAPPDATA, $env:APPDATA
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
    $env:LOCALAPPDATA = $dataDir
    $env:APPDATA = $dataDir
    try {
        $process = Start-Process (Join-Path $staging 'FromSoftSaveManager.exe') -PassThru
    } finally {
        $env:PATH, $env:LOCALAPPDATA, $env:APPDATA = $oldPath, $oldLocal, $oldRoaming
    }
    Start-Sleep -Seconds 5
    if ($process.HasExited) { throw "Packaged application exited on start with code $($process.ExitCode)" }
    Stop-Process -Id $process.Id -Force
    $process.WaitForExit()

    New-Item -ItemType Directory -Force $OutDir | Out-Null
    $zipPath = Join-Path $OutDir "$Name.zip"
    Compress-Archive -Path $staging -DestinationPath $zipPath -Force
    Write-Output "Created $zipPath"
} finally {
    Remove-Item -Recurse -Force $stagingRoot
}
