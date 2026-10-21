#pragma once

// One PLC scan cycle — called every iteration of loop().
// Add all ladder / sequential logic here.
void plcLoop();

void TriggerRefill(bool trigger);
int plcSwitchMode();
bool refillButtonIsHigh();
bool refillIsActive();
