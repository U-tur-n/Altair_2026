#include <Arduino.h>
#include <SD.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <vector>
#include "BME/BME.h"
#include "BNO/BNO.h"
#include "GPS/GPS.h"

#define tact 1 // D0
#define SD_CS 21
#define BME_CS 3

unsigned long lastTime = 0;

float altitude = BME::altitude;
float latitude = GPS::latitude;
float longitude = GPS::longitude;
float ax;
float ay;
float az;
float q;

std::vector<float> measureData;
File fp;
char fileName[] = "/data0.csv";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ネットワーク設定（お好みの名前に変更してください）
const char* ssid = "Altair_Avionics";     // 飛ばすWi-Fiの名前

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  // ブラウザからのボタン入力（SD記録開始など）を処理する場合はここに記述
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

// put function declarations here:
void save(bool);

void setup() {
  // put your setup code here, to run once:
  //ピン設定
  pinMode(SD_CS, OUTPUT);
  pinMode(BME_CS, OUTPUT);
  pinMode(tact, INPUT_PULLUP);

  
SPI.begin();
  //SPI衝突回避用
  digitalWrite(SD_CS, HIGH);
  digitalWrite(BME_CS, HIGH);

  

  Serial.begin(115200);
  while(!Serial);
  delay(100);

  initWebSocket();

  // --- ここから APモード（アクセスポイント）の設定 ---
    Serial.println("Starting Access Point...");
    WiFi.softAP(ssid);
    
    // ESP32のIPアドレスを取得（デフォルトは 192.168.4.1 になります）
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    // --------------------------------------------------

    // --- 2. LittleFSの初期化（HTML等の読み込みに必須） ---
  if(!LittleFS.begin(true)){
    Serial.println("LittleFS Mount Failed");
    while(1);
  }
    server.addHandler(&ws);
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    server.begin();

  //SD初期化 
  Serial.print("Initializing SD card...");
    if (SD.begin(SD_CS) == false) {
    Serial.println("SD card faile ro not present");
    while (1)
      ;
    }
    
  Serial.println("OK");

  
  // Serial.println("receive any key...");
  // while (Serial.available() == 0)
  //   ;
  //   ;
  

  BME::begin();
  BNO::begin();
  GPS::begin();

  //ボーレートを115200
  Serial.end();
  Serial.begin(115200);

    //タクトスイッチが押されたら計測開始
  Serial.print("Opening the file...");
  fp = SD.open(fileName, FILE_WRITE);
  if (fp == false) {
    Serial.println("cannot open the file");
    while (1)
      ;
  }
  Serial.println("OK");
  fp.println("time, altitude, latitude, longitude, ax, ay, az, q");
  Serial.println("press the tact switch...");
  while(digitalRead(tact) == HIGH)
    ;
}

void loop() {
  // put your main code here, to run repeatedly:
  ws.cleanupClients();
  if(millis() - lastTime >= 200){
    measureData.push_back(millis());
    Serial.print(millis());
    Serial.print(", ");

    BME::execute();
    GPS::execute();
    BNO::execute();
    altitude = BME::altitude;
    latitude = GPS::latitude;
    longitude = GPS::longitude;
    ax = BNO::ax;
    ay = BNO::ay;
    az = BNO::az;
    // q = BNO::q;

    measureData.push_back(altitude);
    measureData.push_back(latitude);
    measureData.push_back(longitude);
    measureData.push_back(ax);
    measureData.push_back(ay); 
    measureData.push_back(az);
    // measureData.push_back(q);

    Serial.print(altitude);
    Serial.print(", ");
    Serial.print(latitude);
    Serial.print(", ");
    Serial.print(longitude);
    Serial.print(", ");
    Serial.print(ax);
    Serial.print(", ");
    Serial.print(ay);
    Serial.print(", ");
    Serial.println(az);
    // Serial.print(", ");
    // Serial.println(q);

    // すでに取得済みの変数を元に、JSONフォーマットの文字列を生成
    // (ArduinoJsonライブラリを使用しても良いですが、軽量化のため文字列結合で生成しています)
    String jsonString = "{";
    jsonString += "\"ax\":" + String(ax, 2) + ",";
    jsonString += "\"ay\":" + String(ay, 2) + ",";
    jsonString += "\"az\":" + String(az, 2) + ",";
    jsonString += "\"latitude\":" + String(latitude, 6) + ",";
    jsonString += "\"longitude\":" + String(longitude, 6) + ",";
    jsonString += "\"altitude\":" + String(altitude, 1);
    // jsonString += "\"msg\":\"Data updated at " + String(currentMillis / 1000) + "s\"";
    jsonString += "}";

    // 接続されているすべてのブラウザへデータを送信
    ws.textAll(jsonString);

    if(digitalRead(tact) == LOW){
      save(true);
    }
    if(measureData.size() >= 2100){
      save(false);
    }
  lastTime = millis();
  }
}

// put function definitions here:
void save(bool end) {
  int i = 0;
  digitalWrite(SD_CS, LOW);
    for (double data : measureData)
    {
      fp.print(data);
      i++;
      if (i % 7 == 0) { // 7個のデータごとに改行
        fp.println();
      } else {
        fp.print(",");
      }
      // fp.print(",");
      // fp.println(data); // q
      // Serial.println("receive any key...");
      // if (Serial.available() != 0){
      //   while(1)
      //     ;
      // }
    }
  if (end == false) { // データ保存のみ
    fp.flush();
    Serial.println("saved data");
    measureData.clear();
    digitalWrite(SD_CS, HIGH);
  } else {
    fp.close(); // ファイルを閉じる(スイッチが押された場合)
    Serial.println("saved data and closed file");
    measureData.clear();
    while (1)
      ;
  }
}
