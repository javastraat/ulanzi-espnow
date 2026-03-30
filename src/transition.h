#pragma once
#include <Arduino.h>

// ── Screen transition effects ────────────────────────────────────────────────
// Triggered on every built-in mode change and app-phase boundary.
// transitionBegin() snapshots the current leds[] state; loopTransition() animates
// the outgoing frame away over transSpeedMs, then the new content renders normally.

#define TRANS_NONE      0   // instant switch (no animation)
#define TRANS_FADE      1   // fade old content to black
#define TRANS_WIPE      2   // columns wipe left→right to black
#define TRANS_SLIDE     3   // old content slides out to the left
#define TRANS_BLINK     4   // blink 2× then black
#define TRANS_RELOAD    5   // old content slides out to the right
#define TRANS_CURTAIN   6   // columns close from edges to centre
#define TRANS_RIPPLE    7   // checkerboard dissolve to black
#define TRANS_CROSSFADE 8   // pixel-by-pixel lerp old→new
#define TRANS_PUSHDOWN  9   // new slides in from top, old pushed out bottom
#define TRANS_RANDOM    10  // random choice from 1–9

extern uint8_t  transEffect;    // NVS key "trans_eff"  default TRANS_FADE
extern uint16_t transSpeedMs;   // NVS key "trans_spd"  default 400

// Call once just before changing displayMode / starting a new app.
// Snapshots the current frame; does nothing when transEffect==TRANS_NONE.
void transitionBegin();

// Returns true while an animation is running.
// Call at the top of loopDisplay(); return early when true.
bool loopTransition();
