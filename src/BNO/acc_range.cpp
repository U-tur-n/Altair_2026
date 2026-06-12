#include <Arduino.h>

#include <Wire.h>

uint8_t deviceAddress = 0x28;
uint8_t oprModeAddress = 0x3D; //オペレーションモード変更アドレス
uint8_t configMode = 0x00; //コンフィグモード設定値
uint8_t fusionMode = 0x0C; //フュージョンモード設定値
uint8_t pageIDAddress = 0X07; //レジスタマップ変更アドレス
uint8_t page0 = 0x00;
uint8_t page1 = 0x01;
uint8_t acc_configAddress = 0x08; //加速度設定変更アドレス
uint8_t accRange16g = 0b00001111; //オペレーションモード:ノーマル, バンド幅:62.5Hz, 加速度検出レンジ:16G
uint8_t acc_configAddressData;

void setConfigMode(void);
void setAccRange(void);
void setPage1(void);

void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(115200);
  while(!Serial);
  setPage1(); //レジスタマップをページ1に設定
  setAccRange(); //加速度検出レンジを16Gに変更し，レジスタの値をシリアルモニタに表示する。
}

void loop() {
  // put your main code here, to run repeatedly:

}

void setConfigMode() {
  Wire.beginTransmission(deviceAddress);
  Wire.write(oprModeAddress);
  Wire.write(configMode);
  Wire.endTransmission(true);
  delay(25);
}
void setPage1() {
  Wire.beginTransmission(deviceAddress);
  Wire.write(pageIDAddress);
  Wire.write(page1);
  Wire.endTransmission(true);
  delay(25);
}

void setAccRange() {
  //値の書き込み
  Wire.beginTransmission(deviceAddress);
  Wire.write(acc_configAddress);
  Wire.write(accRange16g);
  Wire.endTransmission(true);


  //値の読み取り, シリアルモニタへの書き出し
  Wire.beginTransmission(deviceAddress);
  Wire.write(acc_configAddress);
  Wire.endTransmission(true);
  Wire.requestFrom(deviceAddress, 1, true);
  acc_configAddressData = Wire.read();
  Serial.println(acc_configAddressData, BIN);
  delay(25);
}