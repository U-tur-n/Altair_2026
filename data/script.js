// カメラに関する設定
const targetIp = window.location.hostname
  ? window.location.hostname
  : "192.168.4.1";
const wsUri = `ws://${targetIp}/ws`;

let isCameraActive = false; // カメラの状態を記憶する変数
const cameraBtn = document.getElementById("btn-camera");
const cameraImg = document.getElementById("camera-stream");

cameraBtn.addEventListener("click", function () {
  if (!isCameraActive) {
    // --- カメラを開始する処理 ---
    cameraImg.src = `http://${targetIp}:81/stream`; // URLをセットして通信開始

    // ボタンの見た目を「停止」用に変更
    cameraBtn.innerText = "カメラ映像を停止";
    cameraBtn.style.backgroundColor = "#ffb3b3"; // 警告色（薄い赤）にして分かりやすく
    isCameraActive = true;
  } else {
    // --- カメラを停止する処理 ---
    cameraImg.removeAttribute("src"); // src属性を削除して通信を強制切断

    // ボタンの見た目を「開始」用に戻す
    cameraBtn.innerText = "カメラ映像を取得 (1台のみ)";
    cameraBtn.style.backgroundColor = "#A4C2F4"; // 元のグレーに戻す
    isCameraActive = false;
  }
});

let websocket;

function initWebSocket() {
  console.log(`Connecting to WebSocket: ${wsUri}`);
  websocket = new WebSocket(wsUri);

  websocket.onopen = function (evt) {
    document.getElementById("ws-status").innerText = "接続中";
    document.getElementById("ws-status").style.color = "green";
  };

  websocket.onclose = function (evt) {
    document.getElementById("ws-status").innerText = "切断";
    document.getElementById("ws-status").style.color = "red";
    setTimeout(initWebSocket, 2000);
  };

  websocket.onmessage = function (evt) {
    try {
      const data = JSON.parse(evt.data);
      if (data.ax !== undefined)
        document.getElementById("ax").innerText = data.ax;
      if (data.ay !== undefined)
        document.getElementById("ay").innerText = data.ay;
      if (data.az !== undefined)
        document.getElementById("az").innerText = data.az;
      if (data.latitude !== undefined)
        document.getElementById("lat").innerText = data.latitude;
      if (data.longitude !== undefined)
        document.getElementById("lng").innerText = data.longitude;
      if (data.altitude !== undefined)
        document.getElementById("alt").innerText = data.altitude;
      if (data.msg !== undefined) {
        const statusBox = document.getElementById("status-msg");
        statusBox.innerText += data.msg + "\n"; // メッセージを改行付きで追記
        statusBox.scrollTop = statusBox.scrollHeight; // 最下部へ自動スクロール
      }
    } catch (e) {
      console.error("JSON Error: ", e);
    }
  };
}

window.addEventListener("load", initWebSocket, false);

// 「SD記録 開始」ボタンのクリックイベント
document.querySelector('.btn-sd').addEventListener('click', function() {
    // WebSocketが接続されているか確認
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send("CMD_SD"); // ESP32にコマンドを送信
        console.log("Sent: CMD_SD");
    } else {
        alert("マイコンと接続されていません！");
    }
});

// 「電装動作」ボタンのクリックイベント
// document.querySelector('.btn-pwr').addEventListener('click', function() {
//     if (websocket && websocket.readyState === WebSocket.OPEN) {
//         websocket.send("CMD_PWR");
//         console.log("Sent: CMD_PWR");
//     } else {
//         alert("マイコンと接続されていません！");
//     }
// });
