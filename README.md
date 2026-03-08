# ESP32-S2 Mini — WiFi HID Device

Control a host computer **wirelessly** via a web browser.  
The ESP32-S2 Mini presents itself to the host as two independent USB HID devices (keyboard **and** mouse) while simultaneously hosting a control UI on its own WiFi Access Point.

---

## Features

| Feature | Details |
|---|---|
| USB HID keyboard | All key codes, F-keys, modifier keys, combos |
| USB HID mouse | Move, left / middle / right buttons, scroll wheel |
| WiFi AP | Self-hosted network — no router needed |
| Web UI | Served on `http://192.168.4.1` |
| On-screen keyboard | Full QWERTY + F-row + nav block, sticky modifiers |
| On-screen touchpad | Pointer-events based, tap = left click, long-press = right click |
| Mouse buttons | Left / Middle / Right with press-and-hold support |
| Scroll wheel | Hold-to-scroll buttons |
| Physical keyboard capture | Browser intercepts keydown/keyup events → HID |
| Physical mouse lock | Pointer Lock API → raw movement + buttons → HID |
| Text send | Type a string and send all characters as HID keystrokes |
| Auto-reconnect | WebSocket reconnects automatically after 2 s |

---

## Hardware

| Component | Value |
|---|---|
| Board | LOLIN S2 Mini (ESP32-S2, 240 MHz, 4 MB flash) |
| USB connector | USB-C (native USB OTG — no CH340/CP2102 needed) |
| WiFi | 802.11 b/g/n 2.4 GHz |

---

## Software Setup

### Option A — Arduino IDE

1. Install the **ESP32 board package** by Espressif (≥ 2.0.11):  
   *File → Preferences → Additional boards URL:*  
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`

2. Select board settings:

   | Setting | Value |
   |---|---|
   | Board | LOLIN S2 Mini |
   | USB Mode | USB-OTG (TinyUSB) |
   | USB CDC On Boot | Disabled |

3. Install libraries via *Tools → Manage Libraries*:
   - **arduinoWebSockets** by Markus Sattler (≥ 2.4.0)
   - **ArduinoJson** by Benoit Blanchon (≥ 6.21.0, **v6.x** — not v7)

4. Open `wifi_hid_device/wifi_hid_device.ino`, compile and upload.

### Option B — PlatformIO

```bash
pio run --target upload
```

Dependencies are resolved automatically via `platformio.ini`.

---

## First Use

1. Plug the ESP32-S2 Mini into the **host computer** via USB-C.  
   The host will detect it as *"WiFi HID Device"* (keyboard + mouse).

2. On a **phone or tablet**, connect to the WiFi network:

   | | |
   |---|---|
   | SSID | `ESP32-HID` |
   | Password | `hiddevice` |

3. Open a browser and navigate to `http://192.168.4.1`.

4. Use the **Keyboard** or **Mouse** tab to control the host.

---

## Web UI Tabs

### ⌨ Keyboard tab

- **Text field** — type any string and press *Send* (or Enter); all characters are forwarded as HID keystrokes.
- **On-screen keyboard** — click / tap any key; modifier keys (Shift, Ctrl, Alt, Win, AltGr) are **sticky** — tap once to arm, tap a regular key to fire the combo, then they auto-release. CapsLock stays on until tapped again.
- **Capture physical keyboard** — when checked, the browser intercepts all keydown/keyup events from your physical keyboard and forwards them as HID events. Overrides browser shortcuts.
- **Capture physical mouse** — requests Pointer Lock; once granted, your physical mouse (movement + buttons + scroll) is forwarded to the host.

### 🖱 Mouse tab

- **Sensitivity slider** — scales pointer speed (0.2× – 6×).
- **Touchpad area** — drag to move; **tap** = left click; **long-press (600 ms)** = right click.
- **Mouse buttons** — Left / Middle / Right with full press-and-hold support.
- **Scroll buttons** — hold *▲ Scroll Up* or *▼ Scroll Down* for continuous scrolling.

---

## WebSocket Protocol

All messages are UTF-8 JSON objects sent on **port 81**.

### Keyboard

```json
{"type":"keyboard","action":"press",    "key": 97}
{"type":"keyboard","action":"release",  "key": 97}
{"type":"keyboard","action":"releaseAll"}
{"type":"keyboard","action":"type",     "text":"Hello, World!"}
{"type":"keyboard","action":"combo",    "keys":[128, 99]}
```

### Mouse

```json
{"type":"mouse","action":"move",    "x": 10,  "y": -5, "wheel": 0}
{"type":"mouse","action":"press",   "button": 1}
{"type":"mouse","action":"release", "button": 1}
{"type":"mouse","action":"click",   "button": 1}
{"type":"mouse","action":"scroll",  "wheel": -1}
```

Button values: `1` = left, `2` = right, `4` = middle.

---

## HID Key Code Reference

Codes follow the Arduino Keyboard library convention:

| Category | Codes |
|---|---|
| ASCII printable | 32 – 126 (sent directly) |
| Esc | 177 |
| Backspace | 178 |
| Tab | 179 |
| Enter | 176 |
| Delete | 212 |
| Insert | 209 |
| Home / End | 210 / 213 |
| Page Up / Down | 211 / 214 |
| Arrow Left / Up / Down / Right | 216 / 218 / 217 / 215 |
| F1 – F12 | 194 – 205 |
| Print Screen | 206 |
| Scroll Lock | 207 |
| Pause | 208 |
| Caps Lock | 193 |
| Left Ctrl / Shift / Alt / GUI | 128 / 129 / 130 / 131 |
| Right Ctrl / Shift / Alt / GUI | 132 / 133 / 134 / 135 |
| Num Lock | 219 |
| Numpad / * - + Enter | 220 / 221 / 222 / 223 / 224 |
| Numpad 1 – 9, 0, . | 225 – 235 |

---

## Configuration

Edit the top of `wifi_hid_device.ino` to change the WiFi credentials:

```cpp
static const char* AP_SSID     = "ESP32-HID";
static const char* AP_PASSWORD = "hiddevice";   // min 8 characters
static const int   AP_CHANNEL  = 1;
```

---

## Troubleshooting

| Symptom | Fix |
|---|---|
| Host doesn't see keyboard/mouse | Check *USB Mode = USB-OTG (TinyUSB)* and *USB CDC On Boot = Disabled* in board settings |
| Can't connect to WiFi AP | Verify SSID/password; ensure only one device connects at a time |
| WebSocket shows "Disconnected" | Refresh the page or wait 2 s for auto-reconnect |
| Keys produce wrong characters | Check that the host keyboard layout matches (HID sends US layout codes) |

---

## License

Apache License 2.0 — see [LICENSE](LICENSE).
