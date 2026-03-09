/*
 * index_html.h
 *
 * Full single-page web interface embedded as a PROGMEM string.
 * Served by the HTTP server on port 80.
 *
 * Features:
 *  - On-screen keyboard  : full QWERTY layout, F-keys, modifiers, nav keys
 *  - Touchpad            : pointer-events relative mouse movement
 *                          tap = left click, long-press = right click
 *  - Mouse buttons       : left / middle / right (hold supported)
 *  - Scroll wheel        : hold-to-scroll up/down buttons
 *  - Sensitivity slider  : adjust pointer speed
 *  - Physical keyboard   : intercept browser keydown/keyup → HID
 *  - Physical mouse lock : Pointer Lock API → raw mouse movement → HID
 *  - Text send           : type a string and forward it as HID keystrokes
 *  - WebSocket reconnect : automatic 2 s retry on disconnect
 */

// clang-format off
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<title>ESP32 HID Controller</title>
<style>
:root {
  --bg:#1a1a2e; --surface:#16213e; --primary:#0f3460; --accent:#e94560;
  --text:#eaeaea; --key-bg:#2d2d44; --key-act:#e94560; --key-mod:#0f3460;
  --border:#3d3d5c; --ok:#4caf50; --err:#f44336; --warn:#ff9800;
}
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent;}
body{background:var(--bg);color:var(--text);font-family:'Segoe UI',system-ui,sans-serif;
  font-size:14px;min-height:100vh;}
#app{max-width:960px;margin:0 auto;padding:10px;}
header{display:flex;justify-content:space-between;align-items:center;
  padding:10px 0;border-bottom:1px solid var(--border);margin-bottom:12px;}
