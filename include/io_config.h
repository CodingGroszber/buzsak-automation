#pragma once

// ── Firmware version ─────────────────────────────────────────
#define FIRMWARE_VERSION "v4.1"

// ═════════════════════════════════════════════════════════════
//  IO CONFIGURATION — shared types and extern declarations
//
//  The INPUTS[] and OUTPUTS[] arrays are defined in main.cpp.
//  Edit them there to add/remove sensors and relays.
//
//  Available input pins:   I0_0, I0_1, I0_2, I0_3, I0_4, I0_5
//  Available output pins:  Q0_0, Q0_1, Q0_2, Q0_3
//  Output mode pins:       S0_24v (Q0_0), S1_24v (Q0_1),
//                          S2_24v (Q0_2), S3_24v (Q0_3)
//  Output mode values:     HIGH = 24 V,  LOW = 5 V
//
//  Set controllable=true to allow remote control from the app.
// ═════════════════════════════════════════════════════════════

struct IOInput
{
    const char *name;  // API / app key  e.g. "button"
    const char *label; // Human label    e.g. "Button Input"
    int pin;           // Hardware pin   e.g. I0_0
};

struct IOOutput
{
    const char *name;  // API / app key  e.g. "relay"
    const char *label; // Human label    e.g. "Relay"
    int pin;           // Hardware pin   e.g. Q0_0
    int modePin;       // Mode pin (-1 = none)
    int modeVal;       // HIGH = 24 V, LOW = 5 V
    bool controllable; // true = settable via POST /api/control
};

struct IOAnalog
{
    const char *name;  // API / app key  e.g. "pressure"
    const char *label; // Human label    e.g. "Pressure Sensor"
    int pin;           // ADC pin        e.g. I0_7_MA
};

extern IOInput INPUTS[];
extern IOOutput OUTPUTS[];
extern IOAnalog ANALOG_INPUTS[];
extern const int NUM_INPUTS;
extern const int NUM_OUTPUTS;
extern const int NUM_ANALOG_SENSORS;

// Returns the index of the "wifi_led" output, or -1 if not present
int wifiLedIndex();

// ═════════════════════════════════════════════════════════════
//  PRESSURE SENSOR CALIBRATION — I0.7 / AI0_0 (GPIO 34)
//
//  Sensor : TOSEASTARS PT506, 0–10 bar, 4–20 mA
//  Circuit: 470 Ω external shunt, IS board voltage divider 3.82
//  Reading: analogReadMilliVolts(I0_7) — mV at the ADC pin
//
//  Calibrated from two gauge reference points:
//    1 bar → 691 mV (~5.62 mA)   3 bar → 1083 mV (~8.79 mA)
//
//  V_adc_mV = (bar × 1.6 + 4) × 470 / 3.82
// ═════════════════════════════════════════════════════════════
constexpr uint32_t PRESSURE_0BAR_MV = 492;      // 4 mA live-zero
constexpr uint32_t PRESSURE_MIN_VALID_MV = 512; // ~0.1 bar — pipe-rupture / no-signal guard
constexpr uint32_t PRESSURE_1BAR_MV = 689;
constexpr uint32_t PRESSURE_2BAR_MV = 886;
constexpr uint32_t PRESSURE_3BAR_MV = 1083;
constexpr uint32_t PRESSURE_4BAR_MV = 1280;
constexpr uint32_t PRESSURE_5BAR_MV = 1476;

// Pressure control thresholds in engineering units for PLC logic.
constexpr float PRESSURE_MIN_VALID_BAR = 0.1f; // ~512 mV (rupture/no-signal guard)
constexpr float PRESSURE_START_BAR = 3.0f;     // ~1083 mV (restart threshold)
constexpr float PRESSURE_STOP_BAR = 4.0f;      // ~1280 mV (stop threshold)

// Convert a raw analogReadMilliVolts reading to bar (float)
inline float pressureMvToBar(uint32_t mv)
{
    return (mv * 3.82f / 470.0f - 4.0f) / 1.6f;
}

// ═════════════════════════════════════════════════════════════
//  WATER LEVEL CALIBRATION — I0.8 / AI0_1 (GPIO 33)
//
//  Current site calibration target:
//    ~2450 mV is observed at full tank; clamp conversion at 2400 mV
//    to keep output stable at 1000 L max.
// ═════════════════════════════════════════════════════════════
constexpr uint32_t WATER_LEVEL_UPPER_BOUND_MV = 2400; // mV at clamped full tank
constexpr uint32_t WATER_LEVEL_FULL_L = 1000;         // L at WATER_LEVEL_UPPER_BOUND_MV
constexpr uint32_t WATER_LEVEL_EMPTY_L = 0;           // L at 0 mV (linear map origin)

// Well pump hysteresis thresholds — already in liters, used directly against
// waterLevelMvToLiters() output (do not re-scale through the mV bound above).
constexpr float WATER_LEVEL_LOW_L = 300.0f;                                  // restart well pump at/below this level
constexpr float WATER_LEVEL_HIGH_L = static_cast<float>(WATER_LEVEL_FULL_L); // stop well pump at this level

inline float waterLevelMvToLiters(uint32_t mv)
{
    if (mv >= WATER_LEVEL_UPPER_BOUND_MV)
        return static_cast<float>(WATER_LEVEL_FULL_L);

    float liters = (static_cast<float>(mv) / static_cast<float>(WATER_LEVEL_UPPER_BOUND_MV)) * static_cast<float>(WATER_LEVEL_FULL_L);
    if (liters < static_cast<float>(WATER_LEVEL_EMPTY_L))
        return static_cast<float>(WATER_LEVEL_EMPTY_L);
    return liters;
}
