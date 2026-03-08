/*
 * ESP32-S2 Mini — WiFi HID Device
 * ================================
 * Presents the ESP32-S2 to the host computer as two independent USB HID
 * devices (keyboard + mouse) while simultaneously hosting a web interface
 * over its own WiFi Access Point.
 *
 * The web UI (served on http://192.168.4.1) lets you:
 *  • Type via an on-screen keyboard with full modifier-key support
 *  • Move the mouse via a touchpad area
 *  • Click / scroll via on-screen mouse buttons
 *  • Forward physical keyboard events captured inside the browser
 *  • Forward physical mouse movement via the Pointer Lock API
 *
 * Board:          LOLIN S2 Mini  (or any ESP32-S2 board)
 * Arduino IDE:    Tools → USB Mode → USB-OTG (TinyUSB)
 *                 Tools → USB CDC On Boot → Disabled
 *
 * Required libraries (install via Arduino Library Manager or PlatformIO):
 *  • arduinoWebSockets  (Markus Sattler / Links2004)  ≥ 2.4.0
 *  • ArduinoJson        (Benoît Blanchon)              ≥ 6.21.0
 *
 * WebSocket protocol — all messages are JSON objects sent from the browser:
 *
 *  Keyboard
 *  --------
 *  {"type":"keyboard","action":"press",   "key":<hid>}
 *  {"type":"keyboard","action":"release", "key":<hid>}
 *  {"type":"keyboard","action":"releaseAll"}
 *  {"type":"keyboard","action":"type",    "text":"<string>"}
 *  {"type":"keyboard","action":"combo",   "keys":[<hid>, ...]}  // simultaneous
 *
 *  Mouse
 *  -----
 *  {"type":"mouse","action":"move",    "x":<-127…127>,"y":<-127…127>,"wheel":0}
 *  {"type":"mouse","action":"press",   "button":<1|2|4>}   // 1=L 2=R 4=M
 *  {"type":"mouse","action":"release", "button":<1|2|4>}
 *  {"type":"mouse","action":"click",   "button":<1|2|4>}
 *  {"type":"mouse","action":"scroll",  "wheel":<-127…127>}
 *
 * HID key codes match the Arduino Keyboard library:
 *  ASCII printable 32-126 sent directly;  special keys use KEY_* constants.
 */

#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDMouse.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

#include "index_html.h"

// ── WiFi Access-Point configuration ──────────────────────────────────────────
static const char* AP_SSID     = "ESP32-HID";
static const char* AP_PASSWORD = "hiddevice";   // WPA2 requires ≥ 8 chars
static const int   AP_CHANNEL  = 1;

// ── Server ports ─────────────────────────────────────────────────────────────
static const uint16_t HTTP_PORT = 80;
static const uint16_t WS_PORT   = 81;

// ── USB HID devices ───────────────────────────────────────────────────────────
USBHIDKeyboard Keyboard;
USBHIDMouse    Mouse;

// ── Network servers ───────────────────────────────────────────────────────────
WebServer        httpServer(HTTP_PORT);
WebSocketsServer wsServer(WS_PORT);

// ── Forward declarations ──────────────────────────────────────────────────────
static void onWebSocketEvent(uint8_t clientNum, WStype_t type,
                             uint8_t* payload, size_t length);
static void handleKeyboardMessage(JsonDocument& doc);
static void handleMouseMessage(JsonDocument& doc);

// ── HTTP handlers ─────────────────────────────────────────────────────────────
static void handleRoot() {
    httpServer.send_P(200, "text/html", INDEX_HTML);
}

static void handleNotFound() {
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
        const char* text = doc["text"] | "";
        Keyboard.print(text);

    } else if (strcmp(action, "combo") == 0) {
        // Press all keys simultaneously then release all
        JsonArrayConst keys = doc["keys"].as<JsonArrayConst>();
        for (JsonVariantConst k : keys) {
            Keyboard.press(static_cast<uint8_t>(k.as<int>()));
        }
        delay(20);
        Keyboard.releaseAll();
    }
}

// ── Mouse command handler ─────────────────────────────────────────────────────
static void handleMouseMessage(JsonDocument& doc) {
    const char* action = doc["action"] | "";

    if (strcmp(action, "move") == 0) {
        int8_t x     = static_cast<int8_t>(doc["x"]     | 0);
        int8_t y     = static_cast<int8_t>(doc["y"]     | 0);
        int8_t wheel = static_cast<int8_t>(doc["wheel"] | 0);
        Mouse.move(x, y, wheel);

    } else if (strcmp(action, "press") == 0) {
        uint8_t button = doc["button"] | static_cast<uint8_t>(MOUSE_LEFT);
        Mouse.press(button);

    } else if (strcmp(action, "release") == 0) {
        uint8_t button = doc["button"] | static_cast<uint8_t>(MOUSE_LEFT);
        Mouse.release(button);

    } else if (strcmp(action, "click") == 0) {
        uint8_t button = doc["button"] | static_cast<uint8_t>(MOUSE_LEFT);
        Mouse.click(button);

    } else if (strcmp(action, "scroll") == 0) {
        int8_t wheel = static_cast<int8_t>(doc["wheel"] | 0);
        Mouse.move(0, 0, wheel);
    }
}

// ── Arduino setup ─────────────────────────────────────────────────────────────
void setup() {
    // USB HID — must call begin() on each device before USB.begin()
    Keyboard.begin();
    Mouse.begin();
    USB.begin();

    // WiFi Access Point
    WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL);

    // HTTP server
    httpServer.on("/", HTTP_GET, handleRoot);
    httpServer.onNotFound(handleNotFound);
    httpServer.begin();

    // WebSocket server
    wsServer.begin();
    wsServer.onEvent(onWebSocketEvent);
}

// ── Arduino loop ──────────────────────────────────────────────────────────────
void loop() {
    httpServer.handleClient();
    wsServer.loop();
}
