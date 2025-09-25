#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ArtronShop_LineMessaging.h>
#include <WiFiManager.h> // ต้องใช้ Library tzapu/WiFiManager

#define LINE_TOKEN "9UlTlVE/iA8YnN8NKnUBusQhFbhWn/Mva09Ba9TzHLYj4YJZZdK2mqUlhzgrnF2gLOvLHFCePtn2fAB0IMykfFHt2tjHzPgqejBZ3V0t1m3/p+8iENI+Zm4ndq0T2GTv6gcVfqLECke9Hf95Xl6DAAdB04t89/1O/w1cDnyilFU="
#define USER_ID "U016dc0863665c83ea862ee070885f26c"
#define CURRENT_VERSION "1.0.0"
#define OTA_JSON_URL "http://yourserver.com/firmware.json"

SemaphoreHandle_t counterMutex;
volatile int a2_counter = 0;

void sendLineMessage() {
  if (LINE.send(USER_ID, "hello")) {
    Serial.println("✅ ส่ง LINE สำเร็จ");
  } else {
    Serial.printf("❌ ส่ง LINE ล้มเหลว (code: %d)\n", LINE.status_code);
  }
}

// === OTA Check ===
void checkOTAUpdate() {
  Serial.println("🔍 Checking for OTA update...");

  HTTPClient http;
  http.begin(OTA_JSON_URL);

  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.printf("❌ Failed to fetch OTA info. HTTP code: %d\n", httpCode);
    http.end();
    return;
  }

  String json = http.getString();
  http.end();

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.println("❌ JSON parse error");
    return;
  }

  const char* latestVersion = doc["version"];
  const char* firmwareURL = doc["firmware"];

  Serial.printf("🆚 Current: %s | Available: %s\n", CURRENT_VERSION, latestVersion);

  if (String(latestVersion) != CURRENT_VERSION) {
    Serial.println("🚀 New version detected, starting OTA...");

    WiFiClient client;
    httpUpdate.begin(client, firmwareURL);
    t_httpUpdate_return ret = httpUpdate.update(client, firmwareURL);

    switch (ret) {
      case HTTP_UPDATE_OK:
        Serial.println("✅ OTA Done");
        break;
      case HTTP_UPDATE_FAILED:
        Serial.printf("❌ OTA failed: %s\n", httpUpdate.getLastErrorString().c_str());
        break;
    }
  } else {
    Serial.println("✅ Firmware up-to-date.");
  }
}

// === Task A1 ===
void taskA1(void* parameter) {
  while (true) {
    Serial.println("📤 Send: A1");
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// === Task A2 ===
void taskA2(void* parameter) {
  while (true) {
    Serial.println("📤 Send: A2");

    xSemaphoreTake(counterMutex, portMAX_DELAY);
    a2_counter++;
    int countCopy = a2_counter;
    xSemaphoreGive(counterMutex);

    if (countCopy == 5) {
      sendLineMessage();

      xSemaphoreTake(counterMutex, portMAX_DELAY);
      a2_counter = 0;
      xSemaphoreGive(counterMutex);
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 🌐 WiFi Manager: เปิด Portal ถ้าเชื่อมต่อไม่ได้
  WiFiManager wm;
  bool res = wm.autoConnect("ESP32_Setup");

  if (!res) {
    Serial.println("❌ Failed to connect. Restarting...");
    ESP.restart();
  }

  Serial.println("✅ WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  LINE.begin(LINE_TOKEN);

  counterMutex = xSemaphoreCreateMutex();

  xTaskCreate(taskA1, "Task A1", 2048, NULL, 1, NULL);
  xTaskCreate(taskA2, "Task A2", 8192, NULL, 1, NULL);

  checkOTAUpdate(); // 🔁 ตรวจสอบ OTA หลังเชื่อม WiFi
}

void loop() {
  // ไม่ใช้ loop เพราะใช้ FreeRTOS แต่สามารถเพิ่ม OTA เช็คซ้ำได้ถ้าต้องการ
}
