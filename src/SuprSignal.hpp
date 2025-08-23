#pragma once
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

  void Present();

private:
  CRGB leds[NUM_LEDS];
  CRGB sys_leds[5];

  static constexpr int SIGNAL_LEN = 100;
  static constexpr size_t SIGNAL_LED_LEN = 25;
  static constexpr int PORT = 666;

  static constexpr unsigned long ANIM_INTERVAL = 100;
  static constexpr unsigned long IDLE_INTERVAL = 10000;

  unsigned long lastAnim = 0;
  unsigned long lastMessageTime;


  bool _wifi_connected = false;
  bool wan = false;
  uint8_t cur_idx = 0;

  WiFiManager* _wifiManager;
  WiFiServer* _wifiServer;

  void Accept();
  void ReadSignal(const uint8_t *mask, const CRGB *palette);
  void Analysis();
  void SetSignalSuccess(uint8_t idx);
  void SetSignalError(uint8_t idx);
};
