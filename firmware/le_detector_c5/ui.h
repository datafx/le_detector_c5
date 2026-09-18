// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "detector.h"

// Minimal working UI - border flash + status text. Colours, border
// thickness, and layout are cosmetic and get tuned later (decision #3);
// this is deliberately not the final design.
void uiInit();
void uiBootScreen();
void uiRender(const DetectorStatus& st, const char* phaseLabel);
