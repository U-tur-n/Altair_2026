const targetIp = window.location.hostname
  ? window.location.hostname
  : "192.168.4.1";
const wsUri = `ws://${targetIp}/ws`;

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
      if (data.msg !== undefined)
        document.getElementById("status-msg").innerText = data.msg;
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
