# Fails if version in CMakeLists.txt does not match the release tag. Application with
#   lower version than its tag would keep offering the same update after installing it.
param(
    [Parameter(Mandatory)][string]$Tag,
    [string]$CMakeFile = (Join-Path $PSScriptRoot '../CMakeLists.txt')
)
$ErrorActionPreference = 'Stop'

$match = Select-String -Path $CMakeFile -Pattern 'project\(FromSoftSaveManager VERSION ([0-9.]+)\)'
if (-not $match) { throw "Version not found in $CMakeFile" }
$version = $match.Matches[0].Groups[1].Value

# Tags have 3 components, '0.7.0' and '0.7.0.0' are the same version
if (($version -replace '^(\d+\.\d+\.\d+)\.0$', '$1') -ne $Tag) {
    throw "Version in CMakeLists.txt is '$version' but the tag is '$Tag'. Update the version and move the tag."
}
Write-Output "Version $version matches tag $Tag"
