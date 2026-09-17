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
float x_dist = GPS::x_dist;
float y_dist = GPS::y_dist;
float dist = GPS::dist;
float ax;
float ay;
float az;
float q;

bool isRemoteSdActive = false; //記録開始用フラグ
bool isRemotePwrActive = false;//電装動作用フラグ

bool last_camera_in_use = false;

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
    else if (message == "CMD_PWR") {
      isRemotePwrActive = true;
      Serial.print("電装動作フラグが変更されました: ");
      Serial.println(isRemotePwrActive ? "ON" : "OFF");
    }

    
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
  // while(!Serial);
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
server.serveStatic("/", LittleFS, "/")
      .setDefaultFile("index.html")
      .setCacheControl("no-store, no-cache, must-revalidate, max-age=0"); //キャッシュを無効化するためのヘッダを追加
    server.begin();

  // --- 3. カメラの初期化とカメラサーバーの開始 ---
  initCamera();
  startCameraServer();

  //websocket接続まで待機
  Serial.println("Waiting for Web UI to connect...");
  while (ws.count() == 0) {
    delay(100); // 接続されるまで100msごとに待機（WDTリセット防止のためdelayは必須）
  }
  Serial.println("Web UI Connected!");
  delay(500); // 接続直後の安定化のために少し待つ

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
  
  delay(100);
  if(BME::BMEbegin()){
    sendStatusMessage("BME280 initialized successfully.");
  } else {
    sendStatusMessage("BME280 initialization failed.");
    while (1)
      ;
  }
  delay(100);
  if(BNO::BNObegin()){
    sendStatusMessage("BNO055 initialized successfully.");
  } else {
    sendStatusMessage("BNO055 initialization failed.");
    while (1)
      ;
  }
  delay(100);
  if(GPS::GPSbegin()){
    sendStatusMessage("GPS initialized successfully.");
  } else {
    sendStatusMessage("GPS initialization failed.");
    while (1)
      ;
  }
  delay(100);
  //ボーレートを115200

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
    if(camera_in_use != last_camera_in_use){
      sendStatusMessage(camera_in_use ? "Camera is active" : "Camera is inactive");
      last_camera_in_use = camera_in_use;
    }
    String jsonString = "{";
    jsonString += "\"sd_active\":" + String(isRemoteSdActive ? "true" : "false") + ",";
    jsonString += "\"cam_active\":" + String(camera_in_use ? "true" : "false");
    jsonString += "}";
    ws.textAll(jsonString);
    delay(50);
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
    x_dist = GPS::x_dist;
    y_dist = GPS::y_dist;
    dist = GPS::dist;
    ax = BNO::ax;
    ay = BNO::ay;
    az = BNO::az;
    q = BNO::q;

    float ln;
    measureData.push_back(altitude);
    measureData.push_back(latitude);
    measureData.push_back(longitude);
    measureData.push_back(x_dist);
    measureData.push_back(y_dist);
    measureData.push_back(dist);
    measureData.push_back(ax);
    measureData.push_back(ay); 
    measureData.push_back(az);
    measureData.push_back(q);
    measureData.push_back(9999); //この値を受け取ったらcsvで改行

    Serial.print(altitude);
    Serial.print(", ");
    Serial.print(latitude);
    Serial.print(", ");
    Serial.print(longitude);
    Serial.print(", ");
    Serial.print(x_dist);
    Serial.print(", ");
    Serial.print(y_dist);
    Serial.print(", ");
    Serial.print(dist);
    Serial.print(",");
    Serial.print(ax);
    Serial.print(", ");
    Serial.print(ay);
    Serial.print(", ");
    Serial.println(az);
    Serial.print(", ");
    Serial.println(q);

    // すでに取得済みの変数を元に、JSONフォーマットの文字列を生成
    // (ArduinoJsonライブラリを使用しても良いですが、軽量化のため文字列結合で生成しています)
    String jsonString = "{";
    jsonString += "\"ax\":" + String(ax, 2) + ",";
    jsonString += "\"ay\":" + String(ay, 2) + ",";
    jsonString += "\"az\":" + String(az, 2) + ",";
    jsonString += "\"latitude\":" + String(latitude, 6) + ",";
    jsonString += "\"longitude\":" + String(longitude, 6) + ",";
    jsonString += "\"x_dist\":" + String(x_dist, 1) + ",";
    jsonString += "\"y_dist\":" + String(y_dist, 1) + ",";
    jsonString += "\"dist\":" + String(dist, 1) + ",";
    jsonString += "\"altitude\":" + String(altitude, 1) + ",";
    jsonString += "\"sd_active\":" + String(isRemoteSdActive ? "true" : "false") + ",";
    jsonString += "\"cam_active\":" + String(camera_in_use ? "true" : "false") + ",";
    jsonString += "\"pwr_active\":" + String(isRemotePwrActive ? "true" : "false");
    // jsonString += "\"msg\":\"Data updated at " + String(currentMillis / 1000) + "s\"";
    jsonString += "}";

    // 接続されているすべてのブラウザへデータを送信
    ws.textAll(jsonString);

    if(camera_in_use != last_camera_in_use){
      sendStatusMessage(camera_in_use ? "Camera is active" : "Camera is inactive");
      last_camera_in_use = camera_in_use;
    }

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

  if (isRemoteSdActive) {
  }
  lastTime = millis();
  }

  // リモートSD記録がONの場合、指定間隔でタイムラプスを保存

}

// put function definitions here:
void save(bool end) {
  int i = 0;
  digitalWrite(SD_CS, LOW);
    for (double data : measureData)
    {
      fp.print(data);
      i++;
      if (data == 9999) { // この値を受け取ったら改行
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
      if(camera_in_use != last_camera_in_use){
      sendStatusMessage(camera_in_use ? "Camera is active" : "Camera is inactive");
      last_camera_in_use = camera_in_use;
    }
    String jsonString = "{";
    jsonString += "\"sd_active\":" + String(isRemoteSdActive ? "true" : "false") + ",";
    jsonString += "\"cam_active\":" + String(camera_in_use ? "true" : "false");
    jsonString += "}";

    // 接続されているすべてのブラウザへデータを送信
    ws.textAll(jsonString);
      delay(50);
    }
    delay(10);
  while(digitalRead(tact) == LOW)
  ;
  isRemoteSdActive = true; // リモートSD記録フラグを強制的にtrueに設定
  sendStatusMessage("start");
}
}
