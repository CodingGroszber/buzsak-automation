#pragma once

// ── Firmware version ─────────────────────────────────────────
#define FIRMWARE_VERSION "v4.0"

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
