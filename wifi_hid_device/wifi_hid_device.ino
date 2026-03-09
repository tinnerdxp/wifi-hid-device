/*
 * ESP32-S2 Mini — WiFi HID Device
 * ================================
 * Presents the ESP32-S2 to the host computer as two independent USB HID
 * devices (keyboard + mouse) while simultaneously hosting a web interface.
 *
 * Operating modes
 * ───────────────
 *  STATE_PROVISIONING  No home-WiFi credentials stored (or previous connect
 *                      attempt failed).  The board starts a provisioning AP
 *                      ("ESP32-HID-Setup") with a captive portal so a mobile
 *                      phone can enter home-WiFi credentials.
 *                      Built-in LED flashes at LED_FLASH_MS interval.
 *
 *  STATE_CONNECTED     Credentials exist and STA connection succeeded.  The
 *                      full HID control UI is served on the board's STA IP.
 *                      Built-in LED is solid ON.
 *
 * WiFi credentials are stored in NVS (Preferences library) and survive
 * power cycles.  A "Reset WiFi" button in the Settings tab of the UI clears
 * them and reboots into provisioning mode.
 *
 * Board:          LOLIN S2 Mini  (or any ESP32-S2 board)
 * Arduino IDE:    Tools → USB Mode → USB-OTG (TinyUSB)
 *                 Tools → USB CDC On Boot → Disabled
 *
 * Required libraries (install via Arduino Library Manager or PlatformIO):
 *  • arduinoWebSockets  (Markus Sattler / Links2004)  ≥ 2.4.0
 *  • ArduinoJson        (Benoît Blanchon)              ≥ 6.21.0
 *
 *  DNSServer and Preferences are part of the ESP32 Arduino core — no extra
 *  installation required.
 *
 * All user-configurable values live in config.h.
 *
 * WebSocket protocol — all messages are JSON objects sent from the browser:
 *
 *  Keyboard
 *  --------
 *  {"type":"keyboard","action":"press",   "key":<hid>}
 *  {"type":"keyboard","action":"release", "key":<hid>}
 *  {"type":"keyboard","action":"releaseAll"}
 *  {"type":"keyboard","action":"type",    "text":"<string>"}
 *  {"type":"keyboard","action":"combo",   "keys":[<hid>, ...]}
 *
 *  Mouse
 *  -----
 *  {"type":"mouse","action":"move",    "x":<-127…127>,"y":<-127…127>,"wheel":0}
 *  {"type":"mouse","action":"press",   "button":<1|2|4>}   // 1=L 2=R 4=M
 *  {"type":"mouse","action":"release", "button":<1|2|4>}
 *  {"type":"mouse","action":"click",   "button":<1|2|4>}
 *  {"type":"mouse","action":"scroll",  "wheel":<-127…127>}
 */

// config.h must be first so USB_MANUFACTURER / USB_PRODUCT are defined
// before USB.h is included.
#include "config.h"

#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDMouse.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#include "index_html.h"
#include "setup_html.h"

// ── Device state ──────────────────────────────────────────────────────────────
enum DeviceState { STATE_PROVISIONING, STATE_CONNECTED };
static DeviceState gState = STATE_PROVISIONING;

// ── USB HID devices ───────────────────────────────────────────────────────────
USBHIDKeyboard Keyboard;
USBHIDMouse    Mouse;

// ── Network objects ───────────────────────────────────────────────────────────
static DNSServer         dnsServer;
static WebServer         httpServer(HTTP_PORT);
static WebSocketsServer  wsServer(WS_PORT);

// ── Persistent storage ────────────────────────────────────────────────────────
static Preferences prefs;

// ── LED state (non-blocking blink) ────────────────────────────────────────────
static unsigned long ledLastToggleMs = 0;
static bool          ledOn           = false;

// ── Forward declarations ──────────────────────────────────────────────────────
static void startProvisioning();
static void startConnected();
static void updateLed();
static void onWebSocketEvent(uint8_t, WStype_t, uint8_t*, size_t);
static void handleKeyboardMessage(JsonDocument&);
static void handleMouseMessage(JsonDocument&);

