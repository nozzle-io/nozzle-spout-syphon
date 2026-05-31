[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('windows')]
    [string] $Platform,

    [Parameter(Mandatory = $true)]
    [string] $PackageName,

    [Parameter(Mandatory = $true)]
    [string] $PackageRoot,

    [Parameter(Mandatory = $true)]
    [string] $BuildDir
)

$ErrorActionPreference = 'Stop'

$exe = Get-ChildItem -Path $BuildDir -Recurse -File -Filter 'nozzle-spout.exe' |
    Sort-Object -Property FullName |
    Select-Object -First 1
if ($null -eq $exe) {
    throw "missing nozzle-spout.exe under $BuildDir"
}

Remove-Item -LiteralPath 'package' -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $PackageName -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath 'verify-package' -Recurse -Force -ErrorAction SilentlyContinue

$packageDir = Join-Path 'package' $PackageRoot
New-Item -ItemType Directory -Force -Path $packageDir | Out-Null
Copy-Item -LiteralPath $exe.FullName -Destination (Join-Path $packageDir 'nozzle-spout.exe')
Copy-Item -LiteralPath 'README.md' -Destination (Join-Path $packageDir 'README.md')
Copy-Item -LiteralPath 'LICENSE' -Destination (Join-Path $packageDir 'LICENSE')
Copy-Item -LiteralPath 'THIRD-PARTY-NOTICES.md' -Destination (Join-Path $packageDir 'THIRD-PARTY-NOTICES.md')

$binaryPath = Join-Path $packageDir 'nozzle-spout.exe'
$bytes = [System.IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $binaryPath))
if ($bytes.Length -lt 2 -or $bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a) {
    throw "not a PE/MZ binary: $binaryPath"
}

Compress-Archive -Path $packageDir -DestinationPath $PackageName -CompressionLevel Optimal
if (!(Test-Path -LiteralPath $PackageName -PathType Leaf)) {
    throw "package was not created: $PackageName"
}

Expand-Archive -Path $PackageName -DestinationPath 'verify-package'
$required = @(
    "$PackageRoot/nozzle-spout.exe",
    "$PackageRoot/README.md",
    "$PackageRoot/LICENSE",
    "$PackageRoot/THIRD-PARTY-NOTICES.md"
)
foreach ($entry in $required) {
    $entryPath = Join-Path 'verify-package' $entry
    if (!(Test-Path -LiteralPath $entryPath -PathType Leaf)) {
        throw "package is missing: $entry"
    }
    Write-Output $entry
}

$verifiedExe = Join-Path 'verify-package' "$PackageRoot/nozzle-spout.exe"
$helpOut = Join-Path 'verify-package' 'nozzle-spout-help.out'
$listOut = Join-Path 'verify-package' 'nozzle-spout-list.out'
& $verifiedExe --help 1> $helpOut
if ($LASTEXITCODE -ne 0) {
    throw "nozzle-spout.exe --help failed with exit code $LASTEXITCODE"
}
if (!(Select-String -LiteralPath $helpOut -SimpleMatch 'Windows runs the SpoutDX D3D11 bridge')) {
    throw "nozzle-spout.exe --help did not report the compiled SpoutDX bridge"
}
& $verifiedExe --list 1> $listOut
if ($LASTEXITCODE -ne 0) {
    throw "nozzle-spout.exe --list failed with exit code $LASTEXITCODE"
}
