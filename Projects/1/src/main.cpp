#include <WiFi.h>
#include <PubSubClient.h>

// WiFi settings
const char* ssid = "put";
const char* password = "12345678";

// MQTT Broker settings
const char* mqtt_server = "localhost";  // IP ของ MQTT Broker (Mosquitto)
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/random_data";

// WiFi and MQTT client objects
WiFiClient espClient;
PubSubClient client(espClient);

// ฟังก์ชันเชื่อมต่อ Wi-Fi
void setup_wifi() { 
  delay(10);
  
  Serial.println();
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
}

// ฟังก์ชันเชื่อมต่อ MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();  // เชื่อมต่อ Wi-Fi
  client.setServer(mqtt_server, mqtt_port);  // ตั้งค่า MQTT server
}

void loop() {
  if (!client.connected()) {
    reconnect();  // เชื่อมต่อใหม่หากหลุด
  }
  client.loop();  // เรียกใช้งาน client.loop() เพื่อเชื่อมต่อกับ MQTT

  // สุ่มค่าจาก 1 ถึง 100
  int value1 = random(1, 100);
  int value2 = random(1, 100);
  
  // สร้างข้อความในรูปแบบ JSON
  String payload = "{\"value1\": " + String(value1) + ", \"value2\": " + String(value2) + "}";

  // ส่งข้อมูลผ่าน MQTT ไปยัง Node-RED
  client.publish(mqtt_topic, payload.c_str());

  Serial.println("Published: " + payload);  // พิมพ์ข้อมูลที่ส่งออกไปใน Serial Monitor
  
  delay(5000);  // รอ 5 วินาทีแล้วสุ่มค่าครั้งใหม่
}
