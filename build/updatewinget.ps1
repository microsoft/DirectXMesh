<#

.NOTES
Copyright (c) Microsoft Corporation.
Licensed under the MIT License.

.SYNOPSIS
Updates the winget manifests for meshconvert to match a GitHub release.

.DESCRIPTION
This script creates a new winget manifest version for meshconvert under
D:\winget-pkgs based on the most recent release date in README.md. It copies the previous
version's manifests, then updates PackageVersion, ReleaseDate, InstallerSha256, InstallerUrl,
and ReleaseNotesUrl to match the new release.

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
$wingetPkgs = "D:\winget-pkgs"

$meshconvertManifestBase = Join-Path $wingetPkgs "manifests\m\Microsoft\DirectX\Mesh"

if (-Not (Test-Path $readme)) {
    Write-Error "ERROR: Cannot find README.md at $readme" -ErrorAction Stop
}

if (-Not (Test-Path $wingetPkgs)) {
    Write-Error "ERROR: Cannot find winget-pkgs at $wingetPkgs" -ErrorAction Stop
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

# winget version format: YYYY.M.D (no leading zeros)
$packageVersion = $releaseDate.ToString("yyyy.M.d")

# winget ReleaseDate format: YYYY-MM-DD
$releaseDateYaml = $releaseDate.ToString("yyyy-MM-dd")

Write-Host "Release Date: $releaseDateStr"
Write-Host "Package Version: $packageVersion"
Write-Host "Release Date (YAML): $releaseDateYaml"

# --- Find previous version directory ---
function Get-LatestVersionDir {
    param([string]$BasePath)
    $dirs = Get-ChildItem -Path $BasePath -Directory | Sort-Object Name
    if ($dirs.Count -eq 0) {
        Write-Error "ERROR: No existing version directories found in $BasePath" -ErrorAction Stop
    }
    return $dirs[-1]
}

$prevMeshconvert = Get-LatestVersionDir $meshconvertManifestBase

Write-Host "`nPrevious meshconvert version: $($prevMeshconvert.Name)"

# Check if new version already exists
$newMeshconvertDir = Join-Path $meshconvertManifestBase $packageVersion

if (Test-Path $newMeshconvertDir) {
    Write-Error "ERROR: meshconvert version $packageVersion already exists at $newMeshconvertDir" -ErrorAction Stop
}

# --- Download release assets and compute SHA256 hashes ---
$ProgressPreference = 'SilentlyContinue'
$tempDir = Join-Path $Env:Temp $(New-Guid)
New-Item -Type Directory -Path $tempDir | Out-Null

$assets = @(
    @{ Name = "meshconvert.exe"; Arch = "x64" },
    @{ Name = "meshconvert_arm64.exe"; Arch = "arm64" }
)

$hashes = @{}

foreach ($asset in $assets) {
    $url = "https://github.com/microsoft/DirectXMesh/releases/download/$Tag/$($asset.Name)"
    $outPath = Join-Path $tempDir $asset.Name
    Write-Host "`nDownloading $($asset.Name) from $url..."
    try {
        Invoke-WebRequest -Uri $url -OutFile $outPath -ErrorAction Stop
    }
    catch {
        Write-Error "ERROR: Failed to download $($asset.Name)!" -ErrorAction Stop
    }
    $hash = (Get-FileHash -Path $outPath -Algorithm SHA256).Hash.ToLower()
    $hashes[$asset.Name] = $hash
    Write-Host "  SHA256: $hash"
}

# --- Create new meshconvert manifest ---
Write-Host "`nCreating meshconvert $packageVersion manifests..."
Copy-Item -Path $prevMeshconvert.FullName -Destination $newMeshconvertDir -Recurse

foreach ($file in Get-ChildItem -Path $newMeshconvertDir -Filter "*.yaml") {
    $content = Get-Content $file.FullName -Raw
    $content = $content -replace "PackageVersion:\s+\S+", "PackageVersion: $packageVersion"

    if ($file.Name -match "installer") {
        $content = $content -replace "ReleaseDate:\s+\S+", "ReleaseDate: $releaseDateYaml"

        # Update installer URLs
        $content = $content -replace "(InstallerUrl:\s+).+meshconvert\.exe", "`${1}https://github.com/microsoft/DirectXMesh/releases/download/$Tag/meshconvert.exe"
        $content = $content -replace "(InstallerUrl:\s+).+meshconvert_arm64\.exe", "`${1}https://github.com/microsoft/DirectXMesh/releases/download/$Tag/meshconvert_arm64.exe"

        # Update SHA256 hashes per architecture block
        $lines = $content -split "`n"
        $currentArch = ""
        for ($i = 0; $i -lt $lines.Count; $i++) {
            if ($lines[$i] -match "Architecture:\s+(\S+)") {
                $currentArch = $Matches[1]
            }

            if ($lines[$i] -match "InstallerSha256:") {
                $matchingAsset = $assets | Where-Object { $_.Arch -eq $currentArch }
                if ($matchingAsset) {
                    $lines[$i] = "  InstallerSha256: $($hashes[$matchingAsset.Name])"
                }
            }
        }
        $content = $lines -join "`n"
    }

    if ($file.Name -match "locale") {
        $content = $content -replace "(ReleaseNotesUrl:\s+).+", "`${1}https://github.com/microsoft/DirectXMesh/releases/tag/$Tag"
    }

    Set-Content -Path $file.FullName -Value $content -NoNewline
}

foreach ($file in Get-ChildItem -Path $newMeshconvertDir -Filter "*.yaml") {
    Write-Host "  $($file.Name)"
}

# --- Cleanup ---
Remove-Item -Recurse -Force $tempDir

Write-Host "`nwinget manifests created successfully!"
Write-Host "`nNew manifest directory:"
Write-Host "  $newMeshconvertDir"
Write-Host "`nNext steps:"
Write-Host "  1. Review the generated manifest files"
Write-Host "  2. Validate with: winget validate $newMeshconvertDir"
Write-Host "  3. Test with: winget install --manifest $newMeshconvertDir"
Write-Host "  4. Submit a PR to the winget-pkgs repository"
