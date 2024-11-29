#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// Ganti dengan SSID dan password Wi-Fi Anda
const char* ssid = "P1";
const char* password = "69696969";

// Ganti dengan URL Supabase API dan API Key
const char* supabaseSensor = "https://oorjicuehjgqcidtcswo.supabase.co/rest/v1/sensor";
const char* supabaseLoc = "https://oorjicuehjgqcidtcswo.supabase.co/rest/v1/loc";
const char* supabaseTele = "https://oorjicuehjgqcidtcswo.supabase.co/rest/v1/tele";
const char* apiKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im9vcmppY3VlaGpncWNpZHRjc3dvIiwicm9sZSI6ImFub24iLCJpYXQiOjE3MzIyNTg1MjAsImV4cCI6MjA0NzgzNDUyMH0.TVZsSG5zYao7XeoEzfJ4nyNIyNvqULFuj8RIWGymhac";

// Bot Tele
String BOT_TOKEN;
String CHAT_ID;

//Library Sensor asap
#include <TroykaMQ.h>
#define PIN_MQ135  A0
MQ135 mq135(PIN_MQ135);

// Variabel untuk lokasi
String namaLokasi;
String latitude;
String longitude;

// Interval pengiriman data ke Supabase dalam milidetik
int sendingInterval = 10000; // Pengiriman data setiap 10 detik
HTTPClient https;
WiFiClientSecure client;

void setup() {
 // Mengatur pin LED bawaan untuk menunjukkan status pengiriman data
 pinMode(LED_BUILTIN, OUTPUT);
 digitalWrite(LED_BUILTIN, HIGH); // LED menyala saat tidak mengirim data
 
 // Menghubungkan ke Wi-Fi
 Serial.begin(9600);
 
 connectToWiFi();
 // HTTPS tanpa validasi sertifikat
 client.setInsecure();

 // MQ135
  mq135.calibrate();
  Serial.print("Ro = ");
  Serial.println(mq135.getRo());
}

void loop() {
 // Cek status koneksi Wi-Fi
 if (WiFi.status() == WL_CONNECTED) {
 digitalWrite(LED_BUILTIN, LOW); // LED menyala saat mengirim data
 
 // Membaca nilai kelembapan tanah
 int gs = senosrGas();

 // Minta CHATID dan TOKEN
 mintatele();

 // JIKA GAS LEBIH 600 MENAMPILKAN LOKASI
    if (gs > 600) {
        mintalokasi();
        String apiloc = "https://www.google.com/maps?q=";
        String url = String(apiloc) + String(latitude) + "," + String(longitude);
        String message = "Peringatan! Gas terdeteksi tinggi di lokasi: " + namaLokasi + " " + url;
        sendTelegramMessage(message);
    }

 // Mengirim data ke Supabase
 sendToSupabase(gs);
 
 // Matikan LED setelah pengiriman data
 digitalWrite(LED_BUILTIN, HIGH); // LED mati
 } else {
 Serial.println("Error in Wi-Fi connection");
 }
 
 // Tunggu beberapa detik sebelum membaca dan mengirim data lagi
 delay(sendingInterval); // 3 detik sebelum membaca sensor lagi
}

//telegram
void sendTelegramMessage(String message){
  if (WiFi.status() == WL_CONNECTED)  {                          // Pastikan sudah terhubung ke WiFi
    // Format URL Telegram API
    String url = "https://api.telegram.org/bot" + String(BOT_TOKEN) + "/sendMessage?chat_id=" + String(CHAT_ID) + "&text=" + message;
    https.begin(client, url); 

    // Lakukan GET request ke Telegram API
    int httpCode = https.GET();
    // Periksa kode respon HTTP
    if (httpCode > 0)    {
        //      Serial.println("HTTP Response Code: " + String(httpCode));
        String payload = https.getString();
        Serial.println("Respon Telegram       : " + payload);
    }
    else{
      Serial.println("Error Mengirim Tele  : " + String(httpCode));
    }
    https.end(); // Menutup koneksi
  }
  else{
    Serial.println("WiFi not connected");
  }
}

// Fungsi untuk menghubungkan ke Wi-Fi
void connectToWiFi() {
 Serial.print("Connecting to Wi-Fi");
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
 delay(500);
 Serial.print(".");
 }
 Serial.println("");
 Serial.println("Wi-Fi connected.");
 Serial.println("IP address: ");
 Serial.println(WiFi.localIP());
}

// Fungsi untuk membaca nilai mq135
int senosrGas(){
  int gs = mq135.readCO2();
  Serial.print("kadar gas             : ");
  Serial.println(gs);
  return gs;
}

// Fungsi minta bot tele
void mintatele() {
    // Endpoint Supabase dengan filter
    String url = String(supabaseTele) + "?select=token,chatid&order=id.desc&limit=1";
    https.begin(client, url); // Gunakan WiFiClientSecure
    https.addHeader("apikey", apiKey);
    https.addHeader("Authorization", String("Bearer ") + apiKey);
    https.addHeader("Content-Type", "application/json");

    int httpCode = https.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = https.getString();
        Serial.println("Data Bot Telegram     : " + payload);

        // Parsing JSON
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.print("JSON Deserialization Failed: ");
            Serial.println(error.c_str());
            return;
        }
        // Ambil data
        BOT_TOKEN = doc[0]["token"].as<String>();
        CHAT_ID = doc[0]["chatid"].as<String>();
    } else {
        Serial.print("Gagal mendapatkan data, kode HTTP: ");
        Serial.println(httpCode);
    }
    https.end();
}

// Fungsi minta lokasi
void mintalokasi() {
    // Endpoint Supabase dengan filter
    String url = String(supabaseLoc) + "?select=nama,longitude,latitude&order=id.desc&limit=1";
    https.begin(client, url); // Gunakan WiFiClientSecure
    https.addHeader("apikey", apiKey);
    https.addHeader("Authorization", String("Bearer ") + apiKey);
    https.addHeader("Content-Type", "application/json");

    int httpCode = https.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = https.getString();
        Serial.println("Data Lokasi           : " + payload);

        // Parsing JSON
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.print("JSON Deserialization Failed: ");
            Serial.println(error.c_str());
            return;
        }
        // Ambil data
        namaLokasi = doc[0]["nama"].as<String>();
        latitude = doc[0]["latitude"].as<String>();
        longitude = doc[0]["longitude"].as<String>();
    } else {
        Serial.print("Gagal mendapatkan data, kode HTTP: ");
        Serial.println(httpCode);
    }
    https.end();
}

// Fungsi untuk mengirim data ke Supabase
void sendToSupabase(int sensorGas) {
 // Menyusun data JSON untuk dikirim
 String postData = "{\"nilai\":" + String(sensorGas) + "}";
 // Mengirim permintaan POST ke Supabase
 https.begin(client, supabaseSensor);
 https.addHeader("Content-Type", "application/json");
 https.addHeader("Prefer", "return=representation");
 https.addHeader("apikey", apiKey);
 https.addHeader("Authorization", String("Bearer ") + apiKey);
 // Mengirim data JSON
 int httpCode = https.POST(postData);
 String payload = https.getString();
  // Menampilkan hasil HTTP
  Serial.print("Respon Supabase       : ");
  Serial.println(httpCode);
  Serial.print("Tersimpan di Supabase : ");
  Serial.println(payload);
  Serial.println("\n________________________________________________________________________________________________________________________");
}
