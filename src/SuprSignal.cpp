#include "SuprSignal.hpp"
#include "SignalColors.hpp"

SuprSignal::SuprSignal() {
  _wifiManager = new WiFiManager();
  _wifiServer = new WiFiServer(PORT);

  lastAnim = 0;
  lastMessageTime = 0;
  cur_idx = CURSOR_MIN;
  _wifi_connected = false;

  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(115200);

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.setBrightness(20);
  FastLED.show();

  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin();
    M5.Display.print(".");
    delay(2000);
  }
  _wifi_connected = true;
  _wifiServer->begin();

  for (int i = 0; i < 5; i++) {
    sys_leds[i] = coal;
    leds[i] = coal;
  }
  FastLED.show();
}

SuprSignal::~SuprSignal() {
  if (_wifiManager) {
    delete _wifiManager;
    _wifiManager = nullptr;
  }
  if (_wifiServer) {
    delete _wifiServer;
    _wifiServer = nullptr;
  }
}
void SuprSignal::Accept() {
  WiFiClient client = _wifiServer->available();
  if (!client || !client.connected())
    return;

  char buffer[SIGNAL_LEN];
  size_t bread = client.readBytes(buffer, SIGNAL_LEN);
  if (bread < SIGNAL_LEN)
    return;

  uint8_t *ubuffer = reinterpret_cast<uint8_t *>(buffer);
  uint8_t mask[SIGNAL_LED_LEN] = {0};
  CRGB palette[SIGNAL_LED_LEN];

  for (size_t i = 0; i < SIGNAL_LED_LEN; ++i)
    mask[i] = ubuffer[i];

  int idx = SIGNAL_LED_LEN;

  for (size_t i = 0; i < SIGNAL_LED_LEN; ++i) {
    palette[i].r = ubuffer[idx++];
    palette[i].g = ubuffer[idx++];
    palette[i].b = ubuffer[idx++];
  }

  ReadSignal(mask, palette);
  lastMessageTime = millis();
}
void SuprSignal::ReadSignal(const uint8_t *mask, const CRGB *palette) {
  for (size_t i = 0; i < SIGNAL_LED_LEN; ++i)
    leds[i] = mask[i] ? palette[i] : off;
}
void SuprSignal::Present() {
  M5.update();
  Accept();
  Analysis();

  if (M5.BtnA.wasPressed()) {
    M5.Display.println("Resetting Settings...");
    _wifiManager->resetSettings();
    _wifiManager->startConfigPortal("SUPRLIGHT");
  }

  FastLED.show();
}
void SuprSignal::SetSignalSuccess(uint8_t idx) {
  sys_leds[idx] = lime;
  leds[idx] = sys_leds[idx];
}
void SuprSignal::SetSignalError(uint8_t idx) {
  sys_leds[idx] = blood_red;
  leds[idx] = sys_leds[idx];
}
void SuprSignal::Analysis() {

  static unsigned long pauseUntil = 0;
  unsigned long now = millis();

  if (now - lastMessageTime < IDLE_INTERVAL) {
    return;
  }

  if (_wifi_connected) {
    SetSignalSuccess(0);
  } else {
    SetSignalError(0);
  }

  if (pauseUntil > 0 && now < pauseUntil)
    return;

  if (pauseUntil > 0 && now >= pauseUntil) {
    pauseUntil = 0;
    cur_idx = 0;
    for (uint8_t i = CURSOR_MIN; i <= CURSOR_MAX; i++)
      leds[i] = sys_leds[i];
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
