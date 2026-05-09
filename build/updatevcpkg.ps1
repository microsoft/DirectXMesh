<#

.NOTES
Copyright (c) Microsoft Corporation.
Licensed under the MIT License.

.SYNOPSIS
Updates the vcpkg port for DirectXMesh to match a GitHub release.

.DESCRIPTION
This script updates the vcpkg port at D:\vcpkg\ports\directxmesh to match a specific
GitHub release by tag. It updates the version-date in vcpkg.json, the tag and SHA512
hashes in portfile.cmake for the source archive, meshconvert.exe, and meshconvert_arm64.exe.

.PARAMETER Tag
The GitHub release tag (e.g., 'may2026', 'mar2026'). Defaults to the latest tag.

.LINK
https://github.com/microsoft/DirectXMesh/wiki

#>

param(
    [string]$Tag = ""
)

$repoRoot = Split-Path -Path $PSScriptRoot -Parent
$readme = Join-Path $repoRoot "README.md"
$portDir = "D:\vcpkg\ports\directxmesh"
$vcpkgJson = Join-Path $portDir "vcpkg.json"
$portfile = Join-Path $portDir "portfile.cmake"

if (-Not (Test-Path $readme)) {
    Write-Error "ERROR: Cannot find README.md at $readme" -ErrorAction Stop
}

if ((-Not (Test-Path $vcpkgJson)) -Or (-Not (Test-Path $portfile))) {
    Write-Error "ERROR: Cannot find vcpkg port files at $portDir" -ErrorAction Stop
}

# Determine tag from latest git tag if not provided
if ($Tag.Length -eq 0) {
    $Tag = (git --no-pager -C $repoRoot tag --sort=-creatordate | Select-Object -First 1).Trim()
    if ($Tag.Length -eq 0) {
        Write-Error "ERROR: Failed to determine latest tag!" -ErrorAction Stop
    }
}

Write-Host "Release Tag: $Tag"

# Parse release date from README.md (format: "## Month Day, Year")
$rawReleaseDate = (Get-Content $readme) | Select-String -Pattern "^##\s+[A-Z][a-z]+\s+\d+,?\s+\d{4}" | Select-Object -First 1
if ([string]::IsNullOrEmpty($rawReleaseDate)) {
    Write-Error "ERROR: Failed to find release date in README.md!" -ErrorAction Stop
}

$releaseDateStr = ($rawReleaseDate -replace '^##\s+', '').Trim()
$releaseDate = [datetime]::Parse($releaseDateStr, [System.Globalization.CultureInfo]::InvariantCulture)
$versionDate = $releaseDate.ToString("yyyy-MM-dd")

Write-Host "Release Date: $releaseDateStr"
Write-Host "Version Date: $versionDate"

# --- Update vcpkg.json ---
Write-Host "`nUpdating vcpkg.json..."

$jsonContent = Get-Content $vcpkgJson -Raw
$jsonContent = $jsonContent -replace '"version-date":\s*"[^"]*"', "`"version-date`": `"$versionDate`""
$jsonContent = $jsonContent -replace ',\s*"port-version":\s*\d+', ''
$jsonContent = $jsonContent -replace '"port-version":\s*\d+,?\s*', ''
Set-Content -Path $vcpkgJson -Value $jsonContent -NoNewline

Write-Host "  version-date set to $versionDate"

# --- Update portfile.cmake tag ---
Write-Host "`nUpdating portfile.cmake tag..."

$portContent = Get-Content $portfile -Raw
$portContent = $portContent -replace 'set\(DIRECTXMESH_TAG\s+\S+\)', "set(DIRECTXMESH_TAG $Tag)"
Set-Content -Path $portfile -Value $portContent -NoNewline

Write-Host "  Tag set to $Tag"

# --- Download and hash source archive ---
$ProgressPreference = 'SilentlyContinue'
$tempDir = Join-Path $Env:Temp $(New-Guid)
New-Item -Type Directory -Path $tempDir | Out-Null

$sourceUrl = "https://github.com/Microsoft/DirectXMesh/archive/refs/tags/$Tag.tar.gz"
$sourcePath = Join-Path $tempDir "$Tag.tar.gz"

