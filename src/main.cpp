#include <Arduino.h>
#include "io_config.h"
#include "wifi_network.h"
#include "logic.h"

// ═════════════════════════════════════════════════════════════
//  IO LAYOUT — Edit here to add/remove sensors and relays
// ═════════════════════════════════════════════════════════════

// ── INPUTS ────────────────────────────────────────────────────
IOInput INPUTS[] = {
    {"button", "Button Input", I0_0},
    // { "door",     "Door Sensor",   I0_1 },
    // { "motion",   "Motion Sensor", I0_2 },
    // { "sensor_3", "Sensor 3",      I0_3 },
    // { "sensor_4", "Sensor 4",      I0_4 },
    // { "sensor_5", "Sensor 5",      I0_5 },
};

// ── OUTPUTS ───────────────────────────────────────────────────
IOOutput OUTPUTS[] = {
    {"relay", "Relay", Q0_0, S0_24v, HIGH, true},
    {"wifi_led", "WiFi LED", Q0_1, S1_24v, LOW, false},
    // { "output_2", "Output 2", Q0_2, S2_24v, HIGH, true  },
    // { "output_3", "Output 3", Q0_3, S3_24v, HIGH, true  },
};

const int NUM_INPUTS = sizeof(INPUTS) / sizeof(INPUTS[0]);
const int NUM_OUTPUTS = sizeof(OUTPUTS) / sizeof(OUTPUTS[0]);

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

  setupNetwork();
}

// ── Loop ──────────────────────────────────────────────────────
void loop()
{
  loopNetwork();
  plcLoop();
  delay(50);
}
