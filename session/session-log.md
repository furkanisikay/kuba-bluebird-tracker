# Oturum günlüğü — 2026-07-11 MVP #1 kurulumu

Bu dosya, bu repoyu oluşturan Claude Code oturumunda yapılanların özetidir.

## Ortam
- Windows, PowerShell + Git Bash, `D:\repos\kuba-bluebird-tracker`
- `winget install ArduinoSA.CLI` ile arduino-cli 1.5.1 kuruldu
  (`C:\Program Files\Arduino CLI\arduino-cli.exe`)
- Deneyap board index eklendi:
  `https://raw.githubusercontent.com/deneyapkart/deneyapkart-arduino-core/master/package_deneyapkart_index.json`
- `deneyap:esp32@2.0.1` core, `TinyGPSPlus@1.0.3`, `ESP32_HTTPS_Server@1.0.0`
  kütüphaneleri kuruldu.
- FQBN: `deneyap:esp32:deneyapmini`. Bağlı cihaz: ESP32-S2FH4,
  MAC `7c:df:a1:93:17:64`, ilk seri port COM7, upload sonrası reset ile COM8'e
  geçti (native USB CDC).

## Karşılaşılan ve çözülen sorunlar
1. **`hwcrypto/sha.h` bulunamadı** — `ESP32_HTTPS_Server` kütüphanesi eski
   arduino-esp32 core'a göre yazılmış; bu header yeni Deneyap core'da yok.
   Çözüm: `HTTPConnection.hpp`'de include'u `mbedtls/sha1.h` ile değiştirip,
   `HTTPConnection.cpp`'de `esp_sha(SHA1, ...)` çağrısını
   `mbedtls_sha1_ret(...)` ile değiştirdik (websocket handshake'te kullanılan,
   bu projede aktif olmayan bir kod yolu). Detay:
   `firmware/kuba_tracker_mvp/patches/ESP32_HTTPS_Server_sha_fix.md`.
2. **`const uint8_t*` -> `unsigned char*` dönüşüm hatası** — `SSLCert`
   constructor'ı `unsigned char*` bekliyor, `const_cast` eklendi.
3. **COM7 "device does not exist" hatası** — geçiciydi, tekrar denemede
   düzeldi (muhtemelen port bir önceki komut tarafından anlık meşguldü).

## Yapılanlar
- `firmware/kuba_tracker_mvp/` altında MVP firmware yazıldı: WiFi STA
  bağlantısı, `HardwareSerial` üzerinden SE100 NMEA okuma + TinyGPS++ parse,
  öz-imzalı sertifikayla `ESP32_HTTPS_Server` üzerinden `/` (harita sayfası)
  ve `/api/fix` (JSON) endpoint'leri.
- Öz-imzalı sertifika `gen_cert.ps1` + `gen_cert_header.js` ile üretilip
  `cert.h`'ye gömüldü (gitignored — repo'ya private key committen madık).
- Derleme başarılı (1076502 bayt / %82 flash), cihaza yüklendi, seri
  monitörden firmware'in çalıştığı doğrulandı (WiFi bağlantı denemesi
  döngüsü görüldü — `secrets.h` içinde hâlâ placeholder credential var).
- Git repo oluşturuldu, GitHub'a public olarak yayımlandı:
  https://github.com/furkanisikay/kuba-bluebird-tracker

## Tamamlanmamış / kullanıcıdan beklenen
- **Gerçek Wi-Fi SSID/parola** — `firmware/kuba_tracker_mvp/secrets.h`
  içine yazılmalı (gitignored, GitHub'a gitmez).
- **Gerçek GPS kablolama pinleri** — `.ino` dosyasındaki `GPS_RX_PIN` (18) ve
  `GPS_TX_PIN` (17) şu an varsayılan/tahmini; kullanıcının SE100'ü fiziksel
  olarak taktığı pinlerle güncellenmesi gerekiyor.
- Bu ikisi netleşince: `arduino-cli compile` + `upload`, sonra telefon/PC'den
  `https://<cihaz-ip>` adresine gidip ilk canlı testi doğrulamak kaldı.
