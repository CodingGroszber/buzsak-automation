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
static unsigned long ledTimer = 0;
static bool wellPumpOn = false;
static bool tankPumpOn = false;
static bool ledOn = false;
static bool refillButtonHigh = false;
static bool refillRequestPending = false;
static bool refillActive = false;
static unsigned long refillButtonStartedAt = 0;
static int previousSwitchMode = -2;

int plcSwitchMode()
{
    bool rightSw = (digitalRead(INPUTS[IDX_RIGHT_SW].pin) == LOW);
    bool leftSw = (digitalRead(INPUTS[IDX_LEFT_SW].pin) == LOW);

    if (rightSw && leftSw)
        return -1;
    if (rightSw)
        return 1;
    if (leftSw)
        return 2;
    return 0;
}

void TriggerRefill(bool trigger)
{
    unsigned long now = millis();
    if (refillButtonHigh && now - refillButtonStartedAt >= 500UL)
        refillButtonHigh = false;

    if (trigger && !refillButtonHigh)
    {
        refillButtonHigh = true;
        refillButtonStartedAt = now;
        refillRequestPending = true;
    }
    if (trigger)
        return;

    if (refillRequestPending)
    {
        refillRequestPending = false;
        if (plcSwitchMode() == 1 && !refillActive)
        {
            refillActive = true;
            Serial.println("[REFILL] started");
        }
    }

    if (!refillActive)
        return;

    if (plcSwitchMode() != 1)
    {
        refillActive = false;
        return;
    }

    uint32_t mv = analogReadMilliVolts(ANALOG_INPUTS[IDX_WATER_LEVEL].pin);
    float liters = waterLevelMvToLiters(mv);
    if (mv == 0)
    {
        refillActive = false;
        wellPumpOn = false;
        digitalWrite(OUTPUTS[IDX_WELL_PUMP].pin, LOW);
        Serial.println("[REFILL] stopped: water-level signal missing");
        return;
    }

    if (liters >= WATER_LEVEL_HIGH_L)
    {
        refillActive = false;
        wellPumpOn = false;
        digitalWrite(OUTPUTS[IDX_WELL_PUMP].pin, LOW);
        Serial.printf("[REFILL] complete  %.0f L  %lu mV\n", liters, static_cast<unsigned long>(mv));
        return;
    }

    wellPumpOn = true;
    digitalWrite(OUTPUTS[IDX_WELL_PUMP].pin, HIGH);
}

bool refillButtonIsHigh()
{
    if (refillButtonHigh && millis() - refillButtonStartedAt >= 500UL)
        refillButtonHigh = false;
    return refillButtonHigh;
}

bool refillIsActive()
{
    return refillActive;
}

// ── PLC scan cycle (called every iteration of loop()) ─────────
void plcLoop()
{
    unsigned long now = millis();

    int mode = plcSwitchMode();
    if (mode != previousSwitchMode)
    {
        if (previousSwitchMode != -2 && refillActive)
        {
            refillActive = false;
            Serial.println("[REFILL] cancelled by selector");
        }
        previousSwitchMode = mode;
        ledOn = false;
        ledTimer = now;
    }

    TriggerRefill(false);

    // Neutral and contradictory selector contacts both fail safe to OFF.
    if (mode == 0 || mode == -1)
    {
        digitalWrite(OUTPUTS[IDX_WELL_PUMP].pin, LOW);
        digitalWrite(OUTPUTS[IDX_TANK_PUMP].pin, LOW);
        digitalWrite(OUTPUTS[IDX_SWITCH_LED].pin, LOW);
        wellPumpOn = false;
        tankPumpOn = false;
        ledOn = false;
        return;
    }

    if (mode == 1)
    {
        digitalWrite(OUTPUTS[IDX_SWITCH_LED].pin, HIGH);

        if (!refillActive)
        {
            uint32_t wMv = analogReadMilliVolts(ANALOG_INPUTS[IDX_WATER_LEVEL].pin);
            float wLiters = waterLevelMvToLiters(wMv);
            bool desiredWell = false;
            if (wMv > 0)
            {
                if (wellPumpOn)
                    desiredWell = (wLiters < WATER_LEVEL_HIGH_L);
                else
                    desiredWell = (wLiters <= WATER_LEVEL_LOW_L);
            }

            if (desiredWell != wellPumpOn)
            {
                wellPumpOn = desiredWell;
                digitalWrite(OUTPUTS[IDX_WELL_PUMP].pin, wellPumpOn ? HIGH : LOW);
                Serial.printf("[WELL] %s  %.0f L  %lu mV\n",
                              wellPumpOn ? "ON" : "OFF", wLiters, static_cast<unsigned long>(wMv));
            }
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

    refillActive = false;
    digitalWrite(OUTPUTS[IDX_WELL_PUMP].pin, HIGH);
    digitalWrite(OUTPUTS[IDX_TANK_PUMP].pin, LOW);

    if (now - ledTimer >= 500UL)
    {
        ledOn = !ledOn;
        ledTimer = now;
        digitalWrite(OUTPUTS[IDX_SWITCH_LED].pin, ledOn ? HIGH : LOW);
    }

    wellPumpOn = false;
    tankPumpOn = false;
}
