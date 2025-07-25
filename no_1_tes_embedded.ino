#include <Arduino.h>
//By Ahmad Fadhil Ghifari Ekaputra
const int led_pin[5] = {2, 3, 4, 5, 6}; //merah,kuning,hijau,biru,putih
int led_state[5] = {0, 0, 0, 0, 0}; 
unsigned long prev_mil[5] = {0, 0, 0, 0, 0}; 
unsigned long curr_mil;

const long interval[5] = {270, 440, 710, 1330, 1850}; //merah,kuning,hijau,biru,putih

void setup() {
  for (int i = 0; i < 5; i++) {
    pinMode(led_pin[i], OUTPUT);
  }
}

void loop() {
  curr_mil = millis();
  for (int i = 0; i < 5; i++) {
    if (curr_mil - prev_mil[i] >= interval[i]) {
      prev_mil[i] = curr_mil;

      led_state[i] = !(led_state[i]);

      digitalWrite(led_pin[i], led_state[i]);
    }
  }
}
