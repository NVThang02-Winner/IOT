#include <dummy.h>

int relayPin = D1; // Chân D1 gắn Relay 1
const int analogInPin = A0; // Chân kết nối ACS712 với Arduino

int sensorValue = 0; // Biến lưu giữ giá trị đọc từ ACS712

#define BLYNK_TEMPLATE_ID "TMPL6yAtUbtBn"
#define BLYNK_TEMPLATE_NAME "Smart Socket"
#define BLYNK_AUTH_TOKEN "sRkn4jnLonaJR1NW1vjIKrfqH2WHcheP"

#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

char auth[] = BLYNK_AUTH_TOKEN;

char ssid[] = "P303";
char pass[] = "vanchau1955";

BlynkTimer timer;

float current;
float totalEnergy = 0.0;
unsigned long previousMillis = 0;
unsigned long interval = 1000; // Đo tổng lượng điện mỗi giây

// phần đặt thời gian:
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 7 * 3600; // UTC +7:00
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", utcOffsetInSeconds);

int startTime;
int endTime;

// Bật tắt từ xa
BLYNK_WRITE(V0)
{
  int relayStatus = param.asInt();
  if (relayStatus == 1)
  {
    digitalWrite(relayPin, LOW); // Bật
  }
  else
    digitalWrite(relayPin, HIGH); // Tắt
}

////////////////////////
// Hàm đọc giá trị dòng điện từ ACS712
float readCurrent()
{
  // Đọc giá trị từ chân analog
  sensorValue = analogRead(analogInPin);

  // Tính toán giá trị dòng điện (thay đổi phụ thuộc vào calibrations của bạn)
  return map(sensorValue, 0, 1023, 0, 20000) / 1000.0; // 20A version
}

// Đo tổng lượng điện đã tiêu thụ
void measureTotalEnergy()
{
  current = readCurrent();
  totalEnergy += (current * (float)(millis() - previousMillis)) / (3600000.0); // Chuyển đổi thành kWh
  previousMillis = millis();
}

void sendValuesToBlynk()
{
  Blynk.virtualWrite(V1, current);
  Blynk.virtualWrite(V2, totalEnergy);
}

void displayValuesOnSerial()
{
  Serial.print("Current Energy: ");
  Serial.print(current, 3); // Hiển thị 3 chữ số sau dấu thập phân
  Serial.print(" kWh | Total Energy: ");
  Serial.print(totalEnergy, 3);
  Serial.println(" kWh");
}

//////////////////////////////
// đặt lịch bật tắt
BLYNK_WRITE(V3){
 startTime = param[0].asInt();
 endTime = param[1].asInt();
 if(startTime == 0 && endTime == 0)
 {
  startTime = 999999; // thời gian không được đặt
  endTime = 999999;
 }
}
void scheduledActions()
{
  timeClient.update(); // cập nhật thời gian
  int HH = timeClient.getHours();
  int MM = timeClient.getMinutes();
  int server_time = HH * 3600 + MM * 60;

  if (server_time == startTime)
  {
    digitalWrite(relayPin, LOW); // Bật 
  }
  if(server_time == endTime)
  {
    digitalWrite(relayPin, HIGH); // Tắt
  }
}

void setup()
{
  // Debug console
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);

  pinMode(relayPin, OUTPUT);

  digitalWrite(relayPin, HIGH); // Trạng thái tắt ban đầu
  
  timeClient.begin(); // Bắt đầu cập nhật thời gian từ máy chủ NTP
  timer.setInterval(1000L, scheduledActions); // Đặt lịch thực hiện hàm mỗi giây
}

void loop()
{
  // Đọc giá trị dòng điện từ hàm
  float currentEnergy = readCurrent();

  // Gửi giá trị lên Blynk
  Blynk.virtualWrite(V1,  currentEnergy);

  ///////////////////////////////////////
  // Đo tổng lượng điện
  measureTotalEnergy();

  // Gửi giá trị lên Blynk
  sendValuesToBlynk();

  // Hiển thị giá trị lượng điện trên Serial Monitor
  displayValuesOnSerial();

  Blynk.run();
  timer.run();
}
