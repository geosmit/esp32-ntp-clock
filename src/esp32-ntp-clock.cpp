// Learn about the ESP32 WiFi simulation in
// https://docs.wokwi.com/guides/esp32-wifi

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 16, 2);

#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     1
#define UTC_OFFSET_DST 1
#define SDA_PIN 8
#define SCL_PIN 9

void spinner() {
  static int8_t counter = 0;
  const char* glyphs = "\xa1\xa5\xdb";
  LCD.setCursor(15, 1);
  LCD.print(glyphs[counter++]);
  if (counter == strlen(glyphs)) {
    counter = 0;
  }
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    LCD.setCursor(0, 1);
    LCD.println("Connection Err");
    return;
  }

  LCD.setCursor(8, 0);
  LCD.println(&timeinfo, "%H:%M:%S");

  //LCD.setCursor(0, 1);
  //LCD.println(&timeinfo, "%d/%m/%Y   %Z");
}

void fetchBitcoinPrice() {
  HTTPClient http;
  http.begin("https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd"); // API pro získání kurzu Bitcoinu
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) { // Kontrola, zda je odpověď úspěšná
    String payload = http.getString();
    DynamicJsonDocument doc(256); // Použití DynamicJsonDocument místo StaticJsonDocument
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      int bitcoinPrice = doc["bitcoin"]["usd"]; // Získání kurzu a převod na celé číslo
      LCD.setCursor(0, 1);
      LCD.printf("BTC: %d USD  ", bitcoinPrice); // Zobrazení na displeji bez desetinných míst
    } else {
      LCD.setCursor(0, 1);
      LCD.println("JSON Error");
    }
  } else {
    LCD.setCursor(0, 1);
    LCD.println("HTTP Error");
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN); // Inicializace I²C sběrnice
  
  LCD.init();
  LCD.backlight();
  LCD.setCursor(0, 0);
  LCD.print("Connecting to ");
  LCD.setCursor(0, 1);
  LCD.print("WiFi ");

  WiFi.begin("B53C50", "k@rpE_d1eM", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    spinner();
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  LCD.clear();
  LCD.setCursor(0, 0);
  LCD.println("Online  ");
  LCD.setCursor(0, 1);
  LCD.println("Updating...");

  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
}

void loop() {
  static unsigned long lastBitcoinUpdate = 0; // Čas poslední aktualizace kurzu BTC
  unsigned long currentMillis = millis();

  // Aktualizace času na displeji v reálném čase
  printLocalTime();

  // Aktualizace kurzu BTC každých 60 sekund
  if (currentMillis - lastBitcoinUpdate >= 60000) { // 60 000 ms = 1 minuta
    fetchBitcoinPrice();
    lastBitcoinUpdate = currentMillis;
  }

  delay(1000); // Zpoždění 1 sekundu pro plynulé aktualizace času
}
