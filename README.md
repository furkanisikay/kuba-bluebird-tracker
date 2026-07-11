# Kuba Bluebird Tracker

Kuba Bluebird Pro 50 (2025) scooter için DIY GPS takip / hırsızlık alarmı /
telemetri projesi. MVP #1: Deneyap Mini (ESP32-S2) + Radiolink SE100 GPS
modülünden konum okuyup, cihazın kendi kurduğu HTTPS sunucusundan aynı Wi-Fi
ağındaki bir tarayıcıya canlı konum göstermek.

## Donanım

- **Deneyap Mini** (ESP32-S2, sadece Wi-Fi, Bluetooth yok)
- **Radiolink SE100** GPS modülü (UART NMEA, M8N seride 9600 baud)
- Telefon: Samsung Galaxy S24 FE (uzun vadede kontrol uygulaması hedefi)

Diğer donanım/karar geçmişi için proje notlarına bakın: [`docs/project-context.md`](docs/project-context.md).

## MVP #1 kapsamı

- Deneyap Mini, kullanıcının Wi-Fi ağına (STA modu) bağlanır.
- SE100'den NMEA cümleleri UART üzerinden okunup TinyGPS++ ile parse edilir.
- Cihaz kendi HTTPS sunucusunu (öz-imzalı sertifika) `443` portunda açar.
- Aynı ağdaki bir tarayıcıdan `https://<cihaz-ip>` adresine gidilince konum,
  hız, uydu sayısı, HDOP ve harita (OpenStreetMap embed + "Google Haritada aç"
  linki) 2 saniyede bir güncellenerek gösterilir.

## Kurulum

### 1. Araç zinciri

```powershell
winget install --id ArduinoSA.CLI -e
arduino-cli config set board_manager.additional_urls https://raw.githubusercontent.com/deneyapkart/deneyapkart-arduino-core/master/package_deneyapkart_index.json
arduino-cli core update-index
arduino-cli core install deneyap:esp32
arduino-cli lib install "TinyGPSPlus" "ESP32_HTTPS_Server"
```

`ESP32_HTTPS_Server` kütüphanesi bu Deneyap core sürümüyle derlenmiyor -
[`firmware/kuba_tracker_mvp/patches/ESP32_HTTPS_Server_sha_fix.md`](firmware/kuba_tracker_mvp/patches/ESP32_HTTPS_Server_sha_fix.md)
dosyasındaki iki satırlık yamayı uygulayın.

### 2. Sertifika ve Wi-Fi bilgileri

```powershell
cd firmware/kuba_tracker_mvp
./gen_cert.ps1                       # cert.h üretir (gitignored)
copy secrets.example.h secrets.h     # WIFI_SSID / WIFI_PASSWORD doldurun (gitignored)
```

### 3. GPS kablolama

`kuba_tracker_mvp.ino` içindeki `GPS_RX_PIN` / `GPS_TX_PIN` sabitlerini
Deneyap Mini üzerinde SE100'ü gerçekte taktığınız pinlere göre güncelleyin.
**SE100'ün UART lojik seviyesini kablolamadan önce ölçün** (5V besleme ama
UART seviyesi seriye göre değişebilir).

### 4. Derle ve yükle

```powershell
arduino-cli compile --fqbn deneyap:esp32:deneyapmini firmware/kuba_tracker_mvp
arduino-cli upload -p COM7 --fqbn deneyap:esp32:deneyapmini firmware/kuba_tracker_mvp
```

Seri port numarasını (`COM7`/`COM8`...) `arduino-cli board list` ile doğrulayın -
ESP32-S2 native USB CDC, resetten sonra port numarasını değiştirebiliyor.

### 5. Test

Seri monitörden (115200 baud) `WiFi baglandi, IP: ...` satırını görün, sonra
telefonunuzdan/bilgisayarınızdan aynı Wi-Fi ağında `https://<o-ip>` adresine
gidin. Öz-imzalı sertifika olduğu için tarayıcı uyarı verecek - "Gelişmiş >
Yine de devam et" ile geçin.

## Yol haritası (MVP sonrası)

- Hırsızlık alarmı: hareket algılama (Kart üzerindeki IMU veya ayrı MPU6050) + bildirim
- Uzaktan motor kesme (durağan + kontak kapalıyken röle)
- SIM800L üzerinden GSM/telemetri (2G Türkiye'de 2029'a kadar lisanslı;
  IMEI kaydı gerekiyor)
- Telefon uygulaması entegrasyonu

## Proje geçmişi

Bu repo, önceki bir Claude oturumunda toplanan proje bağlamını ve kararları
[`docs/project-context.md`](docs/project-context.md) ve
[`session/`](session/) altında taşır.
