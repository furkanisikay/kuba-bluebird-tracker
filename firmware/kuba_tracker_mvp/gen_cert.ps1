# Generates a fresh self-signed TLS cert/key for the ESP32 HTTPS server and
# embeds them as cert.h. Run this once before the first compile (cert.h and
# the .pem files are gitignored - never commit a private key).
$ErrorActionPreference = "Stop"
$dir = $PSScriptRoot
Push-Location $dir
try {
  & openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 3650 -nodes `
    -subj "/C=TR/O=KubaBluebirdTracker/CN=kuba-tracker.local"

  node gen_cert_header.js
  Write-Host "cert.h generated. You can now compile the sketch."
} finally {
  Pop-Location
}
