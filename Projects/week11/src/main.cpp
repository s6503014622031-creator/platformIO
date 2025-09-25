#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

String botToken = "8363532024:AAG_Nsn1wYTI3OijGrfXYNk6UL977WUUy3M";  // BotFather à¹ƒà¸«à¹‰à¸¡à¸²
String chatID = "8300809611";


// WiFi AP SSID
#define WIFI_SSID "put"
// WiFi password
#define WIFI_PASSWORD "12345678"

#define INFLUXDB_URL "http://172.18.158.78:8086"
#define INFLUXDB_TOKEN "7Q89YGHjkTgRK3MOxqwH6nEHiEzVfTUte0UQu94ZHE9RHW2jk1Q72uoWuLZ8Tpbs3f9E-vudzh4bWVKP4EPAlw=="
#define INFLUXDB_ORG "CasaOrg"
#define INFLUXDB_BUCKET "data1"

// Time zone info
#define TZ_INFO "UTC7"

// Declare InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Declare Data point
Point sensor("sensors_data");

void setup() {
  Serial.begin(115200);

  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  // Accurate time is necessary for certificate validation and writing in batches
  // We use the NTP servers in your area as provided by: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");


  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  sensor.addTag("ID", "ESP_32");
}

float temp = 0.00f;
float humidity = 0.00f;

void influxHandler(uint32_t interval) {
  static uint32_t previousTime = 0;
  if (millis() - previousTime >= interval) {
    sensor.clearFields();
    sensor.addField("Temp", temp);
    sensor.addField("Humdity", humidity);

    if (!client.writePoint(sensor)) {
      Serial.print("InfluxDB write failed: ");
      Serial.println(client.getLastErrorMessage());
    }
    else {
      Serial.print("Writing: ");
      Serial.println(client.pointToLineProtocol(sensor));
      Serial.print("InfluxDB write successfully ! \n");
    }

    previousTime = millis();

  }
}

void telegramHandler(uint32_t interval) {
  static uint32_t previousTime = 0;
  if (millis() - previousTime >= interval) {
    String buffer = "ID:" + String("ESP_32") + ",Temp:" + String(temp) + ",Humidity:" + String(humidity);
    sendMessage(buffer);
    previousTime = millis();
  }
}

void sendMessage(String text) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + urlencode(text);

    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("à¸ªà¹ˆà¸‡à¸‚à¹‰à¸­à¸„à¸§à¸²à¸¡à¸ªà¸³à¹€à¸£à¹‡à¸ˆ: " + text);
    } else {
      Serial.println("à¸ªà¹ˆà¸‡à¹„à¸¡à¹ˆà¸ªà¸³à¹€à¸£à¹‡à¸ˆ: " + http.errorToString(httpCode));
    }
    http.end();
  }
}

String urlencode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
  }
  return encodedString;
}

void loop() {
  temp = random(0, 80);
  humidity = random(0, 100);

  influxHandler(1000);
  telegramHandler(1000);

}
