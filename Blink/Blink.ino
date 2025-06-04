#define BLYNK_TEMPLATE_ID "TMPL61VJnw2ZY"
#define BLYNK_TEMPLATE_NAME "Monitoring Tanaman"
#define BLYNK_AUTH_TOKEN "E5kmtMTLlo7EULzXXCNhZA38A2LcGdFn"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp32.h>
#include <UniversalTelegramBot.h>

// WiFi credentials
char ssid[] = "TECNO POVA 4";
char pass[] = "11111111";

// Telegram
#define BOT_TOKEN "7977688911:AAG9mIKTsL7y28A-2v4QIwYmPkylp48T01E"
#define CHAT_ID "5677978794"
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// Sensor Pins
#define LDR_PIN 36
#define SOIL_PIN 39

BlynkTimer timer;

// LDR Averaging
const int ldrSampleSize = 10;
int ldrSamples[ldrSampleSize];
int ldrIndex = 0;

// Soil Moisture Averaging
const int soilSampleSize = 10;
int soilSamples[soilSampleSize];
int soilIndex = 0;

// Nilai acuan berdasarkan data dari kamu (YL-38/YL-69)
const int soilDry = 40;   // Kering
const int soilWet = 80;   // Basah

bool sudahKirimPeringatan = false;

void sendSensor() {
  // === LDR ===
  int rawLDR = analogRead(LDR_PIN);
  ldrSamples[ldrIndex] = rawLDR;
  ldrIndex = (ldrIndex + 1) % ldrSampleSize;

  int totalLDR = 0;
  for (int i = 0; i < ldrSampleSize; i++) totalLDR += ldrSamples[i];
  int avgLDR = totalLDR / ldrSampleSize;

  Serial.print("LDR: "); Serial.println(avgLDR);
  Blynk.virtualWrite(V0, avgLDR);
  Blynk.virtualWrite(V1, avgLDR);

  // === Soil Moisture ===
  int rawSoil = analogRead(SOIL_PIN);
  soilSamples[soilIndex] = rawSoil;
  soilIndex = (soilIndex + 1) % soilSampleSize;

  int totalSoil = 0;
  for (int i = 0; i < soilSampleSize; i++) totalSoil += soilSamples[i];
  int avgSoil = totalSoil / soilSampleSize;

  // Konversi ke persentase: basah = 100%, kering = 0%
  int soilPercent = map(avgSoil, soilWet, soilDry, 100, 0);
  soilPercent = constrain(soilPercent, 0, 100);

  Serial.print("Soil: "); Serial.print(avgSoil);
  Serial.print(" -> "); Serial.print(soilPercent); Serial.println("%");

  Blynk.virtualWrite(V2, soilPercent);

  // === Telegram Notification ===
  if (soilPercent < 30 && !sudahKirimPeringatan) {
    String msg = "⚠️ *Peringatan!*\nKelembapan tanah rendah: " + String(soilPercent) + "%\nTanaman perlu disiram.";
    Serial.println("⚠️ Kelembapan < 30% - kirim notifikasi Telegram...");

    bool success = bot.sendMessage(CHAT_ID, msg, "Markdown");

    if (success) {
      Serial.println("✅ Notifikasi Telegram berhasil dikirim!");
    } else {
      Serial.println("❌ Gagal mengirim notifikasi Telegram!");
    }

    sudahKirimPeringatan = true;
  } else if (soilPercent >= 30 && sudahKirimPeringatan) {
    sudahKirimPeringatan = false;
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  secured_client.setInsecure(); // Abaikan verifikasi sertifikat SSL

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi Terhubung");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Inisialisasi data awal untuk averaging
  for (int i = 0; i < ldrSampleSize; i++) {
    ldrSamples[i] = analogRead(LDR_PIN);
  }
  for (int i = 0; i < soilSampleSize; i++) {
    soilSamples[i] = analogRead(SOIL_PIN);
  }

  timer.setInterval(2000L, sendSensor); // Setiap 2 detik
}

void loop() {
  Blynk.run();
  timer.run();
}
