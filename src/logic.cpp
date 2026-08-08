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

// ── Analog input indices (must match ANALOG_INPUTS[] in main.cpp) ─
static const int IDX_PRESSURE = 0;    // I0_7 — PT506 pressure sensor
static const int IDX_WATER_LEVEL = 1; // I0_8 — TL-136 water level sensor

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

    // ── RIGHT — AUTO mode: pressure-based tank pump control ──────
    // Hysteresis: pump ON below 3 bar, pump OFF at 4 bar; rupture guard < 0.1 bar.
    if (rightSw)
    {
        digitalWrite(OUTPUTS[IDX_SWITCH_LED].pin, HIGH);

        // Well pump hysteresis on tank level:
        // - start when level is low (<= WATER_LEVEL_LOW_L)
        // - stop when high threshold is reached (>= WATER_LEVEL_HIGH_L)
        uint32_t wMv = analogReadMilliVolts(ANALOG_INPUTS[IDX_WATER_LEVEL].pin);
        float wLiters = waterLevelMvToLiters(wMv);
        bool desiredWell;
        if (wellPumpOn)
            desiredWell = (wLiters < static_cast<float>(WATER_LEVEL_HIGH_L)); // running: stop at high level
        else
            desiredWell = (wLiters <= WATER_LEVEL_CAL_POINT_L); // idle: restart at low level

        if (desiredWell != wellPumpOn)
        {
            wellPumpOn = desiredWell;
            digitalWrite(OUTPUTS[IDX_WELL_PUMP].pin, wellPumpOn ? HIGH : LOW);
            Serial.printf("[WELL] %s  %.0f L  %lu mV\n",
                          wellPumpOn ? "ON" : "OFF", wLiters, static_cast<unsigned long>(wMv));
        }

        // Tank pump hysteresis on pressure:
        // - start when pressure is low (< PRESSURE_START_BAR)
        // - stop when pressure reaches high threshold (>= PRESSURE_STOP_BAR)
        // - force OFF when pressure is invalid (<= PRESSURE_MIN_VALID_BAR)
        uint32_t pMv = analogReadMilliVolts(ANALOG_INPUTS[IDX_PRESSURE].pin);
        float pBar = pressureMvToBar(pMv);
        bool valid = (pBar > PRESSURE_MIN_VALID_BAR); // above 0.1 bar — no rupture/fault
        bool desired;
        if (tankPumpOn)
            desired = valid && (pBar < PRESSURE_STOP_BAR); // running: stop at 4 bar
        else
            desired = valid && (pBar < PRESSURE_START_BAR); // idle: restart below 3 bar

        if (desired != tankPumpOn)
        {
            tankPumpOn = desired;
            digitalWrite(OUTPUTS[IDX_TANK_PUMP].pin, tankPumpOn ? HIGH : LOW);
            Serial.printf("[TANK] %s  %.2f bar\n",
                          tankPumpOn ? "ON" : "OFF", pBar);
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
