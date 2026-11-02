#include "servo.h"

namespace servo {
  // 回路図に合わせてピン番号を調整してください（現在は2番ピン）
  const int servoPin = 2; 
  int DutyCycle = 500;

  void begin() {
    pinMode(servoPin, OUTPUT);
    
    // 【ソフトスタート】1500から500へゆっくり動かす
    // 1回の変化量（ステップ幅）を10に設定し、ピーク電流を抑えます
    for (DutyCycle = 1500; DutyCycle >= 500; DutyCycle -= 10) {
      digitalWrite(servoPin, HIGH);
      delayMicroseconds(DutyCycle);
      digitalWrite(servoPin, LOW);
      delayMicroseconds(20000 - DutyCycle);
    }

    // 終点(500)で少しの間確実に保持する（約0.2秒間）
    for (int i = 0; i < 10; i++) {
      digitalWrite(servoPin, HIGH);
      delayMicroseconds(500);
      digitalWrite(servoPin, LOW);
      delayMicroseconds(20000 - 500);
    }

    // ★追加: 保持が終わったら強制的にLOWに落として待機電流をカット（脱力）
    // これにより、この直後にカメラが起動しても電圧降下を防げます
    digitalWrite(servoPin, LOW);
  }

  void execute() {
    // 【ソフトスタート】500から1500へゆっくり動かす
    for (DutyCycle = 500; DutyCycle <= 1500; DutyCycle += 10) {
      digitalWrite(servoPin, HIGH);
      delayMicroseconds(DutyCycle);
      digitalWrite(servoPin, LOW);
      delayMicroseconds(20000 - DutyCycle);
    }

    // 終点(1500)で少しの間確実に保持する（約0.2秒間）
    for (int i = 0; i < 10; i++) {
      digitalWrite(servoPin, HIGH);
      delayMicroseconds(1500);
      digitalWrite(servoPin, LOW);
      delayMicroseconds(20000 - 1500);
    }

    // ★追加: 保持が終わったら強制的にLOWに落として待機電流をカット（脱力）
    digitalWrite(servoPin, LOW);
  }
}