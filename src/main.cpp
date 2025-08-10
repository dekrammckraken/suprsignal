// This file is part of suprsignal
//
// suprps is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// suprps is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include <FastLED.h>
#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <cstdint>
#include <cstring>

#include "gruvbox-dark-palette.hpp"

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
unsigned long lastPause = 0;
const unsigned long CHECK_INTERVAL = 60000;
const unsigned long ANIM_INTERVAL = 60;
const unsigned long PAUSE_INTERVAL = 2000;


bool wifi_connected = false;
bool wan = false;

uint8_t cur_idx = CURSOR_MIN;
bool moving_forward = true;

WiFiManager wifiManager;
WiFiServer server(666);

void setup() {
  M5.begin(true, false, true);
  Serial.begin(115200);

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear(true);
  FastLED.show();

  if (!wifiManager.autoConnect("SUPRLIGHT")) {
    ESP.restart();
  }
  wifi_connected = true;
  server.begin();

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
  case 0:
    wifi_connected = WiFi.isConnected();
    sys_leds[0] = wifi_connected ? cream : blood_red;
    break;

  case 1:
    wan = is_online();
    sys_leds[1] = wan ? cream : blood_red;
    break;

  case 2:
    sys_leds[2] = charcoal;
    break;

  case 3:
    sys_leds[3] = charcoal;
    break;

  case 4:
    sys_leds[4] = charcoal;
    break;

  default:
    break;
  }
}

void UserSignal(const uint8_t *mask, const CRGB *palette) {
  for (size_t i = 5; i < USR_LEN; ++i) {
    leds[i] = mask[i] ? palette[i] : CRGB::Black;
  }
}

void HandleUserSignal() {
  WiFiClient client = server.available();
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

  UserSignal(mask, palette);
}

void SystemBar() {
    static unsigned long pauseUntil = 0;
    unsigned long now = millis();
    
    if (now - lastCheck > CHECK_INTERVAL) {
        for (uint8_t i = CURSOR_MIN; i <= CURSOR_MAX; i++) {
            check_system_status(i);
        }
        lastCheck = now;
    }
    
    if (pauseUntil > 0 && now < pauseUntil) {
        return;
    }
    
    if (pauseUntil > 0 && now >= pauseUntil) {
        pauseUntil = 0;
        cur_idx = 0;
        for (uint8_t i = CURSOR_MIN; i <= CURSOR_MAX; i++) {
            leds[i] = sys_leds[i];
        }
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

  SystemBar();
  HandleUserSignal();
  FastLED.show();
}
