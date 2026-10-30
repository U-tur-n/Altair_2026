/* 動画の時刻だけを入力とする、再現性のあるダミーテレメトリ。
 * 機体座標: zが機軸。加速度は重力を含む加速度計の表示値のイメージ。
 * オイラー角は ZYX: x=roll, y=pitch, z=yaw。実測・姿勢推定ではありません。
 */
(function (root) {
  "use strict";
  const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));
  const mix = (a, b, u) => a + (b - a) * u;
  const smooth = (u) => { u = clamp(u, 0, 1); return u * u * (3 - 2 * u); };
  const wrap = (angle) => ((angle + 180) % 360 + 360) % 360 - 180;
  const vibration = (t, scale) => scale * (0.65 * Math.sin(t * 37.7) + 0.35 * Math.sin(t * 61.3));
  const defaultNoise = { acceleration: 0.06, euler: 0.15, position: 0.03, altitude: 0.08 };

  function sampleTime(seconds, config) {
    const t = Number.isFinite(seconds) ? Math.max(0, seconds) : 0;
    const interval = config.telemetryInterval ?? 0.5;
    return Math.floor(t / interval + 1e-9) * interval;
  }

  // 各軸・各サンプルで異なる擬似乱数。時刻から決まるのでシークしても同じ値に戻る。
  function noise(seconds, channel, config) {
    const tick = Math.floor(seconds / (config.telemetryInterval ?? 0.5) + 1e-9);
    let value = Math.imul(tick + 1, 0x9e3779b1) ^ Math.imul(channel, 0x85ebca6b);
    value = Math.imul(value ^ (value >>> 16), 0x7feb352d);
    value = Math.imul(value ^ (value >>> 15), 0x846ca68b);
    value ^= value >>> 16;
    return (value >>> 0) / 0xffffffff * 2 - 1;
  }

  function validate(config) {
    const keys = ["launch", "burnout", "apogee", "parachute", "landing"];
    let previous = -1;
    for (const key of keys) {
      const value = config.timeline?.[key];
      if (!Number.isFinite(value) || value < 0 || value <= previous) {
        throw new Error("timeline は 0 ≤ launch < burnout < apogee < parachute < landing の秒数で指定してください。");
      }
      previous = value;
    }
    for (const key of ["maxAltitude", "driftX", "driftY"]) {
      if (!Number.isFinite(config[key])) throw new Error(`${key} は有限の数値にしてください。`);
    }
    if (config.maxAltitude <= 0) throw new Error("maxAltitude は正の数値にしてください。");
    if (typeof config.videoSrc !== "string" || !config.videoSrc.trim()) throw new Error("videoSrc に動画のパスを指定してください。");
    if (!Number.isFinite(config.telemetryInterval ?? 0.5) || (config.telemetryInterval ?? 0.5) <= 0) {
      throw new Error("telemetryInterval は正の秒数にしてください。");
    }
    for (const [key, fallback] of Object.entries(defaultNoise)) {
      const amplitude = config.sensorNoise?.[key] ?? fallback;
      if (!Number.isFinite(amplitude) || amplitude < 0) throw new Error(`sensorNoise.${key} は0以上の有限の数値にしてください。`);
    }
    if (config.attitude !== undefined) {
      if (!Array.isArray(config.attitude) || config.attitude.length < 2) throw new Error("attitude には2点以上の姿勢データが必要です。");
      let last = -1;
      for (const row of config.attitude) {
        if (!Array.isArray(row) || row.length !== 4 || !row.every(Number.isFinite) || row[0] < 0 || row[0] <= last) {
          throw new Error("attitude は [秒, roll, pitch, yaw] を時刻の昇順で指定してください。");
        }
        last = row[0];
      }
    }
    return config;
  }

  function attitudeAt(t, points) {
    if (t <= points[0][0]) return points[0].slice(1);
    for (let i = 1; i < points.length; i++) {
      if (t <= points[i][0]) {
        const a = points[i - 1], b = points[i];
        const u = smooth((t - a[0]) / (b[0] - a[0]));
        // 角度は連続する値で設定し、最後に±180度に正規化。
        return [1, 2, 3].map((j) => mix(a[j], b[j], u));
      }
    }
    return points[points.length - 1].slice(1);
  }

  function sample(seconds, config) {
    const t = Number.isFinite(seconds) ? Math.max(0, seconds) : 0;
    const { launch, burnout, apogee, parachute, landing } = config.timeline;
    let phase, ax = 0, ay = 0, az = 9.80665, ox = 0, oy = 0, oz = 0;
    const flight = clamp((t - launch) / (landing - launch), 0, 1);
    // 上昇・降下を連続した曲線で作成。高度や位置も説明用のダミーです。
    let alt = t < apogee
      ? config.maxAltitude * Math.sin(clamp((t - launch) / (apogee - launch), 0, 1) * Math.PI / 2) ** 2
      : config.maxAltitude * (1 - smooth((t - apogee) / (landing - apogee)));
    if (t < launch) {
      phase = "発射前";
      ax = vibration(t, 0.015); ay = vibration(t + 0.3, 0.015); az += vibration(t, 0.025);
    } else if (t < burnout) {
      phase = "燃焼・上昇";
      const u = (t - launch) / (burnout - launch);
      const ignition = smooth((t - launch) / 0.12);
      const cutoff = smooth((burnout - t) / 0.15);
      az = mix(9.80665, 55 + 8 * Math.sin(u * Math.PI), ignition) * cutoff + vibration(t, 1.2) * ignition * cutoff;
      ax = vibration(t, 1.8) * ignition; ay = vibration(t + 0.3, 1.4) * ignition;
      ox = 38 * u; oy = 4 * Math.sin(u * Math.PI); oz = 12 * u;
    } else if (t < parachute) {
      phase = t < apogee ? "慣性上昇" : "頂点・降下開始";
      const u = (t - burnout) / (parachute - burnout);
      az = vibration(t, 0.08); ax = vibration(t, 0.1); ay = vibration(t + 0.3, 0.1);
      ox = 38 + 150 * u; oy = 20 * Math.sin(u * Math.PI / 2); oz = 12 + 65 * u;
    } else if (t < landing) {
      phase = "パラシュート降下";
      const d = t - parachute;
      const opening = smooth(d / 0.3);
      const swing = Math.sin(d * 2.8) * (1 - Math.exp(-d));
      az = opening * (9.80665 + 22 * Math.exp(-(((d - 0.4) / 0.18) ** 2)) + 0.7 * swing);
      ax = 1.2 * swing; ay = 0.8 * Math.sin(d * 2.1) * opening;
      ox = 188 + 15 * swing; oy = 20 + 10 * swing; oz = 77 + 7 * d;
    } else {
      phase = "着地";
      const settle = Math.exp(-(t - landing) * 9);
      az = 9.80665 + 28 * settle;
      ax = 4 * settle; ay = -3 * settle;
      const d = landing - parachute;
      const swing = Math.sin(d * 2.8) * (1 - Math.exp(-d));
      ox = mix(270, 188 + 15 * swing, settle);
      oy = (20 + 10 * swing) * settle; oz = 77 + 7 * d;
    }
    if (config.attitude) [ox, oy, oz] = attitudeAt(t, config.attitude);
    if (t >= landing) {
      const r = ox * Math.PI / 180, p = oy * Math.PI / 180;
      const shock = Math.exp(-(t - landing) * 9);
      ax = -9.80665 * Math.sin(p) + 4 * shock;
      ay = 9.80665 * Math.sin(r) * Math.cos(p) - 3 * shock;
      az = 9.80665 * Math.cos(r) * Math.cos(p) + 28 * shock;
    }
    const amplitude = (key) => config.sensorNoise?.[key] ?? defaultNoise[key];
    ax += noise(t, 1, config) * amplitude("acceleration");
    ay += noise(t, 2, config) * amplitude("acceleration");
    az += noise(t, 3, config) * amplitude("acceleration");
    ox += noise(t, 4, config) * amplitude("euler");
    oy += noise(t, 5, config) * amplitude("euler");
    oz += noise(t, 6, config) * amplitude("euler");
    const x_dist = config.driftX * smooth(flight) + noise(t, 7, config) * amplitude("position");
    const y_dist = config.driftY * smooth(flight) + noise(t, 8, config) * amplitude("position");
    alt += noise(t, 9, config) * amplitude("altitude");
    return { ax, ay, az, ox: wrap(ox), oy: wrap(oy), oz: wrap(oz), x_dist, y_dist,
      dist: Math.hypot(x_dist, y_dist), alt, phase };
  }
  const api = { validate, sample, sampleTime };
  if (typeof module === "object" && module.exports) module.exports = api;
  else root.FlightTelemetry = api;
})(typeof globalThis !== "undefined" ? globalThis : this);
