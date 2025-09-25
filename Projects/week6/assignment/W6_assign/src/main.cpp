#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <WiFiManager.h> // ✅ ใช้ WiFiManager
#include <ArtronShop_LineMessaging.h>

#define LINE_TOKEN "9UlTlVE/iA8YnN8NKnUBusQhFbhWn/Mva09Ba9TzHLYj4YJZZdK2mqUlhzgrnF2gLOvLHFCePtn2fAB0IMykfFHt2tjHzPgqejBZ3V0t1m3/p+8iENI+Zm4ndq0T2GTv6gcVfqLECke9Hf95Xl6DAAdB04t89/1O/w1cDnyilFU="

LINE_Messaging_Massage_Option_t option;

#define CURRENT_VERSION "1.0.1"
const char *versionInfoUrl = "http://172.20.10.4/firmware.json";

void connectWiFi()
{
  const char* ssid = "put";         // ← ใส่ชื่อ Wi-Fi ที่นี่
  const char* password = "12345678"; // ← ใส่รหัสผ่าน Wi-Fi

  WiFi.begin(ssid, password);
  Serial.println("🔌 Connecting to WiFi...");

  unsigned long startAttemptTime = millis();

  // รอไม่เกิน 15 วินาที
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n❌ Failed to connect. Restarting...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("\n✅ WiFi connected: " + WiFi.localIP().toString());
}


bool checkForUpdate(String &firmwareURL)
{
  HTTPClient http;
  http.begin(versionInfoUrl);
  int httpCode = http.GET();

  if (httpCode == 200)
  {
    String payload = http.getString();
    ArduinoJson::JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);

    if (err)
    {
      Serial.println("❌ JSON Parse failed");
      http.end();
      return false;
    }

    String latestVersion = doc["version"] | "";
    firmwareURL = doc["bin_url"] | "";

    http.end();

    if (latestVersion != CURRENT_VERSION && firmwareURL != "")
    {
      Serial.println("📦 New version available. Ready to OTA.");
      return true;
    }
    else
    {
      Serial.println("✅ Already up-to-date.");
      return false;
    }
  }
  else
  {
    Serial.printf("❌ Failed to fetch version.json. HTTP code: %d\n", httpCode);
    http.end();
    return false;
  }
}


void doOTA(const String &firmwareURL)
{
  HTTPClient http;
  http.begin(firmwareURL);
  int httpCode = http.GET();

  if (httpCode == 200)
  {
    int contentLength = http.getSize();
    WiFiClient *stream = http.getStreamPtr();

    if (Update.begin(contentLength))
    {
      size_t written = Update.writeStream(*stream);
      if (written == contentLength)
      {
        Serial.println("✅ Firmware written successfully.");
        if (Update.end())
        {
          Serial.println("🔁 Restarting...");
          ESP.restart();
        }
      }
      else
      {
        Serial.println("❌ Write failed: size mismatch.");
      }
    }
    else
    {
      Serial.println("❌ Not enough space for OTA.");
    }
  }
  else
  {
    Serial.printf("Failed to fetch firmware.bin. HTTP code: %d\n", httpCode);
  }

  http.end();
}

void Task1(void *pvParameters)
{
  uint32_t previousTimeA1 = 0;
  uint32_t previousTimeA2 = 0;
  uint8_t counter = 0;
  while (true)
  {
    if (millis() - previousTimeA1 >= 2000)
    {
      Serial.println("A1");
      previousTimeA1 = millis();
    }

    if (millis() - previousTimeA2 >= 5000)
    {
      Serial.println("A2");
      counter++;
      previousTimeA2 = millis();
    }

    if (counter >= 4)
    {
      if (LINE.send("U016dc0863665c83ea862ee070885f26c", "A2"))
      {
        Serial.println("Send notify successful");
      }
      counter = 0;
    }

    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void Task2(void *pvParameters)
{
  while (true)
  {
    static uint32_t previousTime = 0;
    if (millis() - previousTime >= 100)
    {
      String firmwareURL;
      if (checkForUpdate(firmwareURL))
      {
        doOTA(firmwareURL);
      }
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void init_rtos()
{
  xTaskCreatePinnedToCore(Task1, "Task1", 10000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(Task2, "Task2", 4095, NULL, 1, NULL, 1);
  Serial.println("RTOS:Initialized");
}

void setup()
{
  Serial.begin(115200);
  connectWiFi();
  LINE.begin(LINE_TOKEN);
  init_rtos();
  delay(1000);
}

void loop() {}
