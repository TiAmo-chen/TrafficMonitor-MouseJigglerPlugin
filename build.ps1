param(
    [ValidateSet('x64', 'Win32')]
    [string]$Platform = 'x64',
    [switch]$Debug,
    [switch]$Deploy,
    [switch]$Package,
    [string]$TrafficMonitorDir
)

$ErrorActionPreference = 'Stop'
$configuration = if ($Debug) { 'Debug' } else { 'Release' }

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

function Build-One([string]$platformName) {
    & $msbuild $project /t:Build /p:Configuration=$configuration /p:Platform=$platformName /v:minimal /nologo /nodeReuse:false | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "build failed for $platformName (exit $LASTEXITCODE)" }
    $path = Join-Path $PSScriptRoot "bin\$platformName\$configuration\MouseJiggler.dll"
    if (-not (Test-Path $path)) { throw "output not found: $path" }
    Write-Host ("built: {0} ({1} KB)" -f $path, [math]::Round((Get-Item $path).Length / 1KB, 1)) -ForegroundColor Green
    return $path
}

if ($Package) {
    # Build both architectures and produce release zips: MouseJiggler_V<version>_<arch>.zip
    $version = '1.1'
    $dist = Join-Path $PSScriptRoot 'dist'
    New-Item -ItemType Directory -Force -Path $dist | Out-Null
    foreach ($pair in @(@('x64', 'x64'), @('Win32', 'x86'))) {
        $dll = Build-One $pair[0]
        $staging = Join-Path $env:TEMP ("mj_" + $pair[1])
        New-Item -ItemType Directory -Force -Path $staging | Out-Null
        Copy-Item $dll (Join-Path $staging 'MouseJiggler.dll') -Force
        $zip = Join-Path $dist "MouseJiggler_V${version}_$($pair[1]).zip"
        if (Test-Path $zip) { Remove-Item $zip -Force }
        Compress-Archive -Path (Join-Path $staging 'MouseJiggler.dll') -DestinationPath $zip
        Write-Host "package: $zip" -ForegroundColor Green
    }
    exit 0
}

$dll = Build-One $Platform

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
