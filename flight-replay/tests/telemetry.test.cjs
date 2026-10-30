const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const { sample, validate, sampleTime } = require('../telemetry.js');
const context = { window: {} };
vm.runInNewContext(fs.readFileSync(path.join(__dirname, '../replay-config.js'), 'utf8'), context);
const config = context.window.FLIGHT_REPLAY_CONFIG;
const noiseless = { ...config, sensorNoise: { acceleration: 0, euler: 0 } };

test('video timeline is valid and invalid ordering is rejected', () => {
  validate(config);
  assert.throws(() => validate({ ...config, timeline: { ...config.timeline, landing: 0 } }));
  assert.throws(() => validate({ ...config, attitude: [[0, 0, 0, 0], [0, 0, 0, 0]] }));
  for (const telemetryInterval of [0, -1, NaN, Infinity]) {
    assert.throws(() => validate({ ...config, telemetryInterval }));
  }
  assert.throws(() => validate({ ...config, sensorNoise: { euler: -0.1 } }));
});

test('all video frames have finite telemetry and normalized angles', () => {
  for (let t = 0; t <= 35.8; t += 1 / 30) {
    const data = sample(t, config);
    for (const [key, value] of Object.entries(data)) {
      if (key !== 'phase') assert.ok(Number.isFinite(value), `${key} at ${t}`);
    }
    for (const key of ['ox', 'oy', 'oz']) assert.ok(data[key] >= -180 && data[key] < 180);
  }
});

test('seek and replay return the same values regardless of call order', () => {
  const first = sample(19.5, config);
  sample(35.7, config); sample(0, config); sample(6.2, config);
  assert.deepEqual(sample(19.5, config), first);
});

test('video events drive launch, attitude change, and landing', () => {
  assert.equal(sample(1, config).phase, '発射前');
  assert.ok(sample(3.5, config).az > 40);
  assert.ok(Math.abs(sample(6, config).az) < 0.2);
  assert.equal(sample(14.5, config).phase, 'パラシュート降下');
  assert.ok(Math.abs(sample(15.2, config).ox) > 150);
  assert.equal(sample(24, config).phase, '着地');
  const rest = sample(35, noiseless);
  assert.ok(Math.abs(Math.hypot(rest.ax, rest.ay, rest.az) - 9.80665) < 1e-6);
});

test('every displayed sensor value fluctuates before launch and after landing', () => {
  for (const start of [0, 27]) {
    const readings = Array.from({ length: 6 }, (_, i) => sample(start + i * 0.5, config));
    for (const key of ['ax', 'ay', 'az', 'ox', 'oy', 'oz']) {
      assert.ok(new Set(readings.map(row => row[key].toFixed(2))).size > 1, `${key} must fluctuate at ${start}s`);
    }
    for (let i = 0; i < readings.length; i++) {
      const baseline = sample(start + i * 0.5, noiseless);
      for (const [key, group] of Object.entries({ ax: 'acceleration', ay: 'acceleration', az: 'acceleration',
        ox: 'euler', oy: 'euler', oz: 'euler' })) {
        assert.ok(Math.abs(readings[i][key] - baseline[key]) <= config.sensorNoise[group] + 1e-9, `${key} noise exceeds amplitude`);
      }
    }
  }
});

test('only requested acceleration and Euler angle values are generated', () => {
  assert.deepEqual(Object.keys(sample(10, config)).sort(), ['ax', 'ay', 'az', 'ox', 'oy', 'oz', 'phase'].sort());
});

test('values are sampled at half-second video timestamps, including backward seeks', () => {
  for (const [t, expected] of [[0, 0], [0.1, 0], [0.49, 0], [0.5, 0.5], [0.9, 0.5],
    [26.6, 26.5], [26.99, 26.5], [27, 27], [1.1, 1], [0, 0]]) {
    assert.equal(sampleTime(t, config), expected);
  }
  assert.equal(sampleTime(3.5, { ...config, telemetryInterval: 1 }), 3);
  assert.deepEqual(sample(sampleTime(27.1, config), config), sample(sampleTime(27.4, config), config));
  assert.notDeepEqual(sample(sampleTime(27.49, config), config), sample(sampleTime(27.5, config), config));
  const saved = sample(sampleTime(27.5, config), config);
  sample(sampleTime(35.1, config), config);
  sample(sampleTime(0.2, config), config);
  assert.deepEqual(sample(sampleTime(27.5, config), config), saved);
});
