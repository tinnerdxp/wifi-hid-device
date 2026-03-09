# ESP32-S2 Mini — WiFi HID Device

Control a host computer **wirelessly** via a web browser.  
The ESP32-S2 Mini presents itself to the host as two independent USB HID devices (keyboard **and** mouse) while simultaneously hosting a control UI over your home WiFi network.

---

All of the code in this repository was written by CoPilot using Sonnet 4.6. It was built in literally couple of hours with maybe 3-4 prompts in between.
The reason I wanted this is due to my set up at home and at work, I keep all the hardware in one place of the house that is not easily accessible most of the time and putting a screen / keyboard / mouse is not always an option. Same for work - we do some Android testing and sometimes it's easier to plugin something small directly into a phone/tablet/pc rather than look around for a spare keyboard and spare USB port.

## What you need:
- Buy one of these: https://www.aliexpress.com/item/1005008222967786.html
- plug it in
- install Arduino / PlatformIO on your local
- Flash the board with the code
- Connect your phone to a new WiFi access point (details in src/config.h file)
- Reboot the board by either unplugging it for a sec or by pressing RST button.
- Scan your wifi or login to your router to check what IP it got
- Navigate to http://192.168.x.y - this runs on HTTP and default port 80 - so you might want to allow your browser to do that
- Enjoy

---

## Features

| Feature | Details |
|---|---|
| USB HID keyboard | All key codes, F-keys, modifier keys, combos |
| USB HID mouse | Move, left / middle / right buttons, scroll wheel |
| WiFi provisioning | Mobile-friendly setup page to join your home network |
| Persistent credentials | Home-WiFi SSID/password stored in NVS across reboots |
| LED status indicator | Flashing = unconfigured / provisioning; solid = connected |
| On-screen keyboard | Full QWERTY + F-row + nav block, sticky modifiers |
| On-screen touchpad | Pointer-events based, tap = left click, long-press = right click |
| Mouse buttons | Left / Middle / Right with press-and-hold support |
| Scroll wheel | Hold-to-scroll buttons |
| Physical keyboard capture | Browser intercepts keydown/keyup events → HID |
| Physical mouse lock | Pointer Lock API → raw movement + buttons → HID |
| Text send | Type a string and send all characters as HID keystrokes |
| WiFi reset | Settings tab button clears credentials → reboots into setup mode |
| Auto-reconnect | WebSocket reconnects automatically after 2 s |

---

## Hardware

| Component | Value |
|---|---|
| Board | LOLIN S2 Mini (ESP32-S2, 240 MHz, 4 MB flash) |
| USB connector | USB-C (native USB OTG — no CH340/CP2102 needed) |
| WiFi | 802.11 b/g/n 2.4 GHz |
| Built-in LED | GPIO 15, active HIGH (change `LED_PIN` in `config.h` for other boards) |

---

## Configuration

All user-configurable values are in **`wifi_hid_device/config.h`** — edit that file only:

```cpp
// LED
#define LED_PIN          15       // GPIO of the built-in LED
#define LED_FLASH_MS     500      // blink period while unconfigured

// Provisioning AP (mobile phone connects here to enter home-WiFi credentials)
#define PROV_AP_SSID     "ESP32-HID-Setup"
#define PROV_AP_PASSWORD "setup1234"   // min 8 chars; "" = open network

// How long to wait for home-WiFi connection before falling back to setup mode
#define STA_CONNECT_TIMEOUT_MS  15000

// USB descriptor strings shown in the host OS device manager
#define USB_MANUFACTURER "ESP32-HID"
#define USB_PRODUCT      "WiFi HID Device"
```

`platformio.ini` no longer needs to be edited for typical customisation.

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
   
   > `DNSServer` and `Preferences` are built into the ESP32 Arduino core — no extra installation needed.

4. Open `wifi_hid_device/wifi_hid_device.ino`, compile and upload.

### Option B — PlatformIO

```bash
pio run --target upload
```

Dependencies are resolved automatically via `platformio.ini`.

---

## First Use — WiFi Provisioning

On first boot (or after a WiFi reset), the LED **flashes** and the board creates a temporary setup access point:

| | |
|---|---|
| SSID | `ESP32-HID-Setup` |
| Password | `setup1234` |

1. Plug the ESP32-S2 Mini into the **host computer** via USB-C.  
   The host detects it as *"WiFi HID Device"* (keyboard + mouse).

2. On a **phone or tablet**, connect to **`ESP32-HID-Setup`** (`setup1234`).  
   Most phones automatically open the captive portal page.  
   If not, open a browser and go to `http://192.168.4.1`.

3. The **WiFi Setup** page appears.  Tap *Scan for networks*, select your home network, enter the password and tap **Save & Connect**.

4. The device reboots and connects to your home network.  
   The LED turns **solid on** when connected.

5. Find the device's IP address (check your router's DHCP client list, or use a network scanner app) and open it in a browser on your phone or computer.

6. Use the **Keyboard** or **Mouse** tab to control the host.

---

## LED Behaviour

| LED state | Meaning |
|---|---|
| Fast flash (500 ms) | Unconfigured — waiting in setup / provisioning mode |
| Flash during boot | Attempting to connect to saved home network |
| Solid ON | Connected to home network, UI is accessible |

The LED pin and flash interval can be changed in `config.h`.

---

## Re-pairing / Reset

To forget the stored credentials and pair with a different network:

1. Open the web UI and go to the **⚙ Settings** tab.
2. Tap **Reset WiFi & Re-pair**.
3. Confirm the dialog.
4. The device clears its credentials, reboots, and returns to setup mode (LED flashes, `ESP32-HID-Setup` AP appears).

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

### ⚙ Settings tab

- Shows the current network name and IP address.
- **Reset WiFi & Re-pair** — clears stored credentials and reboots into provisioning mode.

---

## HTTP Endpoints

| Endpoint | Method | Mode | Description |
|---|---|---|---|
| `/` | GET | Both | Main HID UI (connected) or provisioning setup page |
| `/scan` | GET | Provisioning | JSON list of nearby WiFi networks |
| `/save` | POST | Provisioning | Save SSID/password → reboot |
| `/wifi-info` | GET | Connected | JSON `{ssid, ip}` of current connection |
| `/wifi-reset` | POST | Connected | Clear credentials → reboot into setup mode |

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

## Troubleshooting

| Symptom | Fix |
|---|---|
| LED flashing continuously | Device is in provisioning mode — connect to `ESP32-HID-Setup` and enter credentials |
| Host doesn't see keyboard/mouse | Check *USB Mode = USB-OTG (TinyUSB)* and *USB CDC On Boot = Disabled* in board settings |
| Can't reach UI after provisioning | Check your router DHCP list for the device IP; or use a network scanner app |
| WebSocket shows "Disconnected" | Refresh the page or wait 2 s for auto-reconnect |
| Keys produce wrong characters | Check that the host keyboard layout matches (HID sends US layout codes) |
| Device stuck connecting (15 s) | Incorrect password or network out of range — device falls back to setup mode automatically |

---

## License

Apache License 2.0 — see [LICENSE](LICENSE).
