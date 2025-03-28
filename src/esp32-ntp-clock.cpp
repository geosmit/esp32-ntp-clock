// Learn about the ESP32 WiFi simulation in
// https://docs.wokwi.com/guides/esp32-wifi

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 16, 2);

#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     3600
#define UTC_OFFSET_DST 1
#define SDA_PIN 4
#define SCL_PIN 5

byte bitcoinSymbol[8] = {
  B00100,
  B11110,
  B10101,
  B11110,
  B10101,
  B11110,
  B00100,
  B00000
};

byte up[8] = {
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00100,
  B00100,
  B00100
};

byte down[8] = {
  B00100,
  B00100,
  B00100,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100
};

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
    LCD.print("Connection Err"); // Změněno z println na print
    return;
  }

  LCD.setCursor(0, 0);
  LCD.print(&timeinfo, "$/BTC   " "%H:%M:%S"); // Změněno z println na print
  //LCD.setCursor(0, 1);
  //LCD.print(&timeinfo, "%d/%m/%Y   %Z"); // Změněno z println na print
}

// Declare lastPrice as a global variable
int lastPrice = 0;

void fetchBitcoinPrice() {
  HTTPClient http;
  http.begin("https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd,czk&include_24hr_change=true");
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    String payload = http.getString();
    //StaticJsonDocument<256> doc;
    JsonDocument doc;
    Serial.println(payload);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      int bitcoinPrice = doc["bitcoin"]["usd"];
      float change24hUSD = doc["bitcoin"]["usd_24h_change"];
      LCD.setCursor(0, 1);
      LCD.print(bitcoinPrice); 
      LCD.print(" ");
      if (lastPrice == 0 || lastPrice == bitcoinPrice) {
      LCD.print("-");
    } else if (lastPrice < bitcoinPrice) {
      LCD.write(1);
    } else {
      LCD.write(2);
    }
    LCD.print(" ");
    LCD.print(change24hUSD, 2); // Zobrazení s dvěma desetinnými místy
    LCD.print("%      ");
    lastPrice = bitcoinPrice;
    } else {
      LCD.setCursor(0, 1);
      LCD.print("JSON Error"); // Změněno z println na print
    }
  } else {
    LCD.setCursor(0, 1);
    LCD.print("HTTP Error"); // Změněno z println na print
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN); // Inicializace I²C sběrnice
  
  LCD.init();
  LCD.createChar(0, bitcoinSymbol);
  LCD.createChar(1, up);
  LCD.createChar(2, down);
  LCD.backlight();
  LCD.setCursor(0, 0);
  LCD.print("Connecting to ");
  LCD.setCursor(0, 1);
  LCD.print("WiFi ");
  
  WiFi.begin("Wokwi-GUEST", "", 6);
  //WiFi.begin("B53C50", "k@rpE_d1eM", 6);
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
  LCD.print("Online  ");
  LCD.setCursor(0, 1);
  LCD.print("Updating...");

  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

  
  // Načtení a zobrazení kurzu Bitcoinu při prvním spuštění
  //LCD.clear();
    fetchBitcoinPrice();
}

void loop() {
  static unsigned long lastBitcoinUpdate = 0; // Čas poslední aktualizace kurzu BTC
  unsigned long currentMillis = millis();

  // Aktualizace času na displeji v reálném čase
  printLocalTime();

  // Aktualizace kurzu BTC každých 60 sekund
  if (currentMillis - lastBitcoinUpdate >= 120000) { // 60 000 ms = 1 minuta
    fetchBitcoinPrice();
    lastBitcoinUpdate = currentMillis;
  }

  delay(1000); // Zpoždění 1 sekundu pro plynulé aktualizace času
}
