#include <Arduino.h>
#include "io_config.h"
#include "logic.h"

// ── Output indices (must match OUTPUTS[] order in main.cpp) ───
static const int IDX_WELL_PUMP = 0;  // Q0_0 — Finder relay coil
static const int IDX_TANK_PUMP = 1;  // Q0_1 — Finder relay coil
static const int IDX_SWITCH_LED = 2; // Q0_2 — 3-state switch LED

// ── Input indices (must match INPUTS[] order in main.cpp) ─────
static const int IDX_RIGHT_SW = 0; // I0_0  LOW = right position active
static const int IDX_LEFT_SW = 1;  // I0_1  LOW = left  position active

// ── Interval timer state ──────────────────────────────────────
static unsigned long wellPumpTimer = 0;
static unsigned long tankPumpTimer = 0;
static unsigned long ledTimer = 0;
static bool wellPumpOn = false;
static bool tankPumpOn = false;
static bool ledOn = false;

// ── PLC scan cycle (called every iteration of loop()) ─────────
void plcLoop()
{
    unsigned long now = millis();

    bool rightSw = (digitalRead(INPUTS[IDX_RIGHT_SW].pin) == LOW);
    bool leftSw = (digitalRead(INPUTS[IDX_LEFT_SW].pin) == LOW);

    // ── NEUTRAL (middle) — everything off ──────────────────────
    if (!rightSw && !leftSw)
    {
        digitalWrite(OUTPUTS[IDX_WELL_PUMP].pin, LOW);
        digitalWrite(OUTPUTS[IDX_TANK_PUMP].pin, LOW);
        digitalWrite(OUTPUTS[IDX_SWITCH_LED].pin, LOW);
        // Reset timers so pumps start from a defined OFF state
        // on the next mode transition
        wellPumpOn = false;
        wellPumpTimer = now;
        tankPumpOn = false;
        tankPumpTimer = now;
        ledOn = false;
        ledTimer = now;
        return;
    }

    // ── RIGHT — LED solid, well 3 s cycle, tank 5 s cycle ──────
    if (rightSw)
    {
        digitalWrite(OUTPUTS[IDX_SWITCH_LED].pin, HIGH);

        if (now - wellPumpTimer >= 3000UL)
        {
            wellPumpOn = !wellPumpOn;
            wellPumpTimer = now;
            digitalWrite(OUTPUTS[IDX_WELL_PUMP].pin, wellPumpOn ? HIGH : LOW);
            Serial.println(wellPumpOn ? "[WELL] ON" : "[WELL] OFF");
        }

        if (now - tankPumpTimer >= 5000UL)
        {
            tankPumpOn = !tankPumpOn;
            tankPumpTimer = now;
            digitalWrite(OUTPUTS[IDX_TANK_PUMP].pin, tankPumpOn ? HIGH : LOW);
            Serial.println(tankPumpOn ? "[TANK] ON" : "[TANK] OFF");
        }
        return;
    }

    // ── LEFT — LED blinks 0.5 s, well pump ON, tank pump OFF ───
    digitalWrite(OUTPUTS[IDX_WELL_PUMP].pin, HIGH);
    digitalWrite(OUTPUTS[IDX_TANK_PUMP].pin, LOW);

    if (now - ledTimer >= 500UL)
    {
        ledOn = !ledOn;
        ledTimer = now;
        digitalWrite(OUTPUTS[IDX_SWITCH_LED].pin, ledOn ? HIGH : LOW);
    }

    // Reset pump timers so they start from OFF when switching to RIGHT
    wellPumpOn = false;
    wellPumpTimer = now;
    tankPumpOn = false;
    tankPumpTimer = now;
}
