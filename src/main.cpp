#include <Arduino.h>
#include <SD.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <vector>
#include "BME/BME.h"
#include "BNO/BNO.h"
#include "GPS/GPS.h"
#include "camera_server/camera_server.h"

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

bool isRemoteSdActive = false; //記録開始用フラグ
// bool isRemotePwrActive = false;//電装動作用フラグ

std::vector<float> measureData;
File fp;
char fileName[] = "/data0.csv";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Web UIのステータスボックスにメッセージを送る関数
void sendStatusMessage(String msg) {
  String jsonString = "{\"msg\":\"" + msg + "\"}";
  ws.textAll(jsonString);
}

// ネットワーク設定（お好みの名前に変更してください）
const char* ssid = "Altair_Avionics";     // 飛ばすWi-Fiの名前

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  // ブラウザからのボタン入力（SD記録開始など）を処理する場合はここに記述
AwsFrameInfo *info = (AwsFrameInfo*)arg;
  
  // テキストメッセージがすべて揃っているか確認
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    
    // 受信したデータ(uint8_tの配列)を、扱いやすいString型に変換する
    String message = "";
    for (size_t i = 0; i < len; i++) {
      message += (char)data[i];
    }
    
    Serial.print("Web UIからコマンドを受信: ");
    Serial.println(message);

    // --- メッセージの内容に応じて変数を変更 ---
    if (message == "CMD_SD" ) { //|| isRemoteSdActive==true
      isRemoteSdActive = !isRemoteSdActive; // true/falseを反転
      Serial.print("SD記録フラグが変更されました: ");
      Serial.println(isRemoteSdActive ? "ON" : "OFF");
      
      // ここにSD記録開始の処理や save() 関数を呼び出す処理を書くことができます
    } 
    // else if (message == "CMD_PWR") {
    //   isRemotePwrActive = !isRemotePwrActive;
    //   Serial.print("電装動作フラグが変更されました: ");
    //   Serial.println(isRemotePwrActive ? "ON" : "OFF");
    // }

    
  }
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

  // --- 3. カメラの初期化とカメラサーバーの開始 ---
  initCamera();
  startCameraServer();

  //SD初期化 
  Serial.print("Initializing SD card...");
  sendStatusMessage("Initializing SD card...");
    if (SD.begin(SD_CS) == false) {
    Serial.println("SD card or file not present");
    sendStatusMessage("SD card or file not present");
    while (1)
      ;
    }
    
  Serial.println("OK");
    sendStatusMessage("OK");

  
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
    sendStatusMessage("Opening the file...");
  fp = SD.open(fileName, FILE_WRITE);
  if (fp == false) {
    Serial.println("cannot open the file");
    sendStatusMessage("cannot open the file");
    while (1)
      ;
  }
  Serial.println("OK");
  sendStatusMessage("OK");

  fp.println("time, altitude, latitude, longitude, ax, ay, az, q");
  Serial.println("press the tact switch...");
  sendStatusMessage("press the button...");
  while(digitalRead(tact) == HIGH && isRemoteSdActive == false) {
    delay(10);
  }
  delay(10);
  while(digitalRead(tact) == LOW)
  ;
  isRemoteSdActive = true; // リモートSD記録フラグを強制的にtrueに設定
  sendStatusMessage("start");
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

    if(digitalRead(tact) == LOW || isRemoteSdActive == false){
      Serial.println("stop");
      sendStatusMessage("stop");
      
  while(digitalRead(tact) == LOW)
  ;
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
    sendStatusMessage("saved data");
    measureData.clear();
    digitalWrite(SD_CS, HIGH);
  } else {
    fp.close(); // ファイルを閉じる(スイッチが押された場合)
    isRemoteSdActive = false; // 記録フラグをfalseに戻す
    Serial.println("saved data and closed file");
    sendStatusMessage("saved data and closed file");
    measureData.clear();
  Serial.println("press the tact switch to restart...");
  sendStatusMessage("press the button to restart...");
    while (digitalRead(tact) == HIGH && isRemoteSdActive == false) {
      delay(10);
    }
    delay(10);
  while(digitalRead(tact) == LOW)
  ;
  isRemoteSdActive = true; // リモートSD記録フラグを強制的にtrueに設定
  sendStatusMessage("start");
}
}
