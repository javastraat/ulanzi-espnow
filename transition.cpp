// transition.cpp — Screen transition animations between display modes.
//
// Effects that only need the outgoing snapshot (easy, no flash):
//   Fade, Wipe, Slide, Blink, Reload, Curtain, Ripple
//
// Effects that need both outgoing and incoming frames:
//   Crossfade — uses a one-frame silent pre-render to capture the new content
//   (loopDisplay renders the new content at brightness=0 for ~16 ms, imperceptible).
#include "transition.h"
#include "globals.h"
#include "display.h"

// ── Config globals (NVS-backed) ───────────────────────────────────────────────
uint8_t  transEffect  = TRANS_FADE;
uint16_t transSpeedMs = 400;

// ── State ─────────────────────────────────────────────────────────────────────
static CRGB          _snap[NUM_LEDS];   // outgoing frame
static CRGB          _snap2[NUM_LEDS];  // incoming frame (crossfade only)
static bool          _active       = false;
static unsigned long _startMs      = 0;
static uint8_t       _effectNow    = TRANS_FADE;

// Crossfade two-frame capture flags
static bool          _captureSkip  = false;  // skip one frame (let loopDisplay render silently)
static bool          _captureNext  = false;  // capture leds[] on next loopTransition() entry

// ── Helpers ───────────────────────────────────────────────────────────────────

static inline CRGB _sp(int x, int y) {
  int idx = (y % 2 == 0) ? y * MATRIX_WIDTH + x : (y + 1) * MATRIX_WIDTH - 1 - x;
  return _snap[idx];
}

// ── Public API ────────────────────────────────────────────────────────────────

void transitionBegin() {
  if (transEffect == TRANS_NONE) return;
  if (_active) return;  // previous transition still running — skip

  memcpy(_snap, leds, sizeof(CRGB) * NUM_LEDS);

  _effectNow = transEffect;
  if (_effectNow == TRANS_RANDOM)
    _effectNow = (uint8_t)((millis() % 8) + 1);  // 1–8

  _captureSkip = (_effectNow == TRANS_CROSSFADE);
  _captureNext = false;
  _active      = true;
  _startMs     = millis();
}

bool loopTransition() {
  if (!_active) return false;

  // ── Crossfade two-frame capture ───────────────────────────────────────────
  if (_captureSkip) {
    // Suppress this frame: render the new content invisible so we can capture it.
    _captureSkip = false;
    _captureNext = true;
    FastLED.setBrightness(0);
    return false;  // loopDisplay will render the new content (invisible)
  }
  if (_captureNext) {
    // loopDisplay just wrote new content to leds[] — capture it, restore brightness.
    memcpy(_snap2, leds, sizeof(CRGB) * NUM_LEDS);
    _captureNext = false;
    FastLED.setBrightness(currentBrightness);
    _startMs = millis();  // animation timer starts now
    // fall through to animation (p≈0)
  }

  // ── Animation ─────────────────────────────────────────────────────────────
  float p = (float)(millis() - _startMs) / (float)max((uint16_t)1, transSpeedMs);
  if (p >= 1.0f) {
    _active = false;
    FastLED.clear();
    FastLED.show();
    return false;
  }

  switch (_effectNow) {

    // ── Fade ─────────────────────────────────────────────────────────────────
    case TRANS_FADE: {
      uint8_t dim = (uint8_t)(p * 255.0f);
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = _snap[i];
        leds[i].fadeToBlackBy(dim);
      }
      FastLED.show();
      break;
    }

    // ── Wipe (left → right) ───────────────────────────────────────────────────
    case TRANS_WIPE: {
      int wipeX = (int)(p * (MATRIX_WIDTH + 1));
      for (int x = 0; x < MATRIX_WIDTH; x++)
        for (int y = 0; y < MATRIX_HEIGHT; y++)
          setLED(x, y, x < wipeX ? CRGB::Black : _sp(x, y));
      FastLED.show();
      break;
    }

    // ── Slide (old exits left) ────────────────────────────────────────────────
    case TRANS_SLIDE: {
      int shift = (int)(p * MATRIX_WIDTH);
      FastLED.clear();
      for (int x = 0; x < MATRIX_WIDTH - shift; x++)
        for (int y = 0; y < MATRIX_HEIGHT; y++)
          setLED(x, y, _sp(x + shift, y));
      FastLED.show();
      break;
    }

    // ── Blink (2 flashes then black) ──────────────────────────────────────────
    case TRANS_BLINK: {
      const float blinkEnd = 0.8f;
      bool showOld = false;
      if (p < blinkEnd) {
        float cycleLen = blinkEnd / 4.0f;  // 2 full cycles = 4 half-cycles
        int   step     = (int)(p / cycleLen);
        showOld = (step % 2 == 0);
      }
      if (showOld)
        memcpy(leds, _snap, sizeof(CRGB) * NUM_LEDS);
      else
        FastLED.clear();
      FastLED.show();
      break;
    }

    // ── Reload (old exits right) ──────────────────────────────────────────────
    case TRANS_RELOAD: {
      int shift = (int)(p * MATRIX_WIDTH);
      FastLED.clear();
      for (int x = shift; x < MATRIX_WIDTH; x++)
        for (int y = 0; y < MATRIX_HEIGHT; y++)
          setLED(x, y, _sp(x - shift, y));
      FastLED.show();
      break;
    }

    // ── Curtain (columns close from edges toward centre) ─────────────────────
    case TRANS_CURTAIN: {
      // At p=0 all visible; at p=1 only centre column visible → fully black
      int halfW     = MATRIX_WIDTH / 2;
      int threshold = (int)((1.0f - p) * halfW);
      for (int x = 0; x < MATRIX_WIDTH; x++) {
        int distFromCentre = abs(x - halfW);
        for (int y = 0; y < MATRIX_HEIGHT; y++)
          setLED(x, y, distFromCentre < threshold ? _sp(x, y) : CRGB::Black);
      }
      FastLED.show();
      break;
    }

    // ── Ripple (checkerboard dissolve) ────────────────────────────────────────
    case TRANS_RIPPLE: {
      // Even-sum pixels fade first (p 0→0.5), odd-sum pixels fade second (p 0.5→1)
      for (int x = 0; x < MATRIX_WIDTH; x++) {
        for (int y = 0; y < MATRIX_HEIGHT; y++) {
          bool isEven = ((x + y) % 2 == 0);
          float threshold = isEven ? 0.35f : 0.65f;
          CRGB c = _sp(x, y);
          if (p > threshold) {
            float local = constrain((p - threshold) / 0.3f, 0.0f, 1.0f);
            c.fadeToBlackBy((uint8_t)(local * 255));
          }
          setLED(x, y, c);
        }
      }
      FastLED.show();
      break;
    }

    // ── Crossfade (pixel lerp old→new) ────────────────────────────────────────
    case TRANS_CROSSFADE: {
      uint8_t mix = (uint8_t)(p * 255.0f);
      for (int i = 0; i < NUM_LEDS; i++)
        leds[i] = _snap[i].lerp8(_snap2[i], mix);
      FastLED.show();
      break;
    }
  }

  return true;
}
