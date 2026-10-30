const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const { sample, validate } = require('../telemetry.js');
const context = { window: {} };
vm.runInNewContext(fs.readFileSync(path.join(__dirname, '../replay-config.js'), 'utf8'), context);
const config = context.window.FLIGHT_REPLAY_CONFIG;

test('video timeline is valid and invalid ordering is rejected', () => {
  validate(config);
  assert.throws(() => validate({ ...config, timeline: { ...config.timeline, landing: 0 } }));
  assert.throws(() => validate({ ...config, attitude: [[0, 0, 0, 0], [0, 0, 0, 0]] }));
});

test('all video frames have finite telemetry and normalized angles', () => {
  for (let t = 0; t <= 35.8; t += 1 / 30) {
    const data = sample(t, config);
    for (const [key, value] of Object.entries(data)) {
      if (key !== 'phase') assert.ok(Number.isFinite(value), `${key} at ${t}`);
    }
    for (const key of ['ox', 'oy', 'oz']) assert.ok(data[key] >= -180 && data[key] < 180);
    assert.ok(data.alt >= 0 && data.alt <= config.maxAltitude);
    assert.ok(Math.abs(data.dist - Math.hypot(data.x_dist, data.y_dist)) < 1e-9);
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
  assert.equal(sample(config.timeline.apogee, config).alt, config.maxAltitude);
  assert.equal(sample(35, config).alt, 0);
  const rest = sample(35, config);
  assert.ok(Math.abs(Math.hypot(rest.ax, rest.ay, rest.az) - 9.80665) < 1e-6);
});
