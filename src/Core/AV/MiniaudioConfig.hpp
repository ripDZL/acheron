#pragma once

// Single source of truth for miniaudio build configuration.
//
// CRITICAL: miniaudio's MA_NO_* options change struct layouts (ma_device,
// ma_context, etc.), so every translation unit that includes "miniaudio.h"
// MUST see the identical set of options, and exactly ONE translation unit may
// define MINIAUDIO_IMPLEMENTATION. Always include this header instead of
// including "miniaudio.h" directly.
//
// Config rationale:
//  - Engine + decoding + resource manager + node graph are ENABLED so the
//    notification-sound path (SoundManager, ma_engine_play_sound) works.
//  - The low-level device/capture path (MiniaudioAudioBackend) only uses
//    ma_device/ma_context, which are unaffected by enabling the higher-level
//    features, so this is safe for the voice path.
//  - Encoding and waveform generation stay disabled (unused, saves size).

#define MA_NO_ENCODING
#define MA_NO_GENERATION

#include "miniaudio.h"
