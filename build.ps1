# Build (and optionally deploy) the TrafficMonitor plugin.
#
#   .\build.ps1                                    build Release
#   .\build.ps1 -Debug                             build Debug
#   .\build.ps1 -Deploy                            build and copy into the running TrafficMonitor
#   .\build.ps1 -Deploy -TrafficMonitorDir "D:\apps\TrafficMonitor"
param(
    [switch]$Debug,
    [switch]$Deploy,
    [string]$TrafficMonitorDir
)

$ErrorActionPreference = 'Stop'
$configuration = if ($Debug) { 'Debug' } else { 'Release' }

# Locate MSBuild (Visual Studio 2022)
$msbuild = 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'
if (-not (Test-Path $msbuild)) {
    $vswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $install = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
        if ($install) { $msbuild = Join-Path $install 'MSBuild\Current\Bin\MSBuild.exe' }
    }
}
if (-not (Test-Path $msbuild)) { throw 'MSBuild not found, please install Visual Studio 2022 with the C++ workload' }

$project = Join-Path $PSScriptRoot 'MouseJigglerPlugin.vcxproj'
& $msbuild $project /t:Build /p:Configuration=$configuration /p:Platform=x64 /v:minimal /nologo /nodeReuse:false
if ($LASTEXITCODE -ne 0) { throw "build failed (exit $LASTEXITCODE)" }

$dll = Join-Path $PSScriptRoot "bin\$configuration\MouseJiggler.dll"
if (-not (Test-Path $dll)) { throw "output not found: $dll" }
Write-Host "`nbuilt: $dll ($([math]::Round((Get-Item $dll).Length / 1KB, 1)) KB)" -ForegroundColor Green

if ($Deploy) {
    if (-not $TrafficMonitorDir) {
        $running = Get-Process TrafficMonitor -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($running) {
            $TrafficMonitorDir = Split-Path $running.Path
        } elseif ($env:TRAFFICMONITOR_DIR) {
            $TrafficMonitorDir = $env:TRAFFICMONITOR_DIR
        } else {
            throw 'TrafficMonitor not found. Pass -TrafficMonitorDir or set TRAFFICMONITOR_DIR.'
        }
    }

    $target = Join-Path $TrafficMonitorDir 'plugins'
    if (-not (Test-Path $target)) { throw "plugins folder not found: $target" }

    Copy-Item $dll (Join-Path $target 'MouseJiggler.dll') -Force
    Write-Host "deployed to: $target\MouseJiggler.dll"
    if (Get-Process TrafficMonitor -ErrorAction SilentlyContinue) {
        Write-Host 'TrafficMonitor is running, restart it so the new dll gets loaded.' -ForegroundColor Yellow
    }
}
