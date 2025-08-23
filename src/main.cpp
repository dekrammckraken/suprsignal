#include "SuprSignal.hpp"

SuprSignal* supr;

void setup() {
  supr = new SuprSignal();
}

void loop() {
  supr->Present();
}
