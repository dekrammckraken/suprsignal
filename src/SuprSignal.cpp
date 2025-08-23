#include "SuprSignal.hpp"
#include "SignalColors.hpp"

SuprSignal::SuprSignal() {
  _wifiManager = new WiFiManager();
  _wifiServer  = new WiFiServer(PORT);

  lastCheck = 0;
  lastAnim = 0;
  CHECK_INTERVAL = 60000;
  ANIM_INTERVAL = 60;
  cur_idx = CURSOR_MIN;
  _wifi_connected = false;

  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(115200);

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.setBrightness(50);
  FastLED.show();

  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin();
    M5.Display.print(".");
    delay(2000);
  }
  _wifi_connected = true;
  _wifiServer->begin();

  M5.Display.clear();
  M5.Display.println("WiFi connected!");
  M5.Display.println(WiFi.localIP());

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

  char buffer[MSG_LEN];
  size_t bread = client.readBytes(buffer, MSG_LEN);
  if (bread < MSG_LEN)
    return;

  uint8_t *ubuffer = reinterpret_cast<uint8_t *>(buffer);
  uint8_t mask[USR_LEN] = {0};
  CRGB palette[USR_LEN];

  for (size_t i = 0; i < USR_LEN; ++i)
    mask[i] = ubuffer[i];

  int idx = USR_LEN;
  
  for (size_t i = 0; i < USR_LEN; ++i) {
    palette[i].r = ubuffer[idx++];
    palette[i].g = ubuffer[idx++];
    palette[i].b = ubuffer[idx++];
  }

  ReadSignal(mask, palette);
}
void SuprSignal::ReadSignal(const uint8_t *mask, const CRGB *palette) {
  for (size_t i = 5; i < USR_LEN; ++i)
    leds[i] = mask[i] ? palette[i] : off;
}
void SuprSignal::Present() {
  M5.update();
  Accept();
  Diagnosis();

  if (M5.BtnA.wasPressed()) {
    M5.Display.println("Resetting Settings...");
    _wifiManager->resetSettings();
    _wifiManager->startConfigPortal("SUPRLIGHT");
  }

  FastLED.show();
}
void SuprSignal::SetSignal(uint8_t idx) {
  sys_leds[idx] = amber;
  leds[idx] = sys_leds[idx];
}
void SuprSignal::Diagnosis() {

  static unsigned long pauseUntil = 0;
  unsigned long now = millis();

  if (now - lastCheck > CHECK_INTERVAL) {
    for (uint8_t i = CURSOR_MIN; i <= CURSOR_MAX; i++)
      SetSignal(i);
    lastCheck = now;
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
