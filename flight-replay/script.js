/* 飛行映像と同期した再生専用デモ。時刻源は常に video.currentTime。 */
(function () {
  "use strict";
  const $ = (id) => document.getElementById(id);
  const video = $("camera-stream");
  const status = $("ws-status");
  const notice = $("video-notice");
  const retry = $("retry-play");
  const fields = ["ax", "ay", "az", "ox", "oy", "oz", "x_dist", "y_dist", "dist", "alt"];
  let objectUrl = null;
  let previousPhase = "";
  let previousTime = -1;
  let lastDraw = -Infinity;
  let lastTelemetryTime = null;
  let config;
  const logLines = [];

  function formatTime(seconds) {
    const centiseconds = Math.floor(Math.max(0, seconds) * 100);
    const minutes = Math.floor(centiseconds / 6000);
    const rest = centiseconds % 6000;
    return `${String(minutes).padStart(2, "0")}:${String(Math.floor(rest / 100)).padStart(2, "0")}.${String(rest % 100).padStart(2, "0")}`;
  }
  function log(message) {
    logLines.push(`[${formatTime(video.currentTime || 0)}] ${message}`);
    if (logLines.length > 60) logLines.shift();
    const box = $("status-msg");
    box.textContent = logLines.join("\n");
    box.scrollTop = box.scrollHeight;
  }
  function setStatus(text) { status.textContent = text; }
  function showNotice(message, canRetry = false) {
    $("video-notice-text").textContent = message;
    retry.hidden = !canRetry;
    notice.hidden = false;
  }
  function clearTelemetry() {
    lastTelemetryTime = null;
    for (const field of fields) $(field).textContent = "-";
    $("replay-time").textContent = "00:00.00";
    $("flight-phase").textContent = "映像待ち";
  }
  function render() {
    if (video.readyState < 1 || video.error) return;
    const t = video.currentTime;
    $("replay-time").textContent = formatTime(t);
    if (t < previousTime - 0.2) log("巻き戻し・リピート：映像時刻から再計算");
    // 同じ0.5秒区間は値を保持。pause/timeupdate等の追加イベントでも更新しない。
    const telemetryTime = FlightTelemetry.sampleTime(t, config);
    if (telemetryTime !== lastTelemetryTime) {
      const sample = FlightTelemetry.sample(telemetryTime, config);
      for (const field of fields) {
        const value = Math.abs(sample[field]) < 0.005 ? 0 : sample[field];
        $(field).textContent = value.toFixed(2);
      }
      $("flight-phase").textContent = sample.phase;
      if (sample.phase !== previousPhase) log(sample.phase);
      previousPhase = sample.phase;
      lastTelemetryTime = telemetryTime;
    }
    previousTime = t;
  }
  async function playAutomatically() {
    try {
      await video.play();
    } catch (error) {
      // 動画の読み込みエラーは error ハンドラで案内。選択直後の中断は無視。
      if (error.name === "AbortError" || video.error) return;
      setStatus("自動再生待ち");
      showNotice("ブラウザが自動再生を制限しています。「映像を再生」で開始できます。", true);
      log("自動再生が制限されました");
    }
  }

  function adjustDashboardScale() {
    const dashboard = document.querySelector(".dashboard");
    dashboard.style.transform = window.innerWidth <= 768 ? "" :
      `scale(${Math.min(window.innerWidth * 0.95 / 1100, window.innerHeight * 0.95 / 750)})`;
  }
  adjustDashboardScale();
  window.addEventListener("resize", adjustDashboardScale);

  try {
    config = FlightTelemetry.validate(window.FLIGHT_REPLAY_CONFIG);
  } catch (error) {
    setStatus("設定エラー");
    showNotice(error.message);
    $("video-file").disabled = true;
    clearTelemetry();
    log(error.message);
    return;
  }
  $("profile-note").textContent = config.profileConfirmed ? "映像に合わせたダミーデータ" : "仮の飛行タイミング";
  video.muted = true; // 音声付きの自動再生制限を避ける。音声はネイティブ操作で有効化可。
  video.loop = Boolean(config.loop);
  video.addEventListener("loadedmetadata", () => {
    render();
    const duration = video.duration;
    if (Number.isFinite(duration) && duration < config.timeline.landing) {
      log("動画が設定の着地時刻より短いため、replay-config.js の時刻を調整してください");
    }
    playAutomatically();
  });
  video.addEventListener("playing", () => { notice.hidden = true; setStatus("再生中（ダミー）"); render(); });
  video.addEventListener("pause", () => { render(); if (!video.error) setStatus(video.ended ? "再生終了" : "一時停止"); });
  video.addEventListener("waiting", () => { render(); setStatus("映像を読み込み中"); });
  video.addEventListener("seeking", () => { render(); setStatus("シーク中"); });
  video.addEventListener("seeked", () => { render(); setStatus(video.paused ? "一時停止" : "再生中（ダミー）"); });
  video.addEventListener("timeupdate", render);
  video.addEventListener("ended", () => { render(); setStatus("再生終了"); });
  video.addEventListener("error", () => {
    clearTelemetry();
    setStatus("動画を読み込めません");
    showNotice("飛行動画を読み込めません。media/flight.mp4 の配置、または動画形式を確認してください。端末の動画を選択することもできます。");
    log("動画読み込みエラー");
  });
  retry.addEventListener("click", playAutomatically);
  $("video-file").addEventListener("change", (event) => {
    const file = event.target.files[0];
    if (!file) return;
    video.pause();
    if (objectUrl) URL.revokeObjectURL(objectUrl);
    objectUrl = URL.createObjectURL(file);
    previousPhase = ""; previousTime = -1;
    clearTelemetry();
    showNotice("選択した飛行動画を読み込んでいます…");
    video.src = objectUrl;
    video.load();
    log(`動画を変更：${file.name}（飛行タイミングは設定ファイルで調整）`);
  });
  // 右側のボタンは表示上のデモ操作。どちらも動画やテレメトリの開始条件にしません。
  const btnSd = document.querySelector(".btn-sd");
  const btnPwr = document.querySelector(".btn-pwr");
  btnSd.addEventListener("click", () => {
    btnSd.disabled = true;
    btnSd.title = "ＳＤ記録停止済み（デモ）";
    log("ＳＤ記録停止（デモ）。映像・テレメトリの再生は継続します");
  });
  btnPwr.addEventListener("click", () => {
    btnPwr.disabled = true;
    btnPwr.title = "電装動作済み（デモ）";
    log("電装動作（デモ）");
  });
  window.addEventListener("pagehide", () => { if (objectUrl) URL.revokeObjectURL(objectUrl); });

  // 独立した経過時間を加算せず、描画時に必ず動画の現在時刻を参照します。
  function tick(now) {
    if (!video.paused && !video.seeking && now - lastDraw >= 1000 / 30) {
      render(); lastDraw = now;
    }
    requestAnimationFrame(tick);
  }
  requestAnimationFrame(tick);
  clearTelemetry();
  log("飛行リプレイ：全センサー値はダミーデータです");
  video.src = config.videoSrc;
  video.load();
})();
