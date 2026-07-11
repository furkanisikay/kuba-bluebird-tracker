# Proje bağlamı (Claude hafızasından taşındı)

Bu dosya, önceki Claude Code oturumlarında toplanan proje bağlamının bir
kopyasıdır. Kaynak: `kuba-bluebird-tracker-project` hafıza kaydı
(2026-07-10 tarihinde başlatıldı, 2026-07-11'de bu repoya taşındı).

> Not: Kapsam bilinçli olarak bu projeye özel tutuldu - kullanıcının diğer,
> ilgisiz projelere ait hafıza kayıtları (PDF pipeline, video pipeline vb.)
> bu genel/GitHub'a açık repoya taşınmadı.

---

Ongoing project (started 2026-07-10): GPS takip + hırsızlık alarmı + telemetri
+ telefon entegrasyonu for user's **Kuba Bluebird Pro 50cc, 2025** scooter.
Phone: **Samsung Galaxy S24 FE**.

Hardware on hand: **Deneyap Mini** (ESP32-S2 — verified: WiFi-only, NO
Bluetooth radio at all) and **Radiolink SE100** GPS (5V ±5% supply; UART NMEA
at 9600 baud on M8N batch / 38400 on ~2023+ M10 batch; compass on separate
I2C; UART logic level undocumented — measure before wiring; compass chip
varies by batch, identify via I2C scan).

Key verified decisions/facts:

- Full Deneyap lineup checked. **Recommended main board: DENEYAP Kart
  (original)** — ESP32-WROVER-E, BT Classic+BLE, and crucially an onboard
  **LSM6DSM 6-axis IMU** (wake-on-motion for deep-sleep park mode → no
  MPU6050 needed) + MEMS mic + magnetic sensor. Kart 1A = same MCU/BT but SD
  card instead of IMU. Kart 1A v2 = ESP32-S3, BLE-only (no BT Classic). Kart
  G = ESP32-C3, BLE-only and **only 2 hardware UARTs** (can't do
  GPS+GSM+K-line at once). Mini/Mini v2 = ESP32-S2, no BT at all.
- Deep sleep 5 µA on ESP32 and C3 per official Deneyap power pages. (Deneyap
  ESP32 page lists light sleep as "0.8 µA" — unit typo, Espressif says
  0.8 mA.)
- Official Bluebird manual = **carbureted + CDI** (no ECU/diagnostic bus
  expected; "Pro 2025" EFI status unconfirmed — user to check for fuel-pump
  prime sound). If EFI: likely Delphi MT05-class, K-line ISO 14230 @10400
  baud, ESP32 + L9637D + muki01/OBD2_KLine_Library.
- Manual PDF downloaded to
  `C:\Users\furka\Desktop\Kuba_Bluebird_Kullanim_Kilavuzu.pdf` from Kuba's
  official Google Drive (file id 1xu5jOHbvTikZQ8vRNiOKYRHJpyzKMAQK). No
  separate "Bluebird Pro" manual published.
- Scooter battery: 12V 7Ah (YTX7A-BS). Cellular: Turkey 2G licensed until
  30 Apr 2029 (Turkcell/Vodafone); SIM800L must be **IMEI-registered** (BTK)
  or gets blacklisted ~90 days; NB-IoT/M2M SIMs are corporate-only in Turkey.
- Remote engine cut: only when stationary (GPS speed 0 + ignition off), relay
  on CDI/starter (carb) or fuel pump (EFI).

## MVP #1 (bu repo, 2026-07-11)

- Deneyap Mini + Radiolink SE100 test kurulumu masaüstü PC'ye USB ile bağlı
  (COM7/COM8, ESP32-S2 native USB CDC — reset sonrası port numarası
  değişebiliyor).
- Firmware: `firmware/kuba_tracker_mvp/` — TinyGPS++ ile NMEA parse,
  ESP32_HTTPS_Server (fhessel, öz-imzalı sertifika) ile `https://<ip>/`
  üzerinden canlı konum sayfası.
- `deneyap:esp32` Arduino core 2.0.1, `ESP32_HTTPS_Server@1.0.0` kütüphanesi
  `hwcrypto/sha.h` başlığı kalktığı için elle yamalanması gerekiyor (bkz.
  `firmware/kuba_tracker_mvp/patches/`).
- Derleme başarılı, cihaza yüklendi ve seri monitörden çalıştığı doğrulandı.
  Gerçek Wi-Fi bilgileri (`secrets.h`) ve gerçek GPS kablolama pinleri
  (`GPS_RX_PIN`/`GPS_TX_PIN`) kullanıcıdan alınmayı bekliyor — bunlar
  olmadan HTTPS sunucusuna gerçek cihazdan bağlanıp test tamamlanamaz.