// ── LED helper ────────────────────────────────────────────────────────────────
static void updateLed() {
    if (gState == STATE_CONNECTED) {
        digitalWrite(LED_PIN, HIGH);
        return;
    }
    // Provisioning: blink
    unsigned long now = millis();
    if (now - ledLastToggleMs >= LED_FLASH_MS) {
        ledLastToggleMs = now;
        ledOn = !ledOn;
        digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
    }
}

// ── Provisioning HTTP handlers ────────────────────────────────────────────────

// GET /  →  provisioning setup page
static void handleSetupRoot() {
    httpServer.send_P(200, "text/html", SETUP_HTML);
}

// GET /scan  →  JSON array of nearby networks
static void handleSetupScan() {
    int n = WiFi.scanNetworks();
    StaticJsonDocument<2048> doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < n; i++) {
        JsonObject net = arr.createNestedObject();
        net["ssid"]   = WiFi.SSID(i);
        net["rssi"]   = WiFi.RSSI(i);
        net["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }
    String out;
    serializeJson(doc, out);
    httpServer.send(200, "application/json", out);
}

// POST /save  →  persist credentials and reboot
static void handleSetupSave() {
    if (!httpServer.hasArg("ssid") || httpServer.arg("ssid").isEmpty()) {
        httpServer.send(400, "text/plain", "Missing SSID");
        return;
    }
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString(NVS_KEY_SSID, httpServer.arg("ssid"));
    prefs.putString(NVS_KEY_PASS, httpServer.arg("pass"));
    prefs.end();
    httpServer.send(200, "text/plain",
                    "Credentials saved. Rebooting\u2026 "
                    "If connection fails the device will return to setup mode.");
    delay(REBOOT_DELAY_MS);
    ESP.restart();
}

// ── Main (connected-mode) HTTP handlers ──────────────────────────────────────

// GET /  →  HID control UI
static void handleRoot() {
    httpServer.send_P(200, "text/html", INDEX_HTML);
}

// GET /wifi-info  →  JSON with current SSID and IP
static void handleWifiInfo() {
    StaticJsonDocument<256> doc;
    doc["ssid"] = WiFi.SSID();
    doc["ip"]   = WiFi.localIP().toString();
    String out;
    serializeJson(doc, out);
    httpServer.send(200, "application/json", out);
}

// POST /wifi-reset  →  clear credentials and reboot into provisioning mode
static void handleWifiReset() {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.clear();
    prefs.end();
    httpServer.send(200, "text/plain",
                    "WiFi credentials cleared. Rebooting into setup mode\u2026");
    delay(REBOOT_DELAY_MS);
    ESP.restart();
}

// Catch-all: captive-portal redirect in provisioning mode, 404 otherwise
static void handleNotFound() {
    if (gState == STATE_PROVISIONING) {
        httpServer.sendHeader("Location", "http://192.168.4.1/", true);
        httpServer.send(302, "text/plain", "");
        return;
    }
    httpServer.send(404, "text/plain", "404 Not Found");
}

// ── WebSocket dispatcher ──────────────────────────────────────────────────────
static void onWebSocketEvent(uint8_t /*clientNum*/, WStype_t type,
                             uint8_t* payload, size_t length) {
    if (type != WStype_TEXT) return;
    StaticJsonDocument<1024> doc;
    if (deserializeJson(doc, payload, length) != DeserializationError::Ok) return;
    const char* msgType = doc["type"] | "";
    if      (strcmp(msgType, "keyboard") == 0) handleKeyboardMessage(doc);
    else if (strcmp(msgType, "mouse")    == 0) handleMouseMessage(doc);
}

// ── Keyboard command handler ──────────────────────────────────────────────────
static void handleKeyboardMessage(JsonDocument& doc) {
    const char* action = doc["action"] | "";

    if (strcmp(action, "press") == 0) {
        uint8_t key = doc["key"] | 0;
        if (key) Keyboard.press(key);

    } else if (strcmp(action, "release") == 0) {
        uint8_t key = doc["key"] | 0;
        if (key) Keyboard.release(key);

    } else if (strcmp(action, "releaseAll") == 0) {
        Keyboard.releaseAll();

    } else if (strcmp(action, "type") == 0) {
        Keyboard.print(doc["text"] | "");

    } else if (strcmp(action, "combo") == 0) {
        for (JsonVariantConst k : doc["keys"].as<JsonArrayConst>())
            Keyboard.press(static_cast<uint8_t>(k.as<int>()));
        delay(KEY_COMBO_HOLD_MS);
        Keyboard.releaseAll();
    }
}

// ── Mouse command handler ─────────────────────────────────────────────────────
static void handleMouseMessage(JsonDocument& doc) {
    const char* action = doc["action"] | "";

    if (strcmp(action, "move") == 0) {
        Mouse.move(static_cast<int8_t>(doc["x"]     | 0),
                   static_cast<int8_t>(doc["y"]     | 0),
                   static_cast<int8_t>(doc["wheel"] | 0));
    } else if (strcmp(action, "press") == 0) {
        Mouse.press(doc["button"] | static_cast<uint8_t>(MOUSE_LEFT));
    } else if (strcmp(action, "release") == 0) {
        Mouse.release(doc["button"] | static_cast<uint8_t>(MOUSE_LEFT));
    } else if (strcmp(action, "click") == 0) {
        Mouse.click(doc["button"] | static_cast<uint8_t>(MOUSE_LEFT));
    } else if (strcmp(action, "scroll") == 0) {
        Mouse.move(0, 0, static_cast<int8_t>(doc["wheel"] | 0));
    }
}

// ── Provisioning mode startup ─────────────────────────────────────────────────
static void startProvisioning() {
    gState = STATE_PROVISIONING;

    WiFi.mode(WIFI_AP);
    WiFi.softAP(PROV_AP_SSID,
                (sizeof(PROV_AP_PASSWORD) > 1) ? PROV_AP_PASSWORD : nullptr,
                PROV_AP_CHANNEL);

    // Captive-portal DNS: redirect every domain to the AP's IP
    dnsServer.start(53, "*", WiFi.softAPIP());

    httpServer.on("/",     HTTP_GET,  handleSetupRoot);
    httpServer.on("/scan", HTTP_GET,  handleSetupScan);
    httpServer.on("/save", HTTP_POST, handleSetupSave);
    httpServer.onNotFound(handleNotFound);
    httpServer.begin();

    wsServer.begin();
    wsServer.onEvent(onWebSocketEvent);
}

// ── Connected mode startup ────────────────────────────────────────────────────
static void startConnected() {
    gState = STATE_CONNECTED;

    httpServer.on("/",           HTTP_GET,  handleRoot);
    httpServer.on("/wifi-info",  HTTP_GET,  handleWifiInfo);
    httpServer.on("/wifi-reset", HTTP_POST, handleWifiReset);
    httpServer.onNotFound(handleNotFound);
    httpServer.begin();

    wsServer.begin();
    wsServer.onEvent(onWebSocketEvent);
}

// ── Arduino setup ─────────────────────────────────────────────────────────────
void setup() {
    // LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // USB HID — must call begin() on each device before USB.begin()
    Keyboard.begin();
    Mouse.begin();
    USB.begin();

    // Load stored WiFi credentials
    prefs.begin(NVS_NAMESPACE, true);   // read-only
    String ssid = prefs.getString(NVS_KEY_SSID, "");
    String pass = prefs.getString(NVS_KEY_PASS, "");
    prefs.end();

    if (ssid.isEmpty()) {
        startProvisioning();
        return;
    }

    // Attempt STA connection; update LED while waiting
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - t0 < STA_CONNECT_TIMEOUT_MS) {
        updateLed();
        delay(50);
    }

    if (WiFi.status() == WL_CONNECTED) {
        startConnected();
    } else {
        // Credentials invalid or network unreachable — fall back to setup
        WiFi.disconnect(true);
        startProvisioning();
    }
}

// ── Arduino loop ──────────────────────────────────────────────────────────────
void loop() {
    if (gState == STATE_PROVISIONING) {
        dnsServer.processNextRequest();
    }
    httpServer.handleClient();
    wsServer.loop();
    updateLed();
}
