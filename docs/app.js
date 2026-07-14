/* ============================================================
SENTINEL Dashboard v2 — Fixed Sound, Timeline & Summary
============================================================ */
(function () {
'use strict';
const WS_URL = 'ws://192.168.4.1/ws';
const RECONNECT_BASE = 1000;
const RECONNECT_MAX = 10000;
const TIMELINE_LEN = 20;
const SPARKLINE_LEN = 30;
const GAUGE_CIRCUM = 75;
const MAX_EVENTS = 50;
const STATUS_LABELS = ['SAFE', 'WARN', 'DANGER', 'EMERGENCY'];
const STATUS_CLASSES = ['safe', 'warn', 'danger', 'emergency'];
const CARD_CLASSES = ['status-safe', 'status-warn', 'status-danger', 'status-emergency'];
const FALL_LABELS = ['No Fall', 'Free Fall', 'Impact', 'Watching', 'Fall Confirmed'];
const FALL_CLASSES = ['no-fall', 'free-fall', 'impact', 'watching', 'confirmed'];
const EMERG_LABELS = [
  'None', 'Fall Detected', 'Panic / SOS', 'Gas Detected',
  'Heat Critical', 'Noise Limit', 'Health Alert'
];
const SPARK_COLORS = {
  temp: { line: '#f97316', area: '#f97316' },
  hum: { line: '#3b82f6', area: '#3b82f6' },
  sound: { line: '#8b5cf6', area: '#8b5cf6' },
  hr: { line: '#ef4444', area: '#ef4444' },
  spo2: { line: '#06b6d4', area: '#06b6d4' }
};
const SPARK_RANGES = {
  temp: { min: 20, max: 50 },
  hum: { min: 20, max: 100 },
  sound: { min: 30, max: 110 },
  hr: { min: 40, max: 180 },
  spo2: { min: 80, max: 100 }
};

let ws = null;
let reconnectDelay = RECONNECT_BASE;
let reconnectTimer = null;
let soundEnabled = localStorage.getItem('sentinel-sound') !== 'false';
let darkTheme = localStorage.getItem('sentinel-theme') !== 'light';
let alarmPlaying = false;
let audioCtx = null, alarmOsc = null, alarmGain = null, pulseInterval = null;
let pendingData = null;
let rafScheduled = false;

// Worker state tracking
const workerState = {};
for (let wid = 1; wid <= 2; wid++) {
  workerState[wid] = {
    lastSeenCounter: 0,
    timeline: [],
    active: false,
    prevStatusCode: -1,
    sparklines: { temp: [], hum: [], sound: [], hr: [], spo2: [] },
    shiftStats: { safe: 0, warn: 0, danger: 0, emergency: 0, peakTemp: 0, peakSound: 0, peakHR: 0, incidents: 0 }
  };
}

let events = [];
let shiftStartTime = Date.now();

const domCache = {};
function $(id) {
  if (!domCache[id]) domCache[id] = document.getElementById(id);
  return domCache[id];
}

let dom;
function cacheDom() {
  dom = {
    connDot: $('conn-dot'),
    connText: $('conn-text'),
    banner: $('emergency-banner'),
    soundBtn: $('sound-toggle'),
    themeBtn: $('theme-toggle'),
    uptime: $('stat-uptime'),
    packets: $('stat-packets'),
    alerts: $('stat-alerts'),
    alertsIcon: $('stat-alerts-icon'),
    eventList: $('event-list'),
    eventCount: $('event-count'),
    shiftDur: $('shift-duration'),
    summaryGrid: $('summary-grid')
  };
}

function init() {
  cacheDom();
  applyTheme();
  applySoundState();
  initTimelines();
  dom.themeBtn.addEventListener('click', toggleTheme);
  dom.soundBtn.addEventListener('click', toggleSound);
  setInterval(tickLastSeen, 1000);
  setInterval(updateShiftDuration, 1000);
  
  // Inline alert buttons
  document.querySelectorAll('.alert-send-btn').forEach(btn => {
    btn.addEventListener('click', function() {
      const workerId = parseInt(this.dataset.worker);
      const alertType = parseInt(this.dataset.type);
      sendInlineAlert(workerId, alertType);
    });
  });

  if (location.hostname !== '192.168.4.1') {
    setConnected(true, 'Mock Mode');
    startMock();
  } else {
    connectWS();
  }
}

function connectWS() {
  if (ws) try { ws.close(); } catch (e) { }
  setConnected(false, 'Connecting…');
  ws = new WebSocket(WS_URL);
  
  ws.onopen = function () {
    setConnected(true, 'Connected');
    reconnectDelay = RECONNECT_BASE;
  };

  ws.onmessage = function (e) {
    try {
      const data = JSON.parse(e.data);
      handleMessage(data);
    } catch (err) {
      console.error('SENTINEL: bad JSON', err);
    }
  };

  ws.onclose = function () {
    setConnected(false, 'Disconnected');
    scheduleReconnect();
  };

  ws.onerror = function () {
    setConnected(false, 'Error');
    try { ws.close(); } catch (e) { }
  };
}

function scheduleReconnect() {
  clearTimeout(reconnectTimer);
  reconnectTimer = setTimeout(() => {
    reconnectDelay = Math.min(reconnectDelay * 2, RECONNECT_MAX);
    connectWS();
  }, reconnectDelay);
  dom.connText.textContent = 'Retry ' + (reconnectDelay / 1000).toFixed(1) + 's…';
}

function setConnected(ok, text) {
  dom.connDot.className = 'conn-dot ' + (ok ? 'connected' : 'disconnected');
  dom.connText.textContent = text;
}

function handleMessage(data) {
  if (data.commandResponse) {
    handleCommandResponse(data);
    return;
  }
  pendingData = data;
  if (!rafScheduled) {
    rafScheduled = true;
    requestAnimationFrame(() => {
      rafScheduled = false;
      if (pendingData) applyUpdate(pendingData);
    });
  }
}

function applyUpdate(data) {
  dom.uptime.textContent = formatUptime(data.uptime || 0);
  dom.packets.textContent = (data.packets || 0).toLocaleString();
  let alertCount = 0;
  let anyEmergency = false;

  if (data.workers) {
    for (let i = 0; i < data.workers.length; i++) {
      const w = data.workers[i];
      updateWorkerCard(w);
      if (w.active && w.statusCode >= 2) alertCount++;
      if (w.active && w.statusCode === 3) anyEmergency = true;
    }
  }

  dom.alerts.textContent = alertCount;
  if (alertCount > 0) dom.alertsIcon.classList.add('has-alerts');
  else dom.alertsIcon.classList.remove('has-alerts');

  if (anyEmergency) {
    dom.banner.classList.add('visible');
    if (soundEnabled && !alarmPlaying) startAlarm();
  } else {
    dom.banner.classList.remove('visible');
    if (alarmPlaying) stopAlarm();
  }

  updateShiftSummary();
}

function updateWorkerCard(w) {
  const id = w.id;
  const p = 'w' + id;
  const card = $(`worker-${id}`);
  if (!card) return;
  
  const state = workerState[id];
  if (!w.active) {
    card.classList.add('inactive');
    state.active = false;
    const ackBtn = card.querySelector('.ack-btn');
    if (ackBtn) ackBtn.style.display = 'none';
    return;
  }

  card.classList.remove('inactive');
  state.active = true;
  state.lastSeenCounter = 0;

  const sc = Math.min(w.statusCode, 3);
  for (let i = 0; i < CARD_CLASSES.length; i++)
    card.classList.remove(CARD_CLASSES[i]);
  card.classList.add(CARD_CLASSES[sc]);

  $(`${p}-zone`).textContent = w.zone || '—';
  const badge = $(`${p}-status-badge`);
  badge.textContent = w.status || STATUS_LABELS[sc];
  badge.className = 'status-badge ' + STATUS_CLASSES[sc];

  const alertEl = $(`${p}-alert`);
  const alertMsg = $(`${p}-alert-msg`);
  if (sc === 3 && w.alertMsg && w.alertMsg !== 'None') {
    alertMsg.textContent = w.alertMsg;
    alertEl.classList.add('visible');
  } else {
    alertEl.classList.remove('visible');
  }

  const fc = Math.min(w.fallStateCode || 0, 4);
  const fb = $(`${p}-fall-badge`);
  fb.textContent = w.fallState || FALL_LABELS[fc];
  fb.className = 'fall-badge ' + FALL_CLASSES[fc];

  const accel = w.accel || 1.0;
  const azN = $(`${p}-az-normal`);
  const azM = $(`${p}-az-moving`);
  const azI = $(`${p}-az-impact`);
  azN.className = 'accel-pill';
  azM.className = 'accel-pill';
  azI.className = 'accel-pill';
  if (accel > 2.5) azI.classList.add('active-impact');
  else if (accel > 1.2) azM.classList.add('active-moving');
  else azN.classList.add('active-normal');

  // Timeline Logic
  state.timeline.push(sc);
  if (state.timeline.length > TIMELINE_LEN) state.timeline.shift();
  renderTimeline(p, state.timeline);

  updateSensorSpark(p, 'temp', w.temp, '°C', state, (v) => v >= 40 ? 'emergency' : v >= 35 ? 'warn' : '');
  updateSensorSpark(p, 'hum', w.hum, '%', state, (v) => v >= 80 ? 'warn' : '');
  updateSensorSpark(p, 'sound', w.soundDB, ' dB', state, (v) => v >= 90 ? 'emergency' : v >= 80 ? 'danger' : v >= 70 ? 'warn' : '');
  updateSensorSpark(p, 'hr', w.hr, ' BPM', state, (v) => v === 0 ? '' : (v < 50 || v > 150) ? 'emergency' : (v < 60 || v > 120) ? 'warn' : 'good');
  updateSensorSpark(p, 'spo2', w.spo2, '%', state, (v) => v === 0 ? '' : v < 90 ? 'emergency' : v < 94 ? 'warn' : 'good');

  const gasCell = $(`${p}-gas-cell`);
  const gasVal = $(`${p}-gas`);
  gasCell.classList.remove('gas-detected', 'emergency', 'good');
  if (w.gas) {
    gasVal.textContent = 'DETECTED';
    gasCell.classList.add('gas-detected', 'emergency');
  } else {
    gasVal.textContent = 'Clear';
    gasCell.classList.add('good');
  }

  updateGauge(`${p}-noise-fill`, `${p}-noise-pct`, w.noiseDose || 0, 'noise');
  updateGauge(`${p}-heat-fill`, `${p}-heat-pct`, w.heatDose || 0, 'heat');

  const bat = Math.min(100, Math.max(0, w.battery || 0)); 
  const bf = $(`${p}-bat-fill`);
  bf.style.width = bat + '%';
  bf.className = 'battery-fill' + (bat <= 15 ? ' low' : bat <= 35 ? ' mid' : '');
  $(`${p}-bat-pct`).textContent = bat + '%';
  $(`${p}-last-seen`).textContent = 'Last seen: now';

  // ── Dynamic ACK Button: Show only in Emergency ──────────
  const actions = card.querySelector('.card-actions');
  let ackBtn = card.querySelector('.ack-btn');

  if (sc === 3 && !ackBtn) {
    ackBtn = document.createElement('button');
    // Apply the new animated emergency class
    ackBtn.className = 'ack-btn ack-btn-emergency'; 
    
    ackBtn.innerHTML = '⚠ ACKNOWLEDGE DANGER';
    // Remove inline styles that were causing the green/red clash
    ackBtn.style.background = ''; 
    ackBtn.style.color = '';
    
    ackBtn.onclick = () => sendManualAck(id);
    actions.appendChild(ackBtn);
  } else if (sc !== 3 && ackBtn) {
    ackBtn.remove();
  }

  if (state.prevStatusCode !== sc) {
    if (state.prevStatusCode >= 0) logEvent(id, sc, w);
    if (sc === 3) state.shiftStats.incidents++;
    state.prevStatusCode = sc;
  }

  if (sc === 0) state.shiftStats.safe++;
  else if (sc === 1) state.shiftStats.warn++;
  else if (sc === 2) state.shiftStats.danger++;
  else state.shiftStats.emergency++;

  if (w.temp > state.shiftStats.peakTemp) state.shiftStats.peakTemp = w.temp;
  if (w.soundDB > state.shiftStats.peakSound) state.shiftStats.peakSound = w.soundDB;
  if (w.hr > state.shiftStats.peakHR) state.shiftStats.peakHR = w.hr;
}

function updateSensorSpark(prefix, key, value, suffix, state, thresholdFn) {
  const valId = prefix + '-' + key;
  const cellId = prefix + '-' + key + '-cell';
  const sparkId = prefix + '-spark-' + key;
  const el = $(valId);
  const cell = $(cellId);
  if (value === undefined || value === null || isNaN(value)) {
    el.textContent = '—';
    return;
  }

  const displayVal = parseFloat(value).toFixed(1);
  el.textContent = displayVal + (suffix || '');

  cell.classList.remove('good', 'warn', 'danger', 'emergency');
  const cls = thresholdFn(parseFloat(value));
  if (cls) cell.classList.add(cls);

  if (SPARK_COLORS[key] && value > 0) {
    const spark = state.sparklines[key];
    spark.push(parseFloat(value));
    if (spark.length > SPARKLINE_LEN) spark.shift();
    renderSparkline(sparkId, spark, key);
  }
}

function renderSparkline(containerId, data, key) {
  const container = $(containerId);
  if (!container || data.length < 2) return;
  const range = SPARK_RANGES[key] || { min: 0, max: 100 };
  const colors = SPARK_COLORS[key] || { line: '#888', area: '#888' };
  const w = 100, h = 20;
  let points = [];

  for (let i = 0 ; i < data.length; i++) {
    const x = (i / (SPARKLINE_LEN - 1)) * w;
    const v = Math.max(range.min, Math.min(range.max, data[i]));
    const y = h - ((v - range.min) / (range.max - range.min)) * h;
    points.push(x.toFixed(1) + ',' + y.toFixed(1));
  }

  const polyline = points.join(' ');
  const area = points.join(' ') + ' ' + w + ',' + h + ' 0,' + h;
  container.innerHTML = `<svg width="${w}" height="${h}" viewBox="0 0 ${w} ${h}">
    <polyline fill="none" stroke="${colors.line}" stroke-width="1.5" points="${polyline}"/>
    <polygon fill="${colors.area}" opacity="0.08" points="${area}"/>
  </svg>`;
}

function updateGauge(fillId, pctId, value, type) {
  var pct = Math.min(100, Math.max(0, value));
  var fill = $(fillId);
  var text = $(pctId);
  if (!fill) return;

  // 1. Calculate the length of the colored arc based on percentage
  // Max length is GAUGE_CIRCUM (75 units, which is 270 degrees)
  var length = (pct / 100) * GAUGE_CIRCUM;
  
  // 2. Calculate the remaining transparent part
  // IMPORTANT: length + unfilled must equal 100 (the pathLength) to prevent repeating patterns
  var unfilled = 100 - length;

  // 3. Set the dash array
  fill.setAttribute('stroke-dasharray', length.toFixed(1) + ' ' + unfilled.toFixed(1));

  // 4. Set the offset to rotate the gauge
  // We want the gauge to start at ~7:30 position (bottom-left)
  // This corresponds to an offset of GAUGE_CIRCUM / 2 (37.5)
  fill.setAttribute('stroke-dashoffset', (GAUGE_CIRCUM / 2).toFixed(1));

  // Color logic
  fill.classList.remove('critical');
  if (pct >= 80) fill.classList.add('critical');

  text.textContent = Math.round(pct) + '%';
}

function logEvent(workerId, statusCode, w) {
  const now = new Date();
  const time = pad(now.getHours()) + ':' + pad(now.getMinutes()) + ':' + pad(now.getSeconds());
  let msg = 'W' + workerId + ' ';
  let dotClass = STATUS_CLASSES[statusCode] || 'info';
  
  if (statusCode === 3) {
    msg += '⚠ ' + ((w.alertMsg && w.alertMsg !== 'None') ? w.alertMsg : 'EMERGENCY') + ' — ' + (w.zone || '');
  } else if (statusCode === 2) {
    msg += 'Entered DANGER state — ' + (w.zone || '');
  } else if (statusCode === 1) {
    msg += 'Warning — approaching limits';
  } else {
    msg += '✓ Returned to SAFE';
    dotClass = 'safe';
  }

  events.unshift({ time, msg, cls: dotClass });
  if (events.length > MAX_EVENTS) events.pop();
  renderEvents();
}

function addSystemEvent(msg, cls) {
  const now = new Date();
  const time = pad(now.getHours()) + ':' + pad(now.getMinutes()) + ':' + pad(now.getSeconds());
  events.unshift({ time, msg, cls: cls || 'info' });
  if (events.length > MAX_EVENTS) events.pop();
  renderEvents();
}

function renderEvents() {
  const list = dom.eventList;
  if (events.length === 0) {
    list.innerHTML = '<div class="event-empty">No events yet — waiting for data…</div>';
    dom.eventCount.textContent = '0 events';
    return;
  }
  let html = '';
  for (let i = 0; i < events.length; i++) {
    const e = events[i];
    html += `<div class="event-item">
      <span class="event-time">${e.time}</span>
      <span class="event-dot ${e.cls}"></span>
      <span class="event-msg">${e.msg}</span>
    </div>`;
  }
  list.innerHTML = html;
  dom.eventCount.textContent = events.length + ' event' + (events.length > 1 ? 's' : '');
}

function updateShiftSummary() {
  let html = '';
  for (let id = 1; id <= 2; id++) {
    const s = workerState[id];
    if (!s.active && s.shiftStats.safe === 0) continue;
    
    const stats = s.shiftStats;
    const total = stats.safe + stats.warn + stats.danger + stats.emergency;
    if (total === 0) continue;

    html += `<div class="summary-worker-label">Worker ${id}</div>`;
    
    // Visual Summary Line (Time Distribution)
    html += `<div class="status-time-bar">
      <div class="stb-seg stb-safe" style="flex:${stats.safe}"></div>
      <div class="stb-seg stb-warn" style="flex:${stats.warn}"></div>
      <div class="stb-seg stb-danger" style="flex:${stats.danger}"></div>
      <div class="stb-seg stb-emergency" style="flex:${stats.emergency}"></div>
    </div>`;
    
    html += summaryRow('Safe Time', Math.round(stats.safe / total * 100) + '%');
    html += summaryRow('Incidents', stats.incidents);
    html += summaryRow('Peak Temp', stats.peakTemp > 0 ? stats.peakTemp.toFixed(1) + '°C' : '—');
    html += summaryRow('Peak Sound', stats.peakSound > 0 ? stats.peakSound.toFixed(1) + ' dB' : '—');
    html += summaryRow('Peak HR', stats.peakHR > 0 ? stats.peakHR + ' BPM' : '—');
  }
  dom.summaryGrid.innerHTML = html || '<div class="event-empty">Waiting for worker data…</div>';
}

function summaryRow(key, val) {
  return `<div class="summary-row"><span class="summary-key">${key}</span><span class="summary-val">${val}</span></div>`;
}

function updateShiftDuration() {
  const elapsed = Math.floor((Date.now() - shiftStartTime) / 1000);
  dom.shiftDur.textContent = formatUptime(elapsed);
}

function initTimelines() {
  for (let id = 1; id <= 2; id++) {
    const container = $(`w${id}-timeline`);
    if (!container) continue;
    container.innerHTML = '';
    for (let i = 0; i < TIMELINE_LEN; i++) {
      const seg = document.createElement('div');
      seg.className = 'timeline-seg';
      container.appendChild(seg);
    }
  }
}

function renderTimeline(prefix, history) {
  const container = $(prefix + '-timeline');
  if (!container) return;
  const segs = container.children;
  for (let i = 0; i < TIMELINE_LEN; i++) {
    segs[i].className = 'timeline-seg';
    if (i < history.length) {
      segs[i].classList.add('s-' + STATUS_CLASSES[history[i]]);
    }
  }
}

function tickLastSeen() {
  for (let id = 1; id <= 2; id++) {
    const s = workerState[id];
    if (!s.active) continue;
    s.lastSeenCounter++;
    const el = $(`w${id}-last-seen`);
    el.textContent = 'Last seen: ' + s.lastSeenCounter + 's ago';
    el.className = '';
    if (s.lastSeenCounter >= 10) el.classList.add('last-seen-danger');
    else if (s.lastSeenCounter >= 5) el.classList.add('last-seen-warn');
  }
}

function startAlarm() {
  if (alarmPlaying) return;
  alarmPlaying = true;
  try {
    if (!audioCtx) audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    alarmOsc = audioCtx.createOscillator();
    alarmGain = audioCtx.createGain();
    alarmOsc.type = 'square';
    alarmOsc.frequency.setValueAtTime(800, audioCtx.currentTime);
    alarmGain.gain.setValueAtTime(0, audioCtx.currentTime);
    alarmOsc.connect(alarmGain);
    alarmGain.connect(audioCtx.destination);
    alarmOsc.start();
    pulseAlarmGain();
  } catch (e) {
    console.error('SENTINEL: Audio error', e);
    alarmPlaying = false;
  }
}

function pulseAlarmGain() {
  clearInterval(pulseInterval);
  let on = true;
  pulseInterval = setInterval(() => {
    if (!alarmGain) return;
    alarmGain.gain.setValueAtTime(on ? 0.3 : 0, audioCtx.currentTime);
    on = !on;
  }, 400);
}

function stopAlarm() {
  alarmPlaying = false;
  clearInterval(pulseInterval);
  try {
    if (alarmOsc) { alarmOsc.stop(); alarmOsc.disconnect(); alarmOsc = null; }
    if (alarmGain) { alarmGain.disconnect(); alarmGain = null; }
  } catch (e) { }
}

function toggleTheme() {
  darkTheme = !darkTheme;
  localStorage.setItem('sentinel-theme', darkTheme ? 'dark' : 'light');
  applyTheme();
}

function applyTheme() {
  if (darkTheme) {
    document.body.classList.remove('light-theme');
    dom.themeBtn.textContent = '🌙';
  } else {
    document.body.classList.add('light-theme');
    dom.themeBtn.textContent = '☀';
  }
}

function toggleSound() {
  soundEnabled = !soundEnabled;
  localStorage.setItem('sentinel-sound', soundEnabled ? 'true' : 'false');
  applySoundState();
  if (!soundEnabled && alarmPlaying) stopAlarm();
}

function applySoundState() {
  dom.soundBtn.textContent = soundEnabled ? '🔔' : '🔕';
  if (soundEnabled) dom.soundBtn.classList.add('active');
  else dom.soundBtn.classList.remove('active');
}

function formatUptime(s) {
  return pad(Math.floor(s / 3600)) + ':' + pad(Math.floor(s % 3600 / 60)) + ':' + pad(s % 60);
}

function sendInlineAlert(workerId, alertType) {
  if (!ws || ws.readyState !== WebSocket.OPEN) {
    alert('Not connected to Hub');
    return;
  }
  const cmd = {
    command: 'sendAlert',
    workerId: workerId,
    alertType: alertType,
    priority: 2,
    message: ''
  };
  ws.send(JSON.stringify(cmd));
  addSystemEvent('Admin sent alert to W' + workerId, 'info');
}

function sendManualAck(workerId) {
  if (!ws || ws.readyState !== WebSocket.OPEN) {
    alert('Not connected to Hub');
    return;
  }
  const cmd = {
    command: 'manualAck',
    workerId: workerId
  };
  ws.send(JSON.stringify(cmd));
  addSystemEvent('Manual ACK sent to W' + workerId, 'info');
}

function handleCommandResponse(data) {
  if (data.commandResponse && data.commandResponse.status === 'sent') {
    console.log('Command delivered to W' + data.commandResponse.workerId);
    const card = $(`worker-${data.commandResponse.workerId}`);
    const btn = card.querySelector('.ack-btn');
    if (btn) {
      btn.textContent = '✅ Acknowledged';
      btn.style.background = '#10b981';
      setTimeout(() => {
        btn.textContent = '⚠ Acknowledge Danger';
        btn.style.background = '#10b981';
      }, 1500);
    }
  } else if (data.commandResponse) {
    console.error('Command failed for W' + data.commandResponse.workerId);
    addSystemEvent('Failed to send command to W' + data.commandResponse.workerId, 'danger');
  }
}

function pad(n) {
  return n < 10 ? '0' + n : '' + n;
}

document.addEventListener('DOMContentLoaded', init);
})();