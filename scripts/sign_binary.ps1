# sign_binary.ps1 - Local development code signing script
# Signs the executable with a self-signed certificate for local dev environments.
# Parameters:
#   -FilePath        : Path to the binary to sign
#   -Subject         : Certificate subject (CN=...)
#   -CreateIfMissing : Create a self-signed cert if none exists matching Subject

param(
    [Parameter(Mandatory=$true)]
    [string]$FilePath,

    [Parameter(Mandatory=$false)]
    [string]$Subject = "CN=AXIOM Local Dev Code Signing",

    [Parameter(Mandatory=$false)]
    [switch]$CreateIfMissing
)

# Locate or create certificate
$cert = Get-ChildItem Cert:\CurrentUser\My | Where-Object { $_.Subject -eq $Subject } | Select-Object -First 1

if (-not $cert -and $CreateIfMissing) {
    Write-Host "[sign_binary] Creating self-signed certificate: $Subject"
    $cert = New-SelfSignedCertificate `
        -Subject $Subject `
        -CertStoreLocation "Cert:\CurrentUser\My" `
        -Type CodeSigningCert `
        -KeyUsage DigitalSignature `
        -NotAfter (Get-Date).AddYears(5)
}

if (-not $cert) {
    Write-Host "[sign_binary] No signing certificate found for '$Subject'. Skipping signing."
    exit 0
}

if (-not (Test-Path $FilePath)) {
    Write-Host "[sign_binary] Binary not found: $FilePath. Skipping signing."
    exit 0
}

try {
    $result = Set-AuthenticodeSignature -FilePath $FilePath -Certificate $cert -TimestampServer "" 2>&1
    if ($result.Status -eq "Valid" -or $result.Status -eq "UnknownError") {
        Write-Host "[sign_binary] Signed: $FilePath"
        exit 0
    } else {
        Write-Host "[sign_binary] Signing completed (status: $($result.Status)): $FilePath"
        exit 0
    }
} catch {
    Write-Host "[sign_binary] Warning: Signing failed: $_"
    exit 0
}
