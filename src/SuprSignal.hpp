#include <FastLED.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiManager.h>

#define LED_PIN 27
#define NUM_LEDS 25
#define CURSOR_MIN 0
#define CURSOR_MAX 4

class SuprSignal {

public:
  SuprSignal();
  ~SuprSignal();

private:

  CRGB leds[NUM_LEDS];
  CRGB sys_leds[5];

  static constexpr int MSG_LEN = 100;
  static constexpr size_t USR_LEN = 25;
  static constexpr int PORT = 666;
  unsigned long lastCheck;
  unsigned long lastAnim;
  unsigned long CHECK_INTERVAL;
  unsigned long ANIM_INTERVAL;

  bool _wifi_connected;
  bool wan;
  uint8_t cur_idx;

  WiFiManager _wifiManager;
  WiFiServer _wifiServer;

  void Accept();
  void ReadSignal(const uint8_t *mask, const CRGB *palette);
  void Present();
  void Status();
  void SetStatus(uint8_t idx);
};