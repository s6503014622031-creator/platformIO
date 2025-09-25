/****************************************************
 * ESP32-S3 → InfluxDB v2 (HTTP Line Protocol)
 * Mock sensors: Temp, Humidity, Soil moisture, Battery voltage
 * Send every 5 seconds
 ****************************************************/

#include <WiFi.h>
#include <HTTPClient.h>

/*************** WiFi *****************/
const char* WIFI_SSID = "put";
const char* WIFI_PASS = "12345678";

/*************** InfluxDB v2 **********/
const char* INFLUX_HOST = "http://172.20.10.2:8086"; // 🔹 ใส่ IP เครื่องที่รัน InfluxDB
const char* ORG         = "my-org";                   
const char* BUCKET      = "farm";
const char* TOKEN       = "EIJObN_cc40ApIcP8ym-e_qUEMo5KN8Uus_83mlir7e9IkfQAxV3Kh02p6sjR-eMasvHBtPcEYhd60lEGO_6xg==";      // 🔹 โทเคนจาก InfluxDB
const uint32_t POST_INTERVAL_MS = 5000;               // ส่งทุก 5 วิ

/*************** Helpers **************/
String deviceTag() {
  // ตั้งชื่ออุปกรณ์เอง (เปลี่ยนได้ตามใจ)
  return "esp32-1";
}

void wifiConnect() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi connecting");
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi OK: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi FAIL (will retry)");
  }
}

bool influxWrite(float temp, float hum, int soil_raw, float soil_pct, float vbat) {
  // Line protocol
  String line = "env,device=" + deviceTag();
  line += " temp=" + String(temp, 2);
  line += ",hum=" + String(hum, 2);
  line += ",soil_raw=" + String(soil_raw) + "i";   // int ต้องลงท้ายด้วย i
  line += ",soil_pct=" + String(soil_pct, 1);
  line += ",vbat=" + String(vbat, 3);

  String url = String(INFLUX_HOST) + "/api/v2/write?org=" + ORG + "&bucket=" + BUCKET + "&precision=ns";

  HTTPClient http;
  http.begin(url);
  http.addHeader("Authorization", String("Token ") + TOKEN);
  http.addHeader("Content-Type", "text/plain; charset=utf-8");
  int code = http.POST(line);
  http.end();

  Serial.printf("[InfluxDB] %d  %s\n", code, line.c_str());
  return (code == 204);
}

uint32_t lastPost = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== ESP32 (Mock) → InfluxDB v2 ===");
  wifiConnect();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnect();
  }

  if (millis() - lastPost >= POST_INTERVAL_MS) {
    lastPost = millis();

    // ==== Mock Data ====
    float t = 25.0 + (esp_random() % 100) / 10.0;   // Temp 25.0–34.9 °C
    float h = 50.0 + (esp_random() % 200) / 10.0;   // Hum 50–69.9 %
    int   soil_raw = 1500 + (esp_random() % 800);   // Soil raw 1500–2299
    float soil_pct = 100.0 * (3000 - soil_raw) / (3000 - 1200);
    if (soil_pct < 0) soil_pct = 0;
    if (soil_pct > 100) soil_pct = 100;
    float vbat = 3.7 + (esp_random() % 40) / 100.0; // Vbat 3.7–4.09 V

    // ==== Send to InfluxDB ====
    influxWrite(t, h, soil_raw, soil_pct, vbat);
  }

  delay(50);
}
