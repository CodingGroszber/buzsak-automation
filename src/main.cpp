#include <Arduino.h>
#include "io_config.h"
#include "wifi_network.h"
#include "logic.h"
#include "diag.h"

// ═════════════════════════════════════════════════════════════
//  IO LAYOUT — Edit here to add/remove sensors and relays
// ═════════════════════════════════════════════════════════════

// ── INPUTS ────────────────────────────────────────────────────
//  Inputs are active LOW: switch connects the pin to GND.
IOInput INPUTS[] = {
    {"right_sw", "Switch Right", I0_0}, // LOW = right position active
    {"left_sw", "Switch Left", I0_1},   // LOW = left  position active
                                        // { "i0_2", "Input 2", I0_2 },
                                        // { "i0_3", "Input 3", I0_3 },
                                        // { "i0_4", "Input 4", I0_4 },
                                        // { "i0_5", "Input 5", I0_5 },
};

// ── OUTPUTS ───────────────────────────────────────────────────
//  All outputs run at 24 V (Mean Well HDR-60-24, mode pin = HIGH).
IOOutput OUTPUTS[] = {
    {"well_pump", "Well Pump", Q0_0, S0_24v, HIGH, false},   // Finder 40.52 relay coil
    {"tank_pump", "Tank Pump", Q0_1, S1_24v, HIGH, false},   // Finder 40.52 relay coil
    {"switch_led", "Switch LED", Q0_2, S2_24v, HIGH, false}, // 3-state switch LED
    {"wifi_led", "WiFi LED", Q0_3, S3_24v, HIGH, false},     // blinks while connecting, solid = linked
};

// ── ANALOG INPUTS ────────────────────────────────────────────
//  4–20 mA channels; raw ADC value 0–4095 (12-bit).
IOAnalog ANALOG_INPUTS[] = {
    {"pressure", "Pressure Sensor", I0_7},       // 4–20 mA channel 0
    {"water_level", "Water Level Sensor", I0_8}, // 4–20 mA channel 1
};

const int NUM_INPUTS = sizeof(INPUTS) / sizeof(INPUTS[0]);
const int NUM_OUTPUTS = sizeof(OUTPUTS) / sizeof(OUTPUTS[0]);
const int NUM_ANALOG_SENSORS = sizeof(ANALOG_INPUTS) / sizeof(ANALOG_INPUTS[0]);

// ── WiFi LED helper ───────────────────────────────────────────
int wifiLedIndex()
{
  for (int i = 0; i < NUM_OUTPUTS; i++)
    if (String(OUTPUTS[i].name) == "wifi_led")
      return i;
  return -1;
}

// ── Setup ─────────────────────────────────────────────────────
void setup()
{
  Serial.begin(115200);

  // Initialise all outputs
  for (int i = 0; i < NUM_OUTPUTS; i++)
  {
    if (OUTPUTS[i].modePin >= 0)
    {
      pinMode(OUTPUTS[i].modePin, OUTPUT);
      digitalWrite(OUTPUTS[i].modePin, OUTPUTS[i].modeVal);
    }
    pinMode(OUTPUTS[i].pin, OUTPUT);
    digitalWrite(OUTPUTS[i].pin, LOW);
  }

  // Initialise all inputs
  for (int i = 0; i < NUM_INPUTS; i++)
    pinMode(INPUTS[i].pin, INPUT);

  // Initialise analog inputs
  for (int i = 0; i < NUM_ANALOG_SENSORS; i++)
    pinMode(ANALOG_INPUTS[i].pin, INPUT);

  setupNetwork();
  diagSetup();
}

// ── Loop ────────────────────────────────────────────
void loop()
{
  loopNetwork();
  diagLoop();
  plcLoop();
  delay(50);
}