h1{font-size:1.2em;color:var(--accent);}
#cs{padding:3px 10px;border-radius:10px;font-size:12px;font-weight:600;}
.ok{background:var(--ok);color:#fff;}
.err{background:var(--err);color:#fff;}
.warn{background:var(--warn);color:#000;}
/* tabs */
.tabs{display:flex;gap:4px;margin-bottom:12px;}
.tb{padding:7px 20px;border:none;border-radius:6px 6px 0 0;cursor:pointer;
  background:var(--surface);color:var(--text);font-size:13px;transition:background .2s;}
.tb.on{background:var(--primary);color:var(--accent);}
.panel{display:none;} .panel.on{display:block;}
/* helpers */
btn-row{display:flex;gap:8px;margin-bottom:10px;}
.row{display:flex;gap:8px;margin-bottom:10px;align-items:center;flex-wrap:wrap;}
input[type=text]{flex:1;padding:7px 10px;background:var(--surface);
  border:1px solid var(--border);border-radius:6px;color:var(--text);font-size:13px;}
button.act{padding:7px 14px;background:var(--accent);border:none;border-radius:6px;
  color:#fff;cursor:pointer;white-space:nowrap;}
button.sec{padding:7px 14px;background:var(--primary);border:none;border-radius:6px;
  color:var(--text);cursor:pointer;white-space:nowrap;}
.toggle-label{display:flex;align-items:center;gap:6px;cursor:pointer;user-select:none;}
/* keyboard */
.kb-wrap{overflow-x:auto;padding-bottom:4px;}
.kb{display:inline-flex;flex-direction:column;gap:3px;padding:10px;
  background:var(--surface);border-radius:8px;min-width:720px;}
.kr{display:flex;gap:3px;}
.k{height:38px;min-width:38px;padding:0 5px;background:var(--key-bg);
  border:1px solid var(--border);border-radius:4px;color:var(--text);font-size:11px;
  cursor:pointer;display:flex;align-items:center;justify-content:center;
  white-space:nowrap;user-select:none;touch-action:none;
  transition:background .1s,transform .05s;}
.k:active,.k.pr{background:var(--key-act);transform:scale(.95);}
.k.mo{background:var(--key-mod);}
.k.mo.on{background:var(--accent);}
.gap{min-width:14px;visibility:hidden;}
/* key widths */
.w-esc{min-width:50px;} .w-bs{min-width:78px;} .w-tab{min-width:62px;}
.w-bsl{min-width:58px;} .w-caps{min-width:72px;} .w-ent{min-width:86px;}
.w-lsh{min-width:92px;} .w-rsh{min-width:92px;} .w-spc{min-width:290px;}
.w-ctrl{min-width:52px;} .w-alt{min-width:52px;} .w-gui{min-width:52px;}
/* mouse panel */
.mp{display:flex;flex-direction:column;gap:12px;}
.sens-row{display:flex;align-items:center;gap:10px;}
.sens-row input[type=range]{flex:1;}
#tp{width:100%;height:260px;background:var(--surface);border:1px solid var(--border);
  border-radius:8px;cursor:crosshair;touch-action:none;user-select:none;
  display:flex;align-items:center;justify-content:center;
  color:var(--border);font-size:12px;position:relative;}
#tp.active-lock{border-color:var(--accent);cursor:none;}
.mbtns{display:flex;gap:8px;justify-content:center;}
.mb{flex:1;max-width:160px;height:46px;background:var(--key-bg);
  border:1px solid var(--border);border-radius:6px;color:var(--text);
  font-size:13px;cursor:pointer;user-select:none;touch-action:none;
  transition:background .1s;}
.mb.pr{background:var(--key-act);}
.scr{display:flex;gap:8px;justify-content:center;}
.sb{padding:8px 22px;background:var(--key-bg);border:1px solid var(--border);
  border-radius:6px;color:var(--text);cursor:pointer;touch-action:none;user-select:none;}
.sb.pr{background:var(--key-act);}
/* settings panel */
.wifi-info-card{background:var(--surface);border:1px solid var(--border);
  border-radius:8px;padding:14px 16px;margin-bottom:14px;}
.wi-row{display:flex;justify-content:space-between;align-items:center;padding:6px 0;
  border-bottom:1px solid var(--border);}
.wi-row:last-child{border-bottom:none;}
.wi-lbl{font-size:12px;color:#aaa;}
.wi-val{font-size:14px;font-weight:600;}
.wi-hint{font-size:13px;color:#aaa;line-height:1.5;margin-bottom:16px;}
.wi-reset{background:var(--err);width:100%;padding:12px;margin-bottom:10px;}
#wi-status{font-size:13px;color:var(--warn);min-height:18px;}
</style>
</head>
<body>
<div id="app">
<header>
  <h1>&#x1F5A5; ESP32 HID Controller</h1>
  <span id="cs" class="warn">Connecting&#8230;</span>
</header>

<div class="tabs">
  <button class="tb on"  onclick="showTab('kb',this)">&#x2328; Keyboard</button>
  <button class="tb"     onclick="showTab('ms',this)">&#x1F5B1; Mouse</button>
  <button class="tb"     onclick="showTab('wifi',this)">&#x2699; Settings</button>
</div>

<!-- ═══ KEYBOARD PANEL ═══════════════════════════════════════════════════════ -->
<div id="panel-kb" class="panel on">

  <div class="row">
    <input type="text" id="ti" placeholder="Type text to send&#8230;"
           autocomplete="off" autocorrect="off" spellcheck="false"
           onkeydown="if(event.key==='Enter'){event.preventDefault();doSendText();}">
    <button class="act" onclick="doSendText()">Send</button>
    <button class="sec" onclick="tapKey(176)">&#x23CE; Enter</button>
  </div>

  <div class="row">
    <label class="toggle-label">
      <input type="checkbox" id="phys-kbd" onchange="toggleKbdCapture(this.checked)">
      Capture physical keyboard
    </label>
    <label class="toggle-label" style="margin-left:16px;">
      <input type="checkbox" id="phys-ms" onchange="toggleMouseCapture(this.checked)">
      Capture physical mouse (requires pointer lock)
    </label>
  </div>

  <div class="kb-wrap">
  <div class="kb">

    <!-- Row 0 : Esc + F1-F12 + sys -->
    <div class="kr">
      <button class="k w-esc" data-h="177">Esc</button>
      <span class="k gap"></span>
      <button class="k" data-h="194">F1</button>
      <button class="k" data-h="195">F2</button>
      <button class="k" data-h="196">F3</button>
      <button class="k" data-h="197">F4</button>
      <span class="k gap"></span>
      <button class="k" data-h="198">F5</button>
      <button class="k" data-h="199">F6</button>
      <button class="k" data-h="200">F7</button>
      <button class="k" data-h="201">F8</button>
      <span class="k gap"></span>
      <button class="k" data-h="202">F9</button>
      <button class="k" data-h="203">F10</button>
      <button class="k" data-h="204">F11</button>
      <button class="k" data-h="205">F12</button>
      <span class="k gap"></span>
      <button class="k" data-h="206">PrtSc</button>
      <button class="k" data-h="207">ScrLk</button>
      <button class="k" data-h="208">Pause</button>
    </div>

    <!-- Row 1 : number row -->
    <div class="kr">
      <button class="k" data-h="96">`</button>
      <button class="k" data-h="49">1</button>
      <button class="k" data-h="50">2</button>
      <button class="k" data-h="51">3</button>
      <button class="k" data-h="52">4</button>
      <button class="k" data-h="53">5</button>
      <button class="k" data-h="54">6</button>
      <button class="k" data-h="55">7</button>
      <button class="k" data-h="56">8</button>
      <button class="k" data-h="57">9</button>
      <button class="k" data-h="48">0</button>
      <button class="k" data-h="45">-</button>
      <button class="k" data-h="61">=</button>
      <button class="k w-bs" data-h="178">&#x232B; Bksp</button>
    </div>

    <!-- Row 2 : QWERTY -->
    <div class="kr">
      <button class="k w-tab" data-h="179">Tab</button>
      <button class="k" data-h="113">Q</button>
      <button class="k" data-h="119">W</button>
      <button class="k" data-h="101">E</button>
      <button class="k" data-h="114">R</button>
      <button class="k" data-h="116">T</button>
      <button class="k" data-h="121">Y</button>
      <button class="k" data-h="117">U</button>
      <button class="k" data-h="105">I</button>
      <button class="k" data-h="111">O</button>
      <button class="k" data-h="112">P</button>
      <button class="k" data-h="91">[</button>
      <button class="k" data-h="93">]</button>
      <button class="k w-bsl" data-h="92">\</button>
    </div>

    <!-- Row 3 : ASDF (CapsLock is a toggle modifier) -->
    <div class="kr">
      <button class="k w-caps mo" data-h="193" data-mod="1">Caps</button>
      <button class="k" data-h="97">A</button>
      <button class="k" data-h="115">S</button>
      <button class="k" data-h="100">D</button>
      <button class="k" data-h="102">F</button>
      <button class="k" data-h="103">G</button>
      <button class="k" data-h="104">H</button>
      <button class="k" data-h="106">J</button>
      <button class="k" data-h="107">K</button>
      <button class="k" data-h="108">L</button>
      <button class="k" data-h="59">;</button>
      <button class="k" data-h="39">'</button>
      <button class="k w-ent" data-h="176">Enter &#x23CE;</button>
    </div>

    <!-- Row 4 : ZXCV (Shift = toggle modifier) -->
    <div class="kr">
      <button class="k w-lsh mo" data-h="129" data-mod="1">&#x21E7; Shift</button>
      <button class="k" data-h="122">Z</button>
      <button class="k" data-h="120">X</button>
      <button class="k" data-h="99">C</button>
      <button class="k" data-h="118">V</button>
      <button class="k" data-h="98">B</button>
      <button class="k" data-h="110">N</button>
      <button class="k" data-h="109">M</button>
      <button class="k" data-h="44">,</button>
      <button class="k" data-h="46">.</button>
      <button class="k" data-h="47">/</button>
      <button class="k w-rsh mo" data-h="133" data-mod="1">&#x21E7; Shift</button>
    </div>

    <!-- Row 5 : bottom row + arrow cluster -->
    <div class="kr">
      <button class="k w-ctrl mo" data-h="128" data-mod="1">Ctrl</button>
      <button class="k w-gui  mo" data-h="131" data-mod="1">&#x2756; Win</button>
      <button class="k w-alt  mo" data-h="130" data-mod="1">Alt</button>
      <button class="k w-spc"    data-h="32">Space</button>
      <button class="k w-alt  mo" data-h="134" data-mod="1">AltGr</button>
      <button class="k w-gui  mo" data-h="135" data-mod="1">&#x2756; Win</button>
      <button class="k w-ctrl mo" data-h="132" data-mod="1">Ctrl</button>
      <button class="k"          data-h="216">&#x25C0;</button>
      <button class="k"          data-h="218">&#x25B2;</button>
      <button class="k"          data-h="217">&#x25BC;</button>
      <button class="k"          data-h="215">&#x25B6;</button>
    </div>

    <!-- Row 6 : navigation block -->
    <div class="kr">
      <button class="k" data-h="209">Ins</button>
      <button class="k" data-h="210">Home</button>
      <button class="k" data-h="211">PgUp</button>
      <button class="k" data-h="212">Del</button>
      <button class="k" data-h="213">End</button>
      <button class="k" data-h="214">PgDn</button>
    </div>

  </div><!-- .kb -->
  </div><!-- .kb-wrap -->
</div><!-- #panel-kb -->

<!-- ═══ MOUSE PANEL ══════════════════════════════════════════════════════════ -->
<div id="panel-ms" class="panel">
  <div class="mp">

    <div class="sens-row">
      <span>Sensitivity: <b id="sv">1.0</b>x</span>
      <input type="range" id="sens" min="0.2" max="6" step="0.1" value="1.0"
             oninput="document.getElementById('sv').textContent=parseFloat(this.value).toFixed(1)">
    </div>

    <div id="tp">Move pointer here<br><small>(tap&#160;=&#160;left&#160;click&nbsp;&nbsp;long-press&#160;=&#160;right&#160;click)</small></div>

    <div class="mbtns">
      <button class="mb" data-btn="1">Left Click</button>
      <button class="mb" data-btn="4">Middle</button>
      <button class="mb" data-btn="2">Right Click</button>
    </div>

    <div class="scr">
      <button class="sb" id="su">&#x25B2; Scroll Up</button>
      <button class="sb" id="sd">&#x25BC; Scroll Down</button>
    </div>

  </div>
</div>

<!-- ═══ SETTINGS PANEL ═══════════════════════════════════════════════════════ -->
<div id="panel-wifi" class="panel">
  <div class="wifi-info-card">
    <div class="wi-row"><span class="wi-lbl">Network</span><span id="wi-ssid" class="wi-val">&#x2014;</span></div>
    <div class="wi-row"><span class="wi-lbl">IP address</span><span id="wi-ip" class="wi-val">&#x2014;</span></div>
  </div>
  <p class="wi-hint">To connect to a different network, reset WiFi credentials below.
     The device will reboot into setup mode and create the <b>ESP32-HID-Setup</b> access
     point so you can enter new credentials from your mobile.</p>
  <button class="act wi-reset" id="wifiResetBtn" onclick="doWifiReset()">
    &#x1F504; Reset WiFi &amp; Re-pair
  </button>
  <div id="wi-status"></div>
</div>

</div><!-- #app -->

<script>
// ── WebSocket ─────────────────────────────────────────────────────────────────
let ws = null;
const WS_URL = 'ws://' + location.hostname + ':81';
const cs = document.getElementById('cs');

function connect() {
  cs.textContent = 'Connecting\u2026'; cs.className = 'warn';
  ws = new WebSocket(WS_URL);
  ws.onopen    = () => { cs.textContent = '\u25CF Connected';    cs.className = 'ok';  };
  ws.onclose   = () => { cs.textContent = '\u25CB Disconnected'; cs.className = 'err';
                         setTimeout(connect, 2000); };
  ws.onerror   = () => ws.close();
}
function send(o) { if (ws && ws.readyState === 1) ws.send(JSON.stringify(o)); }

// ── Tabs ─────────────────────────────────────────────────────────────────────
function showTab(name, el) {
  document.querySelectorAll('.panel').forEach(p => p.classList.remove('on'));
  document.querySelectorAll('.tb').forEach(b => b.classList.remove('on'));
  document.getElementById('panel-' + name).classList.add('on');
  el.classList.add('on');
  if (name === 'wifi') loadWifiInfo();
}

// ── Text send ─────────────────────────────────────────────────────────────────
function doSendText() {
  const el = document.getElementById('ti');
  if (el.value) { send({type:'keyboard',action:'type',text:el.value}); el.value=''; }
}
function tapKey(h) {
  send({type:'keyboard',action:'press',key:h});
  setTimeout(() => send({type:'keyboard',action:'release',key:h}), 40);
}

// ── JS event.code → HID keycode map ──────────────────────────────────────────
const J2H = {
  Escape:177, F1:194, F2:195, F3:196, F4:197, F5:198, F6:199,
  F7:200, F8:201, F9:202, F10:203, F11:204, F12:205,
  PrintScreen:206, ScrollLock:207, Pause:208,
  Backquote:96,
  Digit1:49, Digit2:50, Digit3:51, Digit4:52, Digit5:53,
  Digit6:54, Digit7:55, Digit8:56, Digit9:57, Digit0:48,
  Minus:45, Equal:61, Backspace:178,
  Tab:179,
  KeyQ:113, KeyW:119, KeyE:101, KeyR:114, KeyT:116,
  KeyY:121, KeyU:117, KeyI:105, KeyO:111, KeyP:112,
  BracketLeft:91, BracketRight:93, Backslash:92,
  CapsLock:193,
  KeyA:97,  KeyS:115, KeyD:100, KeyF:102, KeyG:103,
  KeyH:104, KeyJ:106, KeyK:107, KeyL:108,
  Semicolon:59, Quote:39, Enter:176,
  ShiftLeft:129,
  KeyZ:122, KeyX:120, KeyC:99, KeyV:118, KeyB:98,
  KeyN:110, KeyM:109, Comma:44, Period:46, Slash:47,
  ShiftRight:133,
  ControlLeft:128, MetaLeft:131, AltLeft:130, Space:32,
  AltRight:134, MetaRight:135, ControlRight:132,
  ArrowLeft:216, ArrowUp:218, ArrowDown:217, ArrowRight:215,
  Insert:209, Home:210, PageUp:211, Delete:212, End:213, PageDown:214,
  NumLock:219,
  NumpadDivide:220, NumpadMultiply:221, NumpadSubtract:222, NumpadAdd:223,
  NumpadEnter:224,
  Numpad1:225, Numpad2:226, Numpad3:227, Numpad4:228, Numpad5:229,
  Numpad6:230, Numpad7:231, Numpad8:232, Numpad9:233, Numpad0:234,
  NumpadDecimal:235
};

// ── On-screen keyboard ────────────────────────────────────────────────────────
const activeMods = new Set();
// Modifier HID codes that auto-release after one non-modifier keypress
const STICKY_MODS = new Set([128,129,130,131,132,133,134,135]); // not CapsLock(193)

document.querySelectorAll('.k[data-h]').forEach(btn => {
  const h   = parseInt(btn.dataset.h, 10);
  const mod = !!btn.dataset.mod;

  btn.addEventListener('pointerdown', e => {
    e.preventDefault();
    if (mod) {
      if (activeMods.has(h)) {
        activeMods.delete(h); btn.classList.remove('on');
        send({type:'keyboard',action:'release',key:h});
      } else {
        activeMods.add(h); btn.classList.add('on');
        send({type:'keyboard',action:'press',key:h});
      }
    } else {
      btn.classList.add('pr');
      send({type:'keyboard',action:'press',key:h});
    }
  });

  if (!mod) {
    const release = () => {
      btn.classList.remove('pr');
      send({type:'keyboard',action:'release',key:h});
      // auto-release sticky modifiers (not CapsLock)
      STICKY_MODS.forEach(m => {
        if (activeMods.has(m)) {
          activeMods.delete(m);
          const mb = document.querySelector('.k[data-h="'+m+'"]');
          if (mb) mb.classList.remove('on');
          send({type:'keyboard',action:'release',key:m});
        }
      });
    };
    btn.addEventListener('pointerup',     release);
    btn.addEventListener('pointercancel', release);
  }
});

// ── Physical keyboard capture ─────────────────────────────────────────────────
function physDown(e) {
  e.preventDefault(); e.stopPropagation();
  const h = J2H[e.code];
  if (h !== undefined) send({type:'keyboard',action:'press',key:h});
}
function physUp(e) {
  e.preventDefault(); e.stopPropagation();
  const h = J2H[e.code];
  if (h !== undefined) send({type:'keyboard',action:'release',key:h});
}
function toggleKbdCapture(on) {
  if (on) {
    document.addEventListener('keydown', physDown, {capture:true});
    document.addEventListener('keyup',   physUp,   {capture:true});
  } else {
    document.removeEventListener('keydown', physDown, {capture:true});
    document.removeEventListener('keyup',   physUp,   {capture:true});
  }
}

// ── Physical mouse capture (Pointer Lock) ─────────────────────────────────────
let mouseLocked = false;
function physMouseMove(e) {
  if (!mouseLocked) return;
  const sensitivity = parseFloat(document.getElementById('sens').value);
  const x = Math.max(-127, Math.min(127, Math.round(e.movementX * sensitivity)));
  const y = Math.max(-127, Math.min(127, Math.round(e.movementY * sensitivity)));
  if (x !== 0 || y !== 0) send({type:'mouse',action:'move',x,y,wheel:0});
}
function physWheel(e) {
  if (!mouseLocked) return;
  e.preventDefault();
  send({type:'mouse',action:'scroll',wheel: e.deltaY > 0 ? -1 : 1});
}
function physMouseBtn(e, act) {
  if (!mouseLocked) return;
  e.preventDefault();
  const bmap = {0:1,1:4,2:2};
  const b = bmap[e.button] || 1;
  send({type:'mouse',action:act,button:b});
}
document.addEventListener('pointerlockchange', () => {
  mouseLocked = !!document.pointerLockElement;
  document.getElementById('phys-ms').checked = mouseLocked;
  document.getElementById('tp').classList.toggle('active-lock', mouseLocked);
  if (mouseLocked) {
    document.addEventListener('mousemove',  physMouseMove);
    document.addEventListener('wheel',      physWheel,    {passive:false});
    document.addEventListener('mousedown',  e => physMouseBtn(e,'press'));
    document.addEventListener('mouseup',    e => physMouseBtn(e,'release'));
  } else {
    document.removeEventListener('mousemove', physMouseMove);
    document.removeEventListener('wheel',     physWheel);
  }
});
function toggleMouseCapture(on) {
  if (on) { document.getElementById('tp').requestPointerLock(); }
  else    { document.exitPointerLock(); }
}

// ── Touchpad (on-screen) ──────────────────────────────────────────────────────
const tp = document.getElementById('tp');
let tpOn = false, tpLX = 0, tpLY = 0, tpAX = 0, tpAY = 0;
let tpMoved = false, tpTimer = null;

function getSens() { return parseFloat(document.getElementById('sens').value); }

tp.addEventListener('pointerdown', e => {
  tp.setPointerCapture(e.pointerId);
  tpLX = e.clientX; tpLY = e.clientY;
  tpAX = 0; tpAY = 0; tpMoved = false; tpOn = true;
  // Long-press → right click
  tpTimer = setTimeout(() => {
    tpTimer = null;
    if (!tpMoved) send({type:'mouse',action:'click',button:2});
  }, 600);
  e.preventDefault();
});
tp.addEventListener('pointermove', e => {
  if (!tpOn) return;
  const s = getSens();
  const dx = (e.clientX - tpLX) * s;
  const dy = (e.clientY - tpLY) * s;
  tpLX = e.clientX; tpLY = e.clientY;
  if (Math.abs(dx) > 0.3 || Math.abs(dy) > 0.3) tpMoved = true;
  tpAX += dx; tpAY += dy;
  const mx = Math.trunc(tpAX), my = Math.trunc(tpAY);
  if (mx || my) {
    tpAX -= mx; tpAY -= my;
    send({type:'mouse',action:'move',
          x:Math.max(-127,Math.min(127,mx)),
          y:Math.max(-127,Math.min(127,my)),wheel:0});
  }
  e.preventDefault();
});
tp.addEventListener('pointerup', e => {
  if (tpTimer) { clearTimeout(tpTimer); tpTimer = null; }
  if (!tpMoved) send({type:'mouse',action:'click',button:1});
  tpOn = false; e.preventDefault();
});
tp.addEventListener('pointercancel', () => {
  if (tpTimer) { clearTimeout(tpTimer); tpTimer = null; }
  tpOn = false;
});
tp.addEventListener('contextmenu', e => e.preventDefault());

// ── Mouse buttons ─────────────────────────────────────────────────────────────
document.querySelectorAll('.mb').forEach(btn => {
  const b = parseInt(btn.dataset.btn, 10);
  btn.addEventListener('pointerdown', e => {
    e.preventDefault(); btn.classList.add('pr');
    send({type:'mouse',action:'press',button:b});
  });
  const rel = () => { btn.classList.remove('pr'); send({type:'mouse',action:'release',button:b}); };
  btn.addEventListener('pointerup',     rel);
  btn.addEventListener('pointercancel', rel);
  btn.addEventListener('contextmenu',   e => e.preventDefault());
});

// ── Scroll buttons ────────────────────────────────────────────────────────────
let scrIv = null;
function startScroll(d) {
  send({type:'mouse',action:'scroll',wheel:d});
  scrIv = setInterval(() => send({type:'mouse',action:'scroll',wheel:d}), 80);
}
function stopScroll() { clearInterval(scrIv); scrIv = null; }

['su','sd'].forEach((id,i) => {
  const dir = i === 0 ? 1 : -1;
  const el = document.getElementById(id);
  el.addEventListener('pointerdown', e => { e.preventDefault(); el.classList.add('pr'); startScroll(dir); });
  el.addEventListener('pointerup',     () => { el.classList.remove('pr'); stopScroll(); });
  el.addEventListener('pointercancel', () => { el.classList.remove('pr'); stopScroll(); });
});

// ── WiFi settings ─────────────────────────────────────────────────────────────
function loadWifiInfo() {
  fetch('/wifi-info')
    .then(r => r.json())
    .then(d => {
      document.getElementById('wi-ssid').textContent = d.ssid || '\u2014';
      document.getElementById('wi-ip').textContent   = d.ip   || '\u2014';
    })
    .catch(() => {});
}
function doWifiReset() {
  if (!confirm('Reset WiFi credentials?\n\nThe device will reboot into setup mode.\nYou will need to reconnect to "ESP32-HID-Setup" and enter new credentials.')) return;
  const btn = document.getElementById('wifiResetBtn');
  const st  = document.getElementById('wi-status');
  btn.disabled  = true;
  st.textContent = '\u23F3 Resetting\u2026';
  fetch('/wifi-reset', {method:'POST'})
    .then(r => r.text())
    .then(t => { st.textContent = '\u2714 ' + t; })
    .catch(() => { st.textContent = 'Error — please try again.'; btn.disabled = false; });
}

// ── Boot ──────────────────────────────────────────────────────────────────────
connect();
</script>
</body>
</html>
)rawliteral";
// clang-format on
