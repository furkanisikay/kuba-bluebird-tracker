// Kuba Bluebird Tracker - MVP #1
// Board: Deneyap Mini (ESP32-S2, WiFi only, no BT)
// GPS: Radiolink SE100 (UART NMEA, default 9600 baud on M8N)
//
// Reads NMEA sentences from the GPS over UART2, parses them with TinyGPS++,
// joins the configured Wi-Fi network, and serves a small HTTPS page that
// shows the current fix (lat/lon/speed/sats/HDOP) on an embedded Google Map,
// auto-refreshing every couple of seconds.
//
// Wiring (adjust GPS_RX_PIN/GPS_TX_PIN to whatever you actually wired):
//   Deneyap Mini 3V3  -> confirm SE100 accepts 3V3 logic before wiring (module needs 5V supply)
//   Deneyap Mini GND  -> SE100 GND
//   Deneyap Mini GPIO (GPS_RX_PIN) -> SE100 TX
//   Deneyap Mini GPIO (GPS_TX_PIN) -> SE100 RX (only needed if you send config commands)

#include <WiFi.h>
#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <TinyGPS++.h>
#include "cert.h"
#include "secrets.h"

using namespace httpsserver;

// ---- Config ---------------------------------------------------------------
static const int GPS_RX_PIN = 18;   // Deneyap Mini pin wired to SE100 TX
static const int GPS_TX_PIN = 17;   // Deneyap Mini pin wired to SE100 RX
static const uint32_t GPS_BAUD = 9600;

HardwareSerial GPSSerial(1);
TinyGPSPlus gps;

SSLCert *cert = nullptr;
HTTPSServer *secureServer = nullptr;

unsigned long lastFixMillis = 0;
unsigned long sentenceCount = 0;

// ---- Web page ---------------------------------------------------------------
const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Kuba Bluebird Tracker</title>
<style>
  body { font-family: system-ui, sans-serif; margin: 0; background: #10151a; color: #e8edf2; }
  header { padding: 12px 16px; background: #161d24; border-bottom: 1px solid #232c34; }
  h1 { font-size: 16px; margin: 0; }
  #status { font-size: 12px; color: #9ab; margin-top: 4px; }
  #map { height: 60vh; width: 100%; }
  #panel { padding: 12px 16px; display: grid; grid-template-columns: repeat(auto-fit, minmax(120px,1fr)); gap: 8px; }
  .stat { background: #161d24; border-radius: 8px; padding: 8px 10px; }
  .stat .label { font-size: 11px; color: #8ea; opacity: .7; }
  .stat .value { font-size: 18px; font-weight: 600; }
  .ok { color: #5fd07a; } .bad { color: #e2664a; }
</style>
</head>
<body>
<header>
  <h1>Kuba Bluebird Tracker - MVP</h1>
  <div id="status">bekleniyor...</div>
</header>
<div id="map"></div>
<div id="panel">
  <div class="stat"><div class="label">Fix</div><div class="value" id="fix">-</div></div>
  <div class="stat"><div class="label">Lat</div><div class="value" id="lat">-</div></div>
  <div class="stat"><div class="label">Lon</div><div class="value" id="lon">-</div></div>
  <div class="stat"><div class="label">Hiz (km/h)</div><div class="value" id="spd">-</div></div>
  <div class="stat"><div class="label">Uydu</div><div class="value" id="sat">-</div></div>
  <div class="stat"><div class="label">HDOP</div><div class="value" id="hdop">-</div></div>
  <div class="stat"><div class="label">Yon</div><div class="value" id="course">-</div></div>
  <div class="stat"><div class="label">Son fix</div><div class="value" id="age">-</div></div>
</div>
<script>
let map, marker;
function initMap(lat, lon) {
  map = new Map(document.getElementById('map'), { lat, lon });
}
// Basit, API key gerektirmeyen harita: OpenStreetMap embed + link, artı
// "Google Haritalarda ac" linki. Google Maps JS API key gerektirdigi icin
// MVP'de iframe tabanli OSM kullaniyoruz; embed src her fix'te guncellenir.
function setMapFrame(lat, lon) {
  const d = 0.004;
  const bbox = [lon-d, lat-d, lon+d, lat+d].join('%2C');
  const src = `https://www.openstreetmap.org/export/embed.html?bbox=${bbox}&layer=mapnik&marker=${lat}%2C${lon}`;
  const frame = document.getElementById('mapFrame');
  if (frame) frame.src = src;
}
document.getElementById('map').innerHTML =
  '<iframe id="mapFrame" style="width:100%;height:100%;border:0" src=""></iframe>' +
  '<div style="padding:6px 10px;font-size:12px"><a id="gmaps" target="_blank" style="color:#7ab8ff">Google Haritalarda ac</a></div>';

async function poll() {
  try {
    const r = await fetch('/api/fix', { cache: 'no-store' });
    const d = await r.json();
    document.getElementById('status').textContent =
      d.hasFix ? ('WiFi: ' + d.ip) : ('WiFi: ' + d.ip + ' - GPS fix bekleniyor');
    document.getElementById('fix').textContent = d.hasFix ? 'VAR' : 'YOK';
    document.getElementById('fix').className = 'value ' + (d.hasFix ? 'ok' : 'bad');
    document.getElementById('lat').textContent = d.hasFix ? d.lat.toFixed(6) : '-';
    document.getElementById('lon').textContent = d.hasFix ? d.lon.toFixed(6) : '-';
    document.getElementById('spd').textContent = d.hasFix ? d.speedKmh.toFixed(1) : '-';
    document.getElementById('sat').textContent = d.sats;
    document.getElementById('hdop').textContent = d.hdop.toFixed(1);
    document.getElementById('course').textContent = d.hasFix ? d.courseDeg.toFixed(0) + '°' : '-';
    document.getElementById('age').textContent = d.ageMs < 999999 ? (d.ageMs/1000).toFixed(1) + 's' : '-';
    if (d.hasFix) {
      setMapFrame(d.lat, d.lon);
      document.getElementById('gmaps').href =
        'https://www.google.com/maps?q=' + d.lat + ',' + d.lon;
    }
  } catch (e) {
    document.getElementById('status').textContent = 'baglanti hatasi: ' + e;
  }
}
poll();
setInterval(poll, 2000);
</script>
</body>
</html>
)HTML";

void handleRoot(HTTPRequest *req, HTTPResponse *res) {
  res->setHeader("Content-Type", "text/html; charset=utf-8");
  res->print(INDEX_HTML);
}

void handleApiFix(HTTPRequest *req, HTTPResponse *res) {
  res->setHeader("Content-Type", "application/json");
  bool hasFix = gps.location.isValid() && gps.location.isUpdated() || gps.location.isValid();
  unsigned long age = gps.location.isValid() ? gps.location.age() : 999999999UL;

  String json = "{";
  json += "\"hasFix\":" + String(gps.location.isValid() ? "true" : "false") + ",";
  json += "\"lat\":" + String(gps.location.isValid() ? gps.location.lat() : 0.0, 6) + ",";
  json += "\"lon\":" + String(gps.location.isValid() ? gps.location.lng() : 0.0, 6) + ",";
  json += "\"speedKmh\":" + String(gps.speed.isValid() ? gps.speed.kmph() : 0.0, 2) + ",";
  json += "\"courseDeg\":" + String(gps.course.isValid() ? gps.course.deg() : 0.0, 1) + ",";
  json += "\"sats\":" + String(gps.satellites.isValid() ? gps.satellites.value() : 0) + ",";
  json += "\"hdop\":" + String(gps.hdop.isValid() ? gps.hdop.hdop() : 99.9, 2) + ",";
  json += "\"ageMs\":" + String(age) + ",";
  json += "\"sentences\":" + String(sentenceCount) + ",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
  json += "}";
  res->print(json);
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("WiFi'a baglaniliyor: %s", WIFI_SSID);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
    if (millis() - start > 20000) {
      Serial.println("\nWiFi baglanamadi, yeniden deneniyor...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      start = millis();
    }
  }
  Serial.println();
  Serial.print("WiFi baglandi, IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nKuba Bluebird Tracker MVP baslatiliyor...");

  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  connectWiFi();

  cert = new SSLCert(
    const_cast<uint8_t*>(server_cert_pem), server_cert_pem_len,
    const_cast<uint8_t*>(server_key_pem), server_key_pem_len
  );
  secureServer = new HTTPSServer(cert);

  ResourceNode *nodeRoot = new ResourceNode("/", "GET", &handleRoot);
  ResourceNode *nodeApiFix = new ResourceNode("/api/fix", "GET", &handleApiFix);
  secureServer->registerNode(nodeRoot);
  secureServer->registerNode(nodeApiFix);

  secureServer->start();
  if (secureServer->isRunning()) {
    Serial.println("HTTPS sunucu calisiyor. Tarayicidan ac:");
    Serial.print("  https://");
    Serial.println(WiFi.localIP());
    Serial.println("(Ozimzali sertifika oldugu icin tarayici uyari verecek, 'Gelismis > Yine de git' ile devam edin.)");
  } else {
    Serial.println("HTTPS sunucu baslatilamadi!");
  }
}

void loop() {
  while (GPSSerial.available()) {
    char c = GPSSerial.read();
    if (gps.encode(c)) {
      sentenceCount++;
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  secureServer->loop();

  static unsigned long lastLog = 0;
  if (millis() - lastLog > 5000) {
    lastLog = millis();
    Serial.printf("[GPS] fix=%s lat=%.6f lon=%.6f sats=%d hdop=%.1f cumleler=%lu\n",
      gps.location.isValid() ? "VAR" : "YOK",
      gps.location.lat(), gps.location.lng(),
      gps.satellites.isValid() ? gps.satellites.value() : 0,
      gps.hdop.isValid() ? gps.hdop.hdop() : 99.9,
      sentenceCount);
  }
}
