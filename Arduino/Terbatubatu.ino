#define BLYNK_TEMPLATE_ID "TMPL60ymRTh_8"
#define BLYNK_TEMPLATE_NAME "OJT Taman Depan"

#include <WiFi.h>
#include <WiFiMulti.h>
WiFiMulti wifiMulti;

#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// --- Sensor ---
#define DHTPIN 4
#define DHTTYPE DHT22 
#define SOIL_MOISTURE_PIN 34

// --- Relay (LOW = ON) ---
#define RELAY1_PIN 25
#define RELAY2_PIN 33
#define RELAY3_PIN 26   // Pompa manual

// --- Konstanta ---
const int SOIL_THRESHOLD = 40;
const unsigned long RELAY_DURATION = 420000; // 7 menit
const unsigned long RELAY_DELAY = 600000;    // 10 menit jeda

// === Blynk ===
char auth[] = "wBb_xogjLziuPSc9gBJdipsVmjWYgRui";

// --- Init Sensor & LCD ---
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- Variabel ---
unsigned long previousMillis = 0;
const long intervalSuhu = 2000;
const long intervalSoil = 3000;
const long intervalRelay = 2000;

int slide = 0;

float humidity = 0.0;
float temperature = 0.0;
int soilMoisturePercent = 0;

// Mode otomatis/manual
bool modeOtomatis = true;
bool relay1Manual = false;
bool relay2Manual = false;
bool relay3Manual = false;

// Status relay
bool relay1Status = false;
bool relay2Status = false;
bool relay3Status = false;

// Auto timer
unsigned long relayTimer = 0;
int relayStage = 0;

// Countdown aktif
long relayCountdown = 0;

// Countdown delay jeda setelah OFF
long delayCountdown = 0;

// Delay timer
bool delayActive = false;
unsigned long delayTimer = 0;

bool offlineMode = false;

// === Blynk Controls ===
BLYNK_WRITE(V3) { modeOtomatis = param.asInt(); }
BLYNK_WRITE(V4) { relay1Manual = param.asInt(); }
BLYNK_WRITE(V7) { relay2Manual = param.asInt(); }
BLYNK_WRITE(V11) { relay3Manual = param.asInt(); }

// ---- WI-FI MULTI AP INPUT ----
void initWiFiMulti() {
  wifiMulti.addAP("Aku denganya", "n7kfs8gs6j6dsda");
  wifiMulti.addAP("BBPVP SEMARANG", "BBPVPsmg1");
  wifiMulti.addAP("Kreasi", "Kreasi2022");
  wifiMulti.addAP("iPhone", "YYYYYYYY");
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Menghubungkan...");

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);

  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(RELAY3_PIN, HIGH);

  WiFi.mode(WIFI_STA);
  initWiFiMulti();

  unsigned long startAttemptTime = millis();
  while (wifiMulti.run() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    Serial.print(".");
    delay(250);
  }

  if (WiFi.isConnected()) {
    Blynk.config(auth);
    Blynk.connect();
    offlineMode = false;

    lcd.clear();
    lcd.print("WiFi Terhubung");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.SSID());
    delay(2000);

  } else {
    offlineMode = true;
    lcd.clear();
    lcd.print("OFFLINE MODE");
    lcd.setCursor(0, 1);
    lcd.print("WiFi gagal");
    delay(2000);
  }

  lcd.clear();
  lcd.print("Selamat Datang");
  lcd.setCursor(0, 1);
  lcd.print("di BBPVP SMG");
  delay(2500);
}

void loop() {

  if (!WiFi.isConnected()) {
    offlineMode = true;

    static unsigned long lastTry = 0;
    if (millis() - lastTry > 10000) {
      lastTry = millis();
      wifiMulti.run();
    }

  } else {
    offlineMode = false;
    if (!Blynk.connected()) Blynk.connect();
    Blynk.run();
  }

  unsigned long currentMillis = millis();

  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  int soilRaw = analogRead(SOIL_MOISTURE_PIN);
  soilMoisturePercent = map(soilRaw, 3200, 2250, 0, 100);
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

  bool currentAuto = modeOtomatis || offlineMode;

  // ======================= LOGIKA OTOMATIS ======================
  if (currentAuto) {

    if (soilMoisturePercent < SOIL_THRESHOLD) {

      if (relayStage == 0) {
        relayStage = 1;
        relayTimer = millis();
        delayActive = false;
      }

      // ================= Relay 1 =================
      if (relayStage == 1) {
        digitalWrite(RELAY1_PIN, LOW);   // ON
        relay1Status = true;

        relayCountdown = (RELAY_DURATION - (millis() - relayTimer)) / 1000;

        if (millis() - relayTimer >= RELAY_DURATION) {
          digitalWrite(RELAY1_PIN, HIGH); // OFF
          relay1Status = false;
          delayActive = true;
          delayTimer = millis();
          relayStage = 10;
          relayCountdown = 0;
        }
      }

      // ================= Delay 1 =================
      else if (relayStage == 10) {
        delayCountdown = (RELAY_DELAY - (millis() - delayTimer)) / 1000;

        if (millis() - delayTimer >= RELAY_DELAY) {
          delayActive = false;
          relayStage = 2;
          relayTimer = millis();
          delayCountdown = 0;
        }
      }

      // ================= Relay 2 =================
      else if (relayStage == 2) {
        digitalWrite(RELAY2_PIN, LOW);   // ON (INI YANG DIBENARKAN)
        relay2Status = true;

        relayCountdown = (RELAY_DURATION - (millis() - relayTimer)) / 1000;

        if (millis() - relayTimer >= RELAY_DURATION) {
          digitalWrite(RELAY2_PIN, HIGH); // OFF
          relay2Status = false;
          delayActive = true;
          delayTimer = millis();
          relayStage = 20;
          relayCountdown = 0;
        }
      }

      // ================= Delay 2 =================
      else if (relayStage == 20) {
        delayCountdown = (RELAY_DELAY - (millis() - delayTimer)) / 1000;

        if (millis() - delayTimer >= RELAY_DELAY) {
          delayActive = false;
          relayStage = 1;  // kembali ke Relay 1
          relayTimer = millis();
        }
      }

    } else {
      relayStage = 0;
      delayActive = false;
      digitalWrite(RELAY1_PIN, HIGH);
      digitalWrite(RELAY2_PIN, HIGH);
      relay1Status = false;
      relay2Status = false;
    }
  }

  // ======================= MANUAL MODE ========================
  else if (!offlineMode) {
    digitalWrite(RELAY1_PIN, relay1Manual ? LOW : HIGH);
    digitalWrite(RELAY2_PIN, relay2Manual ? LOW : HIGH);
    digitalWrite(RELAY3_PIN, relay3Manual ? LOW : HIGH);

    relay1Status = relay1Manual;
    relay2Status = relay2Manual;
    relay3Status = relay3Manual;
  }

  // ======================= BLYNK SEND =========================
  if (!offlineMode) {
    Blynk.virtualWrite(V0, temperature);
    Blynk.virtualWrite(V1, humidity);
    Blynk.virtualWrite(V2, soilMoisturePercent);

    Blynk.virtualWrite(V5, relay1Status ? "Relay 1 ON" : "Relay 1 OFF");
    Blynk.virtualWrite(V8, relay2Status ? "Relay 2 ON" : "Relay 2 OFF");
    Blynk.virtualWrite(V12, relay3Status ? "Pompa ON" : "Pompa OFF");

    Blynk.virtualWrite(V6, currentAuto ? "AUTO" : "MANUAL");

    Blynk.virtualWrite(V9, relay1Status || relay2Status ? relayCountdown : 0);
    Blynk.virtualWrite(V13, delayActive ? delayCountdown : 0);
  }

  // ======================= LCD SLIDE ==========================
  if (slide == 0 && currentMillis - previousMillis >= intervalSuhu) {
    previousMillis = currentMillis;
    slide = 1;

    lcd.clear();
    lcd.print(offlineMode ? "OFFLINE MODE" : "ONLINE MODE");
    lcd.setCursor(0, 1);
    lcd.print("H:");
    lcd.print(humidity, 1);
    lcd.print("% T:");
    lcd.print(temperature, 1);
  }

  else if (slide == 1 && currentMillis - previousMillis >= intervalSoil) {
    previousMillis = currentMillis;
    slide = 2;

    lcd.clear();
    lcd.print("Soil Moisture");
    lcd.setCursor(4, 1);
    lcd.print(soilMoisturePercent);
    lcd.print("%");
  }

  else if (slide == 2 && currentMillis - previousMillis >= intervalRelay) {
    previousMillis = currentMillis;
    slide = 0;

    lcd.clear();
    lcd.print("R1:");
    lcd.print(relay1Status ? "ON " : "OFF");
    lcd.setCursor(8, 0);
    lcd.print("R2:");
    lcd.print(relay2Status ? "ON" : "OFF");

    lcd.setCursor(0, 1);

    if (relay1Status || relay2Status) {
      lcd.print("CD:");
      lcd.print(relayCountdown);
      lcd.print("s");

    } else if (delayActive) {
      lcd.print("DELAY:");
      lcd.print(delayCountdown);
      lcd.print("s");

    } else {
      lcd.print("P:");
      lcd.print(relay3Status ? "ON" : "OFF");
    }
  }

  delay(30);
}
