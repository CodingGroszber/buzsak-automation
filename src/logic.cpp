#include <Arduino.h>
#include "io_config.h"
#include "logic.h"

// Button → Relay mirror (last state tracking)
static int lastButtonState = -1;

// ── PLC scan cycle (called every iteration of loop()) ─────────
// Add all ladder / sequential logic here.
void plcLoop()
{
    // Button → Relay mirror logic
    int buttonState = digitalRead(INPUTS[0].pin); // I0_0 = button
    if (buttonState != lastButtonState)
    {
        lastButtonState = buttonState;
        digitalWrite(OUTPUTS[0].pin, buttonState == HIGH ? HIGH : LOW); // Q0_0 = relay
        Serial.println(buttonState == HIGH ? "Relay CLOSED" : "Relay OPEN");
    }
}