Write-Host "`nDownloading source archive from $sourceUrl..."
try {
    Invoke-WebRequest -Uri $sourceUrl -OutFile $sourcePath -ErrorAction Stop
}
catch {
    Write-Error "ERROR: Failed to download source archive!" -ErrorAction Stop
}

$sourceHash = (Get-FileHash -Path $sourcePath -Algorithm SHA512).Hash.ToLower()
Write-Host "  Source SHA512: $sourceHash"

# Replace SHA512 in vcpkg_from_github block
$portContent = Get-Content $portfile -Raw
$portContent = $portContent -replace '(vcpkg_from_github\s*\([^)]*SHA512\s+)[0-9a-fA-F]+', "`${1}$sourceHash"
Set-Content -Path $portfile -Value $portContent -NoNewline

# --- Download and hash meshconvert.exe ---
$meshconvertUrl = "https://github.com/Microsoft/DirectXMesh/releases/download/$Tag/meshconvert.exe"
$meshconvertPath = Join-Path $tempDir "meshconvert.exe"

Write-Host "`nDownloading meshconvert.exe from $meshconvertUrl..."
try {
    Invoke-WebRequest -Uri $meshconvertUrl -OutFile $meshconvertPath -ErrorAction Stop
}
catch {
    Write-Error "ERROR: Failed to download meshconvert.exe!" -ErrorAction Stop
}

$meshconvertHash = (Get-FileHash -Path $meshconvertPath -Algorithm SHA512).Hash.ToLower()
Write-Host "  meshconvert.exe SHA512: $meshconvertHash"

# --- Download and hash meshconvert_arm64.exe ---
$meshconvertArm64Url = "https://github.com/Microsoft/DirectXMesh/releases/download/$Tag/meshconvert_arm64.exe"
$meshconvertArm64Path = Join-Path $tempDir "meshconvert_arm64.exe"

Write-Host "`nDownloading meshconvert_arm64.exe from $meshconvertArm64Url..."
try {
    Invoke-WebRequest -Uri $meshconvertArm64Url -OutFile $meshconvertArm64Path -ErrorAction Stop
}
catch {
    Write-Error "ERROR: Failed to download meshconvert_arm64.exe!" -ErrorAction Stop
}

$meshconvertArm64Hash = (Get-FileHash -Path $meshconvertArm64Path -Algorithm SHA512).Hash.ToLower()
Write-Host "  meshconvert_arm64.exe SHA512: $meshconvertArm64Hash"

# --- Replace SHA512 hashes for meshconvert.exe and meshconvert_arm64.exe in portfile ---
# We need to replace the SHA512 in each vcpkg_download_distfile block specifically.
# The x64 block downloads "meshconvert.exe" and the arm64 block downloads "meshconvert_arm64.exe".

$portContent = Get-Content $portfile -Raw

# Match the vcpkg_download_distfile block for meshconvert.exe (x64)
$portContent = $portContent -replace `
    '(vcpkg_download_distfile\s*\(\s*\n\s*MESHCONVERT_EXE\s*\n\s*URLS\s+"[^"]*meshconvert\.exe"\s*\n\s*FILENAME\s+"[^"]*"\s*\n\s*SHA512\s+)[0-9a-fA-F]+', `
    "`${1}$meshconvertHash"

# Match the vcpkg_download_distfile block for meshconvert_arm64.exe
$portContent = $portContent -replace `
    '(vcpkg_download_distfile\s*\(\s*\n\s*MESHCONVERT_EXE\s*\n\s*URLS\s+"[^"]*meshconvert_arm64\.exe"\s*\n\s*FILENAME\s+"[^"]*"\s*\n\s*SHA512\s+)[0-9a-fA-F]+', `
    "`${1}$meshconvertArm64Hash"

Set-Content -Path $portfile -Value $portContent -NoNewline

# --- Cleanup ---
Remove-Item -Recurse -Force $tempDir

Write-Host "`nvcpkg port updated successfully!"
Write-Host "`nUpdated files:"
Write-Host "  $vcpkgJson"
Write-Host "  $portfile"

$portContent = Get-Content $portfile -Raw
if ($portContent -match '\bPATCHES\b') {
    Write-Warning "This port includes patches. Review them to either remove or update."
}
