#include <Arduino.h>
#include <SD.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <vector>
#include "BME/BME.h"
#include "BNO/BNO.h"
#include "GPS/GPS.h"
#include "esp_camera.h"
// #include "esp_http_server.h"

// ＝＝＝ XIAO ESP32S3 Sense 専用カメラピン設定 ＝＝＝
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13
// ＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝

#define tact 1 // D0
#define SD_CS 21 // XIAO ESP32S3 Sense の拡張ボードMicroSDスロット用CSピン
#define BME_CS 3

unsigned long lastTime = 0;

float altitude = BME::altitude;
float latitude = GPS::latitude;
float longitude = GPS::longitude;
float ax, ay, az, q;

bool isRemoteSdActive = false;

std::vector<float> measureData;
File fp;
char fileName[] = "/data0.csv";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
httpd_handle_t stream_httpd = NULL;

const char* ssid = "Altair_Avionics";

// ==========================================
// カメラストリーム(MJPEG)配信用関数
// ==========================================
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=123456789000000000000987654321");
  if (res != ESP_OK) return res;

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
      break;
    }
    if (fb->format != PIXFORMAT_JPEG) {
      bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
      esp_camera_fb_return(fb);
      fb = NULL;
      if (!jpeg_converted) {
        Serial.println("JPEG compression failed");
        res = ESP_FAIL;
        break;
      }
    } else {
      _jpg_buf_len = fb->len;
      _jpg_buf = fb->buf;
    }

    size_t hlen = snprintf((char *)part_buf, 64, "\r\n--123456789000000000000987654321\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", _jpg_buf_len);
    res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, "\r\n", 2);

    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if (res != ESP_OK) break;
  }
  return res;
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 81;

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    Serial.println("Camera Stream Server started on port 81");
  }
}

// ==========================================
// WebSocket コマンド受信処理
// ==========================================
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    String message = "";
    for (size_t i = 0; i < len; i++) {
      message += (char)data[i];
    }
    
    Serial.print("Web UIからコマンドを受信: ");
    Serial.println(message);

    if (message == "CMD_SD") {
      isRemoteSdActive = !isRemoteSdActive;
      Serial.print("SD記録フラグが変更されました: ");
      Serial.println(isRemoteSdActive ? "ON" : "OFF");
    } 
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected\n", client->id());
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

void save(bool);

void setup() {
  pinMode(SD_CS, OUTPUT);
  pinMode(BME_CS, OUTPUT);
  pinMode(tact, INPUT_PULLUP);
  
  SPI.begin();
  digitalWrite(SD_CS, HIGH);
  digitalWrite(BME_CS, HIGH);

  Serial.begin(115200);
  while(!Serial);
  delay(100);

  // --- カメラの初期化 (XIAO ESP32S3 Sense 最適化) ---
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  
  // PSRAMの有無で解像度とバッファを切り替え
  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2; // ダブルバッファでストリーミングを滑らかに
    Serial.println("PSRAM found. Set to VGA with double buffer.");
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    Serial.println("PSRAM NOT found. Set to QVGA with single buffer.");
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
  } else {
    Serial.println("Camera init OK");
    // カメラの上下が反転している場合は、以下のコメントアウトを外して反転させます
    // sensor_t * s = esp_camera_sensor_get();
    // s->set_vflip(s, 1);
  }

  initWebSocket();

  Serial.println("Starting Access Point...");
  WiFi.softAP(ssid);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  if(!LittleFS.begin(true)){
    Serial.println("LittleFS Mount Failed");
    while(1);
  }
  
  server.addHandler(&ws);
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.begin(); // ポート80
  
  // カメラの初期化に成功した場合のみ映像サーバーを立てる
  if(err == ESP_OK){
    startCameraServer(); // ポート81
  }

  Serial.print("Initializing SD card...");
  // 万が一SDの接触不良があっても、UIと映像だけは動くようにwhile(1)を外しています
  if (SD.begin(SD_CS) == false) {
    Serial.println("SD card failed or not present");
  } else {
    Serial.println("OK");
  }

  BME::begin();
  BNO::begin();
  GPS::begin();

  Serial.print("Opening the file...");
  fp = SD.open(fileName, FILE_WRITE);
  if (fp == false) {
    Serial.println("cannot open the file");
  } else {
    Serial.println("OK");
    fp.println("time, altitude, latitude, longitude, ax, ay, az, q");
  }
  
  Serial.println("press the tact switch or Web UI button...");
  while(digitalRead(tact) == HIGH && isRemoteSdActive == false) {
    delay(10);
  }
  delay(10);
}

void loop() {
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

    measureData.push_back(altitude);
    measureData.push_back(latitude);
    measureData.push_back(longitude);
    measureData.push_back(ax);
    measureData.push_back(ay); 
    measureData.push_back(az);

    Serial.print(altitude); Serial.print(", ");
    Serial.print(latitude); Serial.print(", ");
    Serial.print(longitude); Serial.print(", ");
    Serial.print(ax); Serial.print(", ");
    Serial.print(ay); Serial.print(", ");
    Serial.println(az);

    String jsonString = "{";
    jsonString += "\"ax\":" + String(ax, 2) + ",";
    jsonString += "\"ay\":" + String(ay, 2) + ",";
    jsonString += "\"az\":" + String(az, 2) + ",";
    jsonString += "\"latitude\":" + String(latitude, 6) + ",";
    jsonString += "\"longitude\":" + String(longitude, 6) + ",";
    jsonString += "\"altitude\":" + String(altitude, 1);
    jsonString += "}";

    ws.textAll(jsonString);

    if(digitalRead(tact) == LOW || isRemoteSdActive == false){
      save(true);
    }
    if(measureData.size() >= 2100){
      save(false);
    }
    lastTime = millis();
  }
}

void save(bool end) {
  int i = 0;
  digitalWrite(SD_CS, LOW);
  for (double data : measureData) {
    fp.print(data);
    i++;
    if (i % 7 == 0) { 
      fp.println();
    } else {
      fp.print(",");
    }
  }
  if (end == false) { 
    fp.flush();
    Serial.println("saved data");
    measureData.clear();
    digitalWrite(SD_CS, HIGH);
  } else {
    fp.close(); 
    Serial.println("saved data and closed file");
    measureData.clear();
    while (1);
  }
}