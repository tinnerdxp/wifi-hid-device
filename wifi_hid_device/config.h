/*
 * config.h — User-configurable settings for ESP32-S2 WiFi HID Device
 * ====================================================================
 * Edit this file to change board pin assignments, WiFi credentials,
 * server ports, and USB descriptor strings.
 *
 * All other source files include this header; no other file needs editing
 * for typical customisation.
 */

#pragma once

// ── Built-in LED ──────────────────────────────────────────────────────────────
// GPIO of the built-in blue LED on the LOLIN S2 Mini (active HIGH).
// Change to match your board if different (e.g. 2 on most generic ESP32 boards).
#define LED_PIN         15

// Blink period in milliseconds while the device is unconfigured / in
// provisioning mode.  Half-period ON, half-period OFF.
#define LED_FLASH_MS    500

// ── Provisioning Access Point ─────────────────────────────────────────────────
// When no home-WiFi credentials are stored the device creates this AP so a
// mobile phone can connect and fill in the credentials.
//
// Set PROV_AP_PASSWORD to "" to create an open (password-free) network.
// WPA2 requires a password of at least 8 characters.
#define PROV_AP_SSID        "ESP32-HID-Setup"
#define PROV_AP_PASSWORD    "setup1234"
#define PROV_AP_CHANNEL     1

// ── STA (home-network) connection timeout ─────────────────────────────────────
// How long (ms) to wait for WiFi.status() == WL_CONNECTED before giving up
// and falling back to provisioning mode.
#define STA_CONNECT_TIMEOUT_MS  15000

// ── Web-server ports ──────────────────────────────────────────────────────────
#define HTTP_PORT   80
#define WS_PORT     81

// ── Timing constants ──────────────────────────────────────────────────────────
// Delay (ms) before calling ESP.restart() to give the HTTP response time to
// reach the browser before the network goes down.
#define REBOOT_DELAY_MS     1200

// Hold time (ms) for simultaneous key presses in a "combo" action.
#define KEY_COMBO_HOLD_MS   20

// ── Non-volatile storage (NVS / Preferences) ─────────────────────────────────
// Namespace and keys used to persist the home-WiFi credentials across reboots.
#define NVS_NAMESPACE   "hid_cfg"
#define NVS_KEY_SSID    "ssid"
#define NVS_KEY_PASS    "pass"

// ── USB descriptor strings ────────────────────────────────────────────────────
// These appear in the host OS device manager / system profiler.
// They must be defined before USB.h is included; the TinyUSB stack picks
// them up at compile time.
//
// NOTE: platformio.ini no longer sets -DUSB_MANUFACTURER / -DUSB_PRODUCT
//       so these defines here are the single source of truth.
#define USB_MANUFACTURER    "ESP32-HID"
#define USB_PRODUCT         "WiFi HID Device"
