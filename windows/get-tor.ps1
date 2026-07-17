<#
.SYNOPSIS
    Download the Tor Expert Bundle for Windows and extract it into windows/tor_3rd.

.DESCRIPTION
    Idempotent bootstrap step. If the tor_3rd directory already exists it does
    nothing. Otherwise it downloads the Tor Expert Bundle (.tar.gz) with curl and
    extracts it in place, giving you tor_3rd/tor/tor.exe (SOCKS proxy on
    127.0.0.1:9050).

    curl.exe and tar.exe are built into Windows 10 (1803+) and Windows 11, so no
    extra tooling is required.

.PARAMETER Version
    Tor Browser / Expert Bundle version to fetch (e.g. 14.5.5).
    Omit to auto-detect the latest stable from dist.torproject.org.

.EXAMPLE
    .\get-tor.ps1

.EXAMPLE
    .\get-tor.ps1 -Version 14.5.5
#>
[CmdletBinding()]
param(
    [string]$Version
)

$ErrorActionPreference = 'Stop'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

# tor_3rd lives next to this script, inside windows/
$TorDir = Join-Path $PSScriptRoot 'tor_3rd'

# --- only when the directory is not found ---
if (Test-Path -LiteralPath $TorDir) {
    Write-Host "tor_3rd already exists at '$TorDir' - nothing to do." -ForegroundColor Green
    return
}

# --- resolve version ---
if (-not $Version) {
    Write-Host "Detecting latest Tor version..."
    try {
        $html = (Invoke-WebRequest -Uri 'https://dist.torproject.org/torbrowser/' -UseBasicParsing).Content
        $Version = [regex]::Matches($html, 'href="(\d+\.\d+(?:\.\d+)?)/"') |
            ForEach-Object { $_.Groups[1].Value } |
            Sort-Object { [version]$_ } -Descending |
            Select-Object -First 1
    } catch {
        throw "Could not auto-detect the latest version. Re-run with -Version <x.y.z>. ($_)"
    }
}
Write-Host "Using Tor version: $Version"

$archive = "tor-expert-bundle-windows-x86_64-$Version.tar.gz"
$url = "https://dist.torproject.org/torbrowser/$Version/$archive"
$tmp = Join-Path $env:TEMP $archive

# --- download (curl) ---
Write-Host "Downloading $url"
curl.exe -L --fail -o "$tmp" "$url"
if ($LASTEXITCODE -ne 0) {
    throw "curl failed to download $url (exit $LASTEXITCODE). Check the version or your connection."
}

# --- extract into tor_3rd ---
New-Item -ItemType Directory -Path $TorDir -Force | Out-Null
Write-Host "Extracting into $TorDir"
tar.exe -xzf "$tmp" -C "$TorDir"
if ($LASTEXITCODE -ne 0) {
    throw "tar failed to extract $tmp (exit $LASTEXITCODE)."
}

Remove-Item -LiteralPath $tmp -Force -ErrorAction SilentlyContinue

# --- verify ---
$torExe = Join-Path $TorDir 'tor\tor.exe'
if (Test-Path -LiteralPath $torExe) {
    Write-Host "Done. tor.exe is at: $torExe" -ForegroundColor Green
} else {
    Write-Warning "Extraction finished but tor.exe was not found at expected path: $torExe"
}
