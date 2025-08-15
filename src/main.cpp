#include <M5Unified.h>
#include <FastLED.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include "gruvbox-dark-palette.hpp" // definisci i colori come CRGB

#define LED_PIN 27
#define NUM_LEDS 25
#define CURSOR_MIN 0
#define CURSOR_MAX 4

CRGB leds[NUM_LEDS];
CRGB sys_leds[5];

constexpr int MSG_LEN = 100;
constexpr size_t USR_LEN = 25;

unsigned long lastCheck = 0;
unsigned long lastAnim = 0;
const unsigned long CHECK_INTERVAL = 60000;
const unsigned long ANIM_INTERVAL = 60;

bool wifi_connected = false;
bool wan = false;
uint8_t cur_idx = CURSOR_MIN;

WiFiManager wifiManager;
WiFiServer server(666);

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(115200);

  // LED WS2812
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.setBrightness(50);
  FastLED.show();

  // WiFi
  M5.Display.println("Starting WiFi...");
  if (!wifiManager.autoConnect("SUPRLIGHT")) {
    M5.Display.println("WiFi failed!");
    ESP.restart();
  }
  wifi_connected = true;
  server.begin();

  M5.Display.clear();
  M5.Display.println("WiFi connected!");
  M5.Display.println(WiFi.localIP());

  // LED di sistema iniziali
  for (int i = 0; i < 5; i++) {
    sys_leds[i] = charcoal;
    leds[i] = charcoal;
  }
  FastLED.show();
}

bool is_online() {
  WiFiClient client;
  if (client.connect("alchemicats.pet", 443)) {
    client.stop();
    return true;
  }
  return false;
}

void check_system_status(uint8_t idx) {
  switch (idx) {
    case 0: wifi_connected = WiFi.isConnected(); sys_leds[0] = wifi_connected ? cream : blood_red; break;
    case 1: wan = is_online(); sys_leds[1] = wan ? cream : blood_red; break;
    case 2: sys_leds[2] = charcoal; break;
    case 3: sys_leds[3] = charcoal; break;
    case 4: sys_leds[4] = charcoal; break;
  }
}

void UserSignal(const uint8_t *mask, const CRGB *palette) {
  for (size_t i = 5; i < USR_LEN; ++i) {
    leds[i] = mask[i] ? palette[i] : CRGB::Black;
  }
}

void HandleUserSignal() {
  WiFiClient client = server.available();
  if (!client || !client.connected()) return;

  char buffer[MSG_LEN];
  size_t bread = client.readBytes(buffer, MSG_LEN);
  if (bread < MSG_LEN) return;

  uint8_t *ubuffer = reinterpret_cast<uint8_t *>(buffer);
  uint8_t mask[USR_LEN] = {0};
  CRGB palette[USR_LEN];

  for (size_t i = 0; i < USR_LEN; ++i) mask[i] = ubuffer[i];
  int idx = USR_LEN;
  for (size_t i = 0; i < USR_LEN; ++i) {
    palette[i].r = ubuffer[idx++];
    palette[i].g = ubuffer[idx++];
    palette[i].b = ubuffer[idx++];
  }

  UserSignal(mask, palette);
}

void SystemBar() {
  static unsigned long pauseUntil = 0;
  unsigned long now = millis();

  if (now - lastCheck > CHECK_INTERVAL) {
    for (uint8_t i = CURSOR_MIN; i <= CURSOR_MAX; i++) check_system_status(i);
    lastCheck = now;
  }

  if (pauseUntil > 0 && now < pauseUntil) return;

  if (pauseUntil > 0 && now >= pauseUntil) {
    pauseUntil = 0;
    cur_idx = 0;
    for (uint8_t i = CURSOR_MIN; i <= CURSOR_MAX; i++) leds[i] = sys_leds[i];
    leds[cur_idx] = amber;
    lastAnim = now;
    return;
  }

  if (now - lastAnim > ANIM_INTERVAL) {
    leds[cur_idx] = sys_leds[cur_idx];
    cur_idx++;
    if (cur_idx > CURSOR_MAX) {
      pauseUntil = now + 2000;
      return;
    }
    leds[cur_idx] = amber;
    lastAnim = now;
  }
}

void loop() {
  M5.update();
  SystemBar();
  HandleUserSignal();
  FastLED.show();

  if (M5.BtnA.wasPressed()) {
    M5.Display.println("Resetting WiFi...");
    wifiManager.resetSettings();
    ESP.restart();
  }
}
