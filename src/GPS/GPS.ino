//ヘッダーファイルをインクルード
#include "GPS.h"

// ライブラリのインクルードは行わない(ヘッダーファイルですでに行っているため)

void setup() {
    GPS::begin();      //ヘッダーファイルで定義したbegin()を呼び出す
}

void loop() {
    GPS::execute();    //ヘッダーファイルで定義したexecute()を呼び出す
}