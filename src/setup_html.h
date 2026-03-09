/*
 * setup_html.h
 *
 * Captive-portal provisioning page embedded as a PROGMEM string.
 * Served during STATE_PROVISIONING on HTTP port 80.
 *
 * Features:
 *  - Auto-scans for nearby WiFi networks on load
 *  - Clickable network list with RSSI signal bars and lock icon
 *  - Manual SSID entry fallback
 *  - Password field with show/hide toggle
 *  - POST /save → credentials stored → device reboots and connects
 */

// clang-format off
static const char SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<title>ESP32 HID &mdash; WiFi Setup</title>
<style>
:root{
  --bg:#1a1a2e;--surface:#16213e;--primary:#0f3460;--accent:#e94560;
  --text:#eaeaea;--border:#3d3d5c;--ok:#4caf50;--err:#f44336;--warn:#ff9800;
}
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent;}
body{background:var(--bg);color:var(--text);font-family:'Segoe UI',system-ui,sans-serif;
  font-size:15px;min-height:100vh;display:flex;flex-direction:column;align-items:center;
  padding:20px 12px;}
.card{background:var(--surface);border:1px solid var(--border);border-radius:12px;
  padding:24px 20px;width:100%;max-width:420px;}
h1{color:var(--accent);font-size:1.3em;margin-bottom:4px;}
.sub{color:#888;font-size:12px;margin-bottom:20px;}
label{display:block;font-size:12px;color:#aaa;margin-bottom:4px;margin-top:14px;}
input[type=text],input[type=password]{
  width:100%;padding:10px 12px;background:var(--bg);
  border:1px solid var(--border);border-radius:6px;color:var(--text);
  font-size:14px;outline:none;}
input:focus{border-color:var(--accent);}
.pw-wrap{position:relative;}
.pw-wrap input{padding-right:44px;}
.eye{position:absolute;right:10px;top:50%;transform:translateY(-50%);
  background:none;border:none;color:#888;cursor:pointer;font-size:18px;padding:4px;}
button.act{margin-top:18px;width:100%;padding:12px;background:var(--accent);
  border:none;border-radius:8px;color:#fff;font-size:15px;font-weight:600;cursor:pointer;}
button.act:disabled{opacity:.5;cursor:not-allowed;}
button.scan-btn{margin-bottom:12px;padding:7px 14px;background:var(--primary);
  border:none;border-radius:6px;color:var(--text);font-size:13px;cursor:pointer;}
#netlist{max-height:220px;overflow-y:auto;margin-bottom:6px;}
.net{display:flex;align-items:center;gap:10px;padding:9px 10px;
  border-radius:6px;cursor:pointer;transition:background .15s;}
.net:hover,.net.sel{background:var(--primary);}
.net.sel{border:1px solid var(--accent);}
.ssid-name{flex:1;font-size:14px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;}
.lock{font-size:11px;color:#aaa;}
.bars{display:flex;align-items:flex-end;gap:1px;height:14px;}
.bar{width:3px;background:var(--border);border-radius:1px;}
.bar.on{background:var(--ok);}
#status{margin-top:14px;padding:10px 12px;border-radius:6px;font-size:13px;
  display:none;text-align:center;}
#status.ok{background:rgba(76,175,80,.18);color:var(--ok);display:block;}
#status.err{background:rgba(244,67,54,.18);color:var(--err);display:block;}
#status.warn{background:rgba(255,152,0,.18);color:var(--warn);display:block;}
.spinner{display:inline-block;width:14px;height:14px;border:2px solid #555;
  border-top-color:var(--accent);border-radius:50%;animation:spin .7s linear infinite;
  vertical-align:middle;margin-right:6px;}
@keyframes spin{to{transform:rotate(360deg);}}
</style>
</head>
<body>
<div class="card">
  <h1>&#x1F4F6; WiFi Setup</h1>
  <p class="sub">Connect to your home network so the HID controller can be reached over WiFi.</p>

  <button class="scan-btn" onclick="doScan()">&#x1F50D; Scan for networks</button>
  <div id="netlist"></div>

  <label for="ssid">Network name (SSID)</label>
  <input type="text" id="ssid" placeholder="Enter SSID&hellip;" autocomplete="off" spellcheck="false">

  <label for="pass">Password</label>
  <div class="pw-wrap">
    <input type="password" id="pass" placeholder="Leave blank if open network"
           autocomplete="new-password" spellcheck="false">
    <button class="eye" onclick="togglePw()" title="Show / hide password">&#x1F441;</button>
  </div>

  <button class="act" id="saveBtn" onclick="doSave()">Save &amp; Connect</button>
  <div id="status"></div>
</div>

<script>
const ssidEl   = document.getElementById('ssid');
const passEl   = document.getElementById('pass');
const saveBtn  = document.getElementById('saveBtn');
const statusEl = document.getElementById('status');

function setStatus(msg, cls) {
  statusEl.textContent = msg;
  statusEl.className   = cls;
}

// Signal bars from RSSI
function bars(rssi) {
  // 4 bars: >-55 all, >-65 3, >-75 2, else 1
  const n = rssi > -55 ? 4 : rssi > -65 ? 3 : rssi > -75 ? 2 : 1;
  let h = '<div class="bars">';
  for (let i = 1; i <= 4; i++)
    h += `<div class="bar${i <= n ? ' on' : ''}" style="height:${i*3+2}px"></div>`;
  return h + '</div>';
}

function doScan() {
  const nl = document.getElementById('netlist');
  nl.innerHTML = '<div style="padding:10px;color:#888;font-size:13px">'
               + '<span class="spinner"></span>Scanning&hellip;</div>';
  fetch('/scan')
    .then(r => r.json())
    .then(nets => {
      if (!nets.length) { nl.innerHTML = '<div style="padding:8px;color:#888;font-size:13px">No networks found.</div>'; return; }
      // dedupe by SSID, keep strongest
      const seen = {};
      nets.forEach(n => { if (!seen[n.ssid] || seen[n.ssid].rssi < n.rssi) seen[n.ssid] = n; });
      const uniq = Object.values(seen).sort((a,b) => b.rssi - a.rssi);
      nl.innerHTML = uniq.map(n =>
        `<div class="net" onclick="selectNet(this,'${escAttr(n.ssid)}')">
           ${bars(n.rssi)}
           <span class="ssid-name">${escHtml(n.ssid)}</span>
           <span class="lock">${n.secure ? '&#x1F512;' : ''}</span>
         </div>`
      ).join('');
    })
    .catch(() => { nl.innerHTML = '<div style="padding:8px;color:var(--err);font-size:13px">Scan failed. Try again.</div>'; });
}

function selectNet(el, ssid) {
  document.querySelectorAll('.net').forEach(n => n.classList.remove('sel'));
  el.classList.add('sel');
  ssidEl.value = ssid;
  passEl.focus();
}

function escHtml(s) {
  return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}
function escAttr(s) {
  return s.replace(/\\/g, '\\\\').replace(/'/g, "\\'").replace(/"/g, '&quot;');
}

function togglePw() {
  passEl.type = passEl.type === 'password' ? 'text' : 'password';
}

function doSave() {
  const ssid = ssidEl.value.trim();
  if (!ssid) { setStatus('Please enter or select a network name.', 'err'); return; }
  saveBtn.disabled = true;
  setStatus('\u23F3 Saving\u2026 the device will reboot and attempt to connect.', 'warn');
  const fd = new FormData();
  fd.append('ssid', ssid);
  fd.append('pass', passEl.value);
  fetch('/save', {method:'POST', body:fd})
    .then(r => r.text())
    .then(t => setStatus('\u2714 ' + t, 'ok'))
    .catch(() => { setStatus('Error saving. Please try again.', 'err'); saveBtn.disabled = false; });
}

// Auto-scan on load
doScan();
</script>
</body>
</html>
)rawliteral";
// clang-format on
