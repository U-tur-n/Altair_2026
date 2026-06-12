#pragma once          //この行は消さない
#include <Arduino.h>  //この行は消さない

//ライブラリのインクルードはここで行う
#include <TinyGPS++.h>

// 名前空間ここから
namespace GPS {
    // 他の人が読み取るための変数を変数名のみ宣言．ヘッダーファイルに変数を宣言する際は必ず"extern"をつけること．
    extern float latitude; //緯度
    extern float longitude; //経度

    // 共通の関数(自作関数もここで呼び出す)
    void begin();     //setup()の代わり
    void execute();   //loop()の代わり
}