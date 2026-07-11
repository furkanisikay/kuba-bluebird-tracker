# Gerekli kütüphane yaması: ESP32_HTTPS_Server

`ESP32_HTTPS_Server@1.0.0` (fhessel), Deneyap ESP32 core 2.0.1'in artık içermediği
eski `hwcrypto/sha.h` başlığını kullanıyor ve derleme `fatal error: hwcrypto/sha.h:
No such file or directory` ile patlıyor. Websocket handshake'te SHA1 hesaplamak
için kullanılıyor (bu projede websocket kullanılmıyor ama kod yine de derlenmeli).

Kütüphaneyi kurduktan sonra (Arduino IDE Library Manager veya
`arduino-cli lib install "ESP32_HTTPS_Server"`), şu iki değişikliği elle uygula:

**`src/HTTPConnection.hpp`** - satır ~9:
```cpp
- #include <hwcrypto/sha.h>
+ #include <mbedtls/sha1.h>
```

**`src/HTTPConnection.cpp`** - `websocketKeyResponseHash` fonksiyonu içinde:
```cpp
- esp_sha(SHA1, (uint8_t*)newKey.data(), newKey.length(), shaData);
+ mbedtls_sha1_ret((const unsigned char*)newKey.data(), newKey.length(), shaData);
```

Kütüphane genelde şurada kurulur: `Documents/Arduino/libraries/ESP32_HTTPS_Server/`.
