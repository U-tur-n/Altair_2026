// main の画面に飛行動画とダミー値を流すための再生処理。
// 表示の追加・ログ出力は行わず、数値は動画時刻の0.5秒ごとに更新する。
const video = document.getElementById("camera-stream");
const cameraBtn = document.getElementById("btn-camera");
const connection = document.getElementById("ws-status");
const fields = ["ax", "ay", "az", "ox", "oy", "oz"];
const config = FlightTelemetry.validate(window.FLIGHT_REPLAY_CONFIG);
let lastTelemetryTime = null;

function updateTelemetry() {
  if (video.readyState < 1 || video.error) return;
  const time = FlightTelemetry.sampleTime(video.currentTime, config);
  if (time === lastTelemetryTime) return;
  const data = FlightTelemetry.sample(time, config);
  for (const field of fields) {
    const value = Math.abs(data[field]) < 0.005 ? 0 : data[field];
    document.getElementById(field).innerText = value.toFixed(2);
  }
  lastTelemetryTime = time;
}

function updateCameraButton(active) {
  cameraBtn.innerText = active ? "カメラ映像を停止" : "カメラ映像を取得 (1台のみ)";
  cameraBtn.style.backgroundColor = active ? "#ffb3b3" : "#A4C2F4";
}

async function playVideo() {
  try {
    await video.play();
  } catch (error) {
    // 自動再生がブラウザに制限された場合も、元のカメラボタンから再生できる。
    updateCameraButton(false);
  }
}

video.muted = true;
video.loop = Boolean(config.loop);
video.addEventListener("loadedmetadata", () => {
  lastTelemetryTime = null;
  updateTelemetry();
  playVideo();
});
video.addEventListener("playing", () => {
  connection.innerText = "接続中";
  connection.style.color = "green";
  updateCameraButton(true);
});
video.addEventListener("pause", () => updateCameraButton(false));
for (const event of ["timeupdate", "seeked", "pause", "ended"]) {
  video.addEventListener(event, updateTelemetry);
}
video.addEventListener("error", () => {
  connection.innerText = "切断";
  connection.style.color = "red";
  updateCameraButton(false);
  lastTelemetryTime = null;
  for (const field of fields) document.getElementById(field).innerText = "-";
});
cameraBtn.addEventListener("click", () => {
  if (video.paused) playVideo();
  else video.pause();
});
// SD記録中の色は main の既存処理に合わせる。再生はボタン操作と独立。
document.querySelector(".btn-sd").style.backgroundColor = "#FFD700";
video.src = config.videoSrc;
function tick() {
  if (!video.paused && !video.seeking) updateTelemetry();
  requestAnimationFrame(tick);
}
requestAnimationFrame(tick);

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
