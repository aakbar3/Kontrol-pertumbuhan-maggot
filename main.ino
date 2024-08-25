#include <ThingerESP8266.h>
#include <Adafruit_AHTX0.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiManager.h>

ThingerESP8266 thing("Madar", "esp8266maggot", "maggot2121");

LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_AHTX0 aht;
WiFiManager wifiManager;

#define Heater_PIN 14 // heater di pin d5
#define Mist_PIN 12 // mist di pin d6 

int heat = 0, mist = 0;
float suhu, kelembapan;
String suhuStr, kelembapanStr;

unsigned long lastDataUpdateTime = 0, lastControlUpdateTime = 0;
const unsigned long dataUpdateInterval = 1000, controlUpdateInterval = 60000; 

void configModeCallback(WiFiManager *myWiFiManager) {
  lcd.clear();
  lcd.print("AP Mode: ");
  lcd.setCursor(0, 1);
  lcd.print(myWiFiManager->getConfigPortalSSID());
}

void setup() {
  lcd.init();  lcd.backlight();
  aht.begin();

  pinMode(Heater_PIN, OUTPUT);   
  pinMode(Mist_PIN, OUTPUT);
  digitalWrite(Heater_PIN, LOW);
  digitalWrite(Mist_PIN, LOW);

  lcd.clear();
  lcd.print("Connecting...");

  wifiManager.setTimeout(180); // Set timeout ke 3 menit jika tidak terhubung
  wifiManager.setAPCallback(configModeCallback);
  if (!wifiManager.autoConnect("Maggot_AP")) { delay(2000); ESP.restart(); }

  // Jika berhasil terhubung, tampilkan pesan sukses dan SSID di LCD
  lcd.clear();
  lcd.print("Connected to:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.SSID()); // Tampilkan nama WiFi yang terhubung
  delay(3000); // Tampilkan pesan selama 3 detik
  lcd.clear();

  tampilkanData();

  thing["Kondisi kandang"] >> [](pson &out) {  
    out["suhu"] = suhuStr;
    out["kelembapan"] = kelembapanStr;
    out["mist"] = mist;
    out["heater"] = heat;
  };
}

void loop() {
  unsigned long currentTime = millis();

  // Periksa apakah sudah waktunya untuk memperbarui data
  if (currentTime - lastDataUpdateTime >= dataUpdateInterval) {
    lastDataUpdateTime = currentTime;
    thing.handle();
  }

  // Periksa apakah sudah waktunya untuk mengontrol alat
  if (currentTime - lastControlUpdateTime >= controlUpdateInterval) {
    lastControlUpdateTime = currentTime;
    tampilkanData();
    kontrolAlat();
    kirimData();
  }
}

void tampilkanData() {
  sensors_event_t humidity, temp;
  
  bool success = aht.getEvent(&humidity, &temp); // Membaca suhu dan kelembapan

  suhu = success ? round(temp.temperature * 10) / 10.0 : 0;
  kelembapan = success ? round(humidity.relative_humidity) : 0;
  
  suhuStr = String(suhu, 1);
  kelembapanStr = String(kelembapan, 0);

  lcd.clear();
  lcd.setCursor(0, 0);  lcd.printf("Temp: %s C", suhuStr.c_str());
  lcd.setCursor(0, 1);  lcd.printf("Humi: %s %%", kelembapanStr.c_str());
}

void kontrolAlat() {
  // Prioritas: Heater diutamakan jika suhu di bawah 30 atau kelembapan di atas 90
  if (suhu < 30 || kelembapan > 90) {
    digitalWrite(Heater_PIN, HIGH);
    digitalWrite(Mist_PIN, LOW);
    heat = 1;
    mist = 0;
  } 
  else if (suhu >= 36 || kelembapan <= 60) { // nyala mistmaker
    digitalWrite(Mist_PIN, HIGH);
    digitalWrite(Heater_PIN, LOW);
    heat = 0;
    mist = 1;
  } 
  else {
    digitalWrite(Heater_PIN, LOW);
    digitalWrite(Mist_PIN, LOW);
    heat = 0;
    mist = 0;
  }
}

void kirimData() {
  pson data;
  data["suhu"] = suhuStr;
  data["kelembapan"] = kelembapanStr;
  data["mist"] = mist;
  data["heater"] = heat;

  thing.write_bucket("Skripsi", data);
}
