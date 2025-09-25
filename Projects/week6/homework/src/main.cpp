#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP_Mail_Client.h>

#define WIFI_SSID "put"
#define WIFI_PASSWORD "12345678"

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

#define AUTHOR_EMAIL "kansudaa.p@gmail.com"
#define AUTHOR_PASSWORD "blgi rdor ihml ureb"  // App Password

#define RECIPIENT_EMAIL "s6503014622031@email.kmutnb.ac.th"

// NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000); // GMT+7

struct DataRecord {
  String datetime;
  int value1;
  int value2;
};

DataRecord records[10];
int dataIndex = 0;
unsigned long lastUpdate = 0;

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected");
}

void sendEmail(DataRecord data[10]) {
  SMTPSession smtp;
  ESP_Mail_Session session;

  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";

  SMTP_Message message;
  message.sender.name = "ESP32 Logger";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "ESP32 Logged Data (10 records)";
  message.addRecipient("Recipient", RECIPIENT_EMAIL);

  // สร้างข้อความอีเมล
  String content = "Timestamp, Value1, Value2\n";
  for (int i = 0; i < 10; i++) {
    content += data[i].datetime;
    content += ", ";
    content += String(data[i].value1);
    content += ", ";
    content += String(data[i].value2);
    content += "\n";
  }

  message.text.content = content.c_str();
  message.text.charSet = "utf-8";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  if (!smtp.connect(&session)) {
    String msg = "Failed to connect to mail server";
    Serial.println(msg);
    return;
  }

  if (!MailClient.sendMail(&smtp, &message)) {
    String failMsg = "Failed to send email: ";
    failMsg.concat(smtp.errorReason());
    Serial.println(failMsg);
  } else {
    Serial.println("Email sent successfully.");
  }

  smtp.closeSession();
}

void setup() {
  Serial.begin(115200);
  connectWiFi();
  timeClient.begin();
}

void loop() {
  timeClient.update();

  if (millis() - lastUpdate >= 2000) {
    lastUpdate = millis();

    int val1 = random(0, 101);
    int val2 = random(0, 101);
    String timestamp = timeClient.getFormattedTime();

    Serial.print("[");
    Serial.print(timestamp);
    Serial.print("] Value1: ");
    Serial.print(val1);
    Serial.print(", Value2: ");
    Serial.println(val2);

    records[dataIndex] = { timestamp, val1, val2 };
    dataIndex++;

    if (dataIndex >= 10) {
      sendEmail(records);
      dataIndex = 0;
    }
  }
}
