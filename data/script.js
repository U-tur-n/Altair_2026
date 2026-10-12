// カメラに関する設定
const targetIp = window.location.hostname
  ? window.location.hostname
  : "192.168.4.1";
const wsUri = `ws://${targetIp}/ws`;

let isCameraActive = false; // 「自分の端末」でカメラを開いているか
let globalCameraActive = false; // 「全体」で誰かがカメラを開いているか
const cameraBtn = document.getElementById("btn-camera");
const cameraImg = document.getElementById("camera-stream");

cameraBtn.addEventListener("click", function () {
  if (!isCameraActive) {
    // --- カメラを開始する処理 ---

    // ▼ 追加：既に誰かが使っていたらポップアップを出して処理を中断する ▼
    if (globalCameraActive) {
      alert(
        "既に別の端末でカメラが接続済みです。\n映像を見るには、現在接続中の端末で停止してください。",
      );
      return;
    }

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
        document.getElementById("alt").innerText = data.altitude;
      if (data.x_dist !== undefined)
        document.getElementById("x_dist").innerText = data.x_dist;
      if (data.y_dist !== undefined)
        document.getElementById("y_dist").innerText = data.y_dist;
      if (data.dist !== undefined)
        document.getElementById("dist").innerText = data.dist;
      if (data.sd_active !== undefined) {
        if (data.sd_active !== isRemoteSdActive) {
          isRemoteSdActive = data.sd_active;
          if (isRemoteSdActive) {
            btnSd.innerHTML = "SD記録<br>停止";
            btnSd.style.backgroundColor = "#FFD700"; // 黄色
          } else {
            btnSd.innerHTML = "SD記録<br>開始";
            btnSd.style.backgroundColor = "#a4c2f4"; // 青色
          }
        }
      }
      if (data.pwr_active !== undefined) {
        if (data.pwr_active !== isRemotePwrActive) {
          isRemotePwrActive = data.pwr_active;
          if (isRemotePwrActive) {
            btnPwr.innerHTML = "電装動作済";
            btnPwr.style.backgroundColor = "#b9b9b9"; // 黄色
          } else {
            btnPwr.innerHTML = "電装動作";
            btnPwr.style.backgroundColor = "#dc3545"; // 青色
          }
        }
      }
        if (data.cam_active !== undefined) {
          globalCameraActive = data.cam_active;
        }
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

// --- ページ読み込み完了時の処理 ---
window.addEventListener("load", function() {
  // ① アクセスしてきた端末・ブラウザの情報を取得
  const ua = navigator.userAgent;
  
  // ② iOS(iPhone/iPad/iPod) または Mac版Safariかどうかの判定条件
  const isIOS = /iPhone|iPad|iPod/i.test(ua) || (navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1);
  const isSafari = /Safari/i.test(ua) && !/Chrome/i.test(ua) && !/Edg/i.test(ua);

// ③ 該当するブラウザ(WebKit系)だった場合、カメラのUIを非表示にし、メッセージを表示
  if (isIOS || isSafari) {
    const btnCamera = document.getElementById("btn-camera");
    const cameraStream = document.getElementById("camera-stream");

    // 元のボタンとカメラ枠を非表示
    btnCamera.style.display = "none";
    cameraStream.style.display = "none";

    // メッセージを表示する要素を動的に作成
    const unsupportedMsg = document.createElement("div");
    unsupportedMsg.className = "video-feed"; // 既存のカメラ枠のCSSクラスを流用（スマホでのサイズ可変にも対応）
    unsupportedMsg.innerHTML =
      "iOS, iPadOS, macOSでは<br>カメラの表示ができません";

    // テキストを中央配置・装飾するための追加スタイル
    unsupportedMsg.style.display = "flex";
    unsupportedMsg.style.justifyContent = "center";
    unsupportedMsg.style.alignItems = "center";
    unsupportedMsg.style.color = "#ffb3b3"; // 警告の薄い赤色
    unsupportedMsg.style.textAlign = "center";
    unsupportedMsg.style.fontWeight = "bold";
    unsupportedMsg.style.fontSize = "18px";
    unsupportedMsg.style.border = "1px solid #dc3545"; // 赤枠をつけて目立たせる
    unsupportedMsg.style.boxSizing = "border-box";
    // 高さを強制的に変更・固定するための追加指定
    unsupportedMsg.style.flexGrow = "0"; // 自動拡張を無効化
    unsupportedMsg.style.minHeight = "95px"; // CSSの320pxを上書き
    unsupportedMsg.style.height = "95px"; // 高さを95pxに固定

    // カメラ領域（col-left）の、cameraStreamの直後にメッセージ要素を挿入
    cameraStream.parentNode.insertBefore(
      unsupportedMsg,
      cameraStream.nextSibling,
    );

    console.log(
      "WebKit/iOS detected: Camera UI replaced with unsupported message.",
    );
  }

  // ④ 既存のWebSocket接続を開始
  initWebSocket();
}, false);


// SD記録の状態を記憶する変数
let isRemoteSdActive = false;
const btnSd = document.querySelector('.btn-sd');
// 「SD記録 開始」ボタンのクリックイベント
document.querySelector('.btn-sd').addEventListener('click', function() {
  // WebSocketが接続されているか確認
  if (websocket && websocket.readyState === WebSocket.OPEN) {
    if (isRemoteSdActive === true) {
      const result = confirm("SDの記録を停止しますか？");
      if (!result) {
        return; // ユーザーがキャンセルした場合は処理を中断
      }
    }
      websocket.send("CMD_SD"); // ESP32にコマンドを送信
      console.log("Sent: CMD_SD");

      // UIの状態を反転させる
      // if (!isRemoteSdActive) {
      //   // 記録開始状態へ変更
      //   btnSd.innerHTML = "SD記録<br>停止"; // <br>で改行を維持
      //   btnSd.style.backgroundColor = "#FFD700"; // 指定の黄色
      //   isRemoteSdActive = true;
      // } else {
      //   // 記録停止（初期）状態へ戻す
      //   btnSd.innerHTML = "SD記録<br>開始";
      //   btnSd.style.backgroundColor = "#a4c2f4"; // 元の青色
      //   isRemoteSdActive = false;
      // }
    } else {
        alert("マイコンと接続されていません！");
    }
});
let isRemotePwrActive = false;
const btnPwr = document.querySelector(".btn-pwr");
document.querySelector(".btn-pwr").addEventListener("click", function () {
  // WebSocketが接続されているか確認
  if (websocket && websocket.readyState === WebSocket.OPEN) {
    if (isRemotePwrActive === false) {
      const result = confirm("電装動作をしますか？");
      if (!result) {
        return; // ユーザーがキャンセルした場合は処理を中断
      }
    }
    websocket.send("CMD_PWR"); // ESP32にコマンドを送信
    console.log("Sent: CMD_PWR");

    // UIの状態を反転させる
    // if (!isRemoteSdActive) {
    //   // 記録開始状態へ変更
    //   btnSd.innerHTML = "SD記録<br>停止"; // <br>で改行を維持
    //   btnSd.style.backgroundColor = "#FFD700"; // 指定の黄色
    //   isRemoteSdActive = true;
    // } else {
    //   // 記録停止（初期）状態へ戻す
    //   btnSd.innerHTML = "SD記録<br>開始";
    //   btnSd.style.backgroundColor = "#a4c2f4"; // 元の青色
    //   isRemoteSdActive = false;
    // }
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

// =======================================================
// ダッシュボードを画面サイズに合わせて自動で等倍縮小・拡大する処理
// =======================================================
function adjustDashboardScale() {
  // スマホサイズ（768px以下）の時は等倍縮小を解除し、通常のレスポンシブ表示にする
  if (window.innerWidth <= 768) {
    const dashboard = document.querySelector('.dashboard');
    if (dashboard) {
      dashboard.style.transform = '';
    }
    return;
  }

  const dashboard = document.querySelector('.dashboard');
  if (!dashboard) return;

  // CSSで設定したダッシュボードの基準サイズ
  const baseWidth = 1100;
  const baseHeight = 750;

  // 画面の枠いっぱいに張り付かないよう、95%（0.95）のサイズに収めるよう計算
  const scaleX = (window.innerWidth * 0.95) / baseWidth;
  const scaleY = (window.innerHeight * 0.95) / baseHeight;

  // 縦・横のうち、より狭い方に合わせて等倍縮小（Math.min）
  // ※もし元のサイズより大きく（拡大）したくない場合は Math.min(scaleX, scaleY, 1) としてください
  const scale = Math.min(scaleX, scaleY);

  // 計算した倍率をダッシュボード全体に適用
  dashboard.style.transform = `scale(${scale})`;
}

// ページ読み込み時と、ウィンドウのサイズが変更された時に実行する
window.addEventListener('load', adjustDashboardScale);
window.addEventListener('resize', adjustDashboardScale);