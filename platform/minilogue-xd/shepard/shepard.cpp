/*
 * File: shepard.cpp
 *
 * Shepard tone user oscillator for minilogue xd.
 */

#include "userosc.h"

namespace {

constexpr uint8_t k_max_partials = 10;

struct ShepardParams {
  float motion;      // -1..1 (sign: direction, magnitude: speed)
  float width;       // 0..1
  uint8_t octaves;   // 3..10
  float detune;      // 0..1
  float gain;        // 0..1
  float waveform;    // 0..1 (sine->tri->saw->square)
  float shape;       // 0..1
  float shiftshape;  // 0..1
};

struct ShepardState {
  float phase[k_max_partials];
  float glide;       // semitone offset in [-12, 12)
  float lfoz;
};

static ShepardParams s_params;
static ShepardState s_state;

static inline float note_to_w0f(float note) {
  const float clipped = clipminmaxf(0.f, note, 151.f);
  const uint8_t note_i = (uint8_t)clipped;
  const float frac = clipped - note_i;
  const uint8_t mod = (uint8_t)(clip01f(frac) * 255.f);
  return osc_w0f_for_note(note_i, mod);
}

static inline float shepard_weight(float note, float center_note, float span_st) {
  const float lo = center_note - 0.5f * span_st;
  const float hi = center_note + 0.5f * span_st;
  const float x = (note - lo) / (hi - lo);
  if (x <= 0.f || x >= 1.f) {
    return 0.f;
  }

  // Raised-cosine bell with gamma shaping:
  // gamma < 1 widens the bell, gamma > 1 narrows it.
  constexpr float k_bell_gamma = 0.75f;
  const float w = 0.5f - 0.5f * osc_cosf(x);
  return si_powf(w, k_bell_gamma);
}

static inline float wave_triangle(float phase) {
  const float p = phase - (uint32_t)phase;
  return 1.f - 4.f * si_fabsf(p - 0.5f);
}

static inline float wave_morphed(float phase, float waveform) {
  const float m = clip01f(waveform) * 3.f;
  const uint8_t seg = clipmaxu32((uint32_t)m, 2);
  const float fr = m - seg;

  const float s0 = osc_sinf(phase);
  const float s1 = wave_triangle(phase);
  const float s2 = osc_sawf(phase);
  const float s3 = osc_sqrf(phase);

  switch (seg) {
    case 0:
      return linintf(fr, s0, s1);
    case 1:
      return linintf(fr, s1, s2);
    default:
      return linintf(fr, s2, s3);
  }
}

}  // namespace

void OSC_INIT(uint32_t platform, uint32_t api) {
  (void)platform;
  (void)api;

  // Classic Shepard tone defaults: pure sine stack with gentle upward motion.
  s_params.motion = 0.28f;
  s_params.width = 0.75f;
  s_params.octaves = 8;
  s_params.detune = 0.f;
  s_params.gain = 0.68f;
  s_params.waveform = 0.f;
  s_params.shape = 0.5f;
  s_params.shiftshape = 0.5f;

  s_state.glide = 0.f;
  s_state.lfoz = 0.f;
  for (uint8_t i = 0; i < k_max_partials; ++i) {
    s_state.phase[i] = 0.f;
  }
}

void OSC_CYCLE(const user_osc_param_t *const params, int32_t *yn, const uint32_t frames) {
  const float base_note = ((params->pitch >> 8) & 0xFF) + ((params->pitch & 0xFF) * (1.f / 255.f));
  const float lfo = q31_to_f32(params->shape_lfo);
  float lfoz = s_state.lfoz;
  const float lfo_inc = (lfo - lfoz) / frames;

  // Motion combines direction and speed in one bipolar control.
  const float rate_stps = s_params.motion * 18.f;  // semitones per second
  const float glide_inc = rate_stps * k_samplerate_recipf;

  const uint8_t partials = clipminmaxu32(3, s_params.octaves, k_max_partials);
  const float partial_center = 0.5f * (partials - 1);

  // Shape + LFO moves the spectral focus; Width controls bell span.
  // Increase base/span values for gentler entry/exit of top/bottom partials.
  const float center_note = base_note + (s_params.shape - 0.5f) * 24.f + lfoz * 12.f;
  const float span_st = 18.f + 66.f * s_params.width;
  const float detune_amt = 0.015f * s_params.detune;

  q31_t *y = (q31_t *)yn;
  const q31_t *y_end = y + frames;

  for (; y != y_end; ++y) {
    float sample = 0.f;
    float norm = 0.f;

    for (uint8_t i = 0; i < partials; ++i) {
      const float octave_offset = (i - partial_center) * 12.f;
      const float note = base_note + s_state.glide + octave_offset;
      const float w = shepard_weight(note, center_note, span_st);
      if (w <= 0.f) {
        continue;
      }

      float w0 = note_to_w0f(note);
      const float detune = ((float)i - partial_center) / partial_center;
      w0 *= 1.f + detune * detune_amt;

      s_state.phase[i] += w0;
      s_state.phase[i] -= (uint32_t)s_state.phase[i];

      sample += w * wave_morphed(s_state.phase[i], s_params.waveform);
      norm += w;
    }

    if (norm > 1e-6f) {
      sample *= 1.f / norm;
    }

    const float gain = 0.15f + 0.85f * s_params.gain;
    sample *= gain * (0.7f + 0.6f * s_params.shiftshape);
    sample = osc_softclipf(0.125f, sample);
    *y = f32_to_q31(sample);

    s_state.glide += glide_inc;
    if (s_state.glide >= 12.f) {
      s_state.glide -= 12.f;
    } else if (s_state.glide < -12.f) {
      s_state.glide += 12.f;
    }
    lfoz += lfo_inc;
  }

  s_state.lfoz = lfoz;
}

void OSC_NOTEON(const user_osc_param_t *const params) {
  (void)params;
  s_state.glide = 0.f;
}

void OSC_NOTEOFF(const user_osc_param_t *const params) {
  (void)params;
}

void OSC_PARAM(uint16_t index, uint16_t value) {
  switch (index) {
    case k_user_osc_param_id1: {
      // Bipolar percent arrives as 0..200 where 100 means 0%.
      const int32_t centered = clipminmaxi32(0, value, 200) - 100;
      s_params.motion = 0.01f * centered;
    } break;

    case k_user_osc_param_id2:
      s_params.width = clip01f(value * 0.01f);
      break;

    case k_user_osc_param_id3:
      s_params.octaves = 3 + (value & 0x7);
      break;

    case k_user_osc_param_id4:
      s_params.detune = clip01f(value * 0.01f);
      break;

    case k_user_osc_param_id5:
      s_params.gain = clip01f(value * 0.01f);
      break;

    case k_user_osc_param_id6:
      // Waveform morph map:
      //   0..33  : sine -> triangle
      //   34..66 : triangle -> saw
      //   67..100: saw -> square
      s_params.waveform = clip01f(value * 0.01f);
      break;

    case k_user_osc_param_shape:
      s_params.shape = param_val_to_f32(value);
      break;

    case k_user_osc_param_shiftshape:
      s_params.shiftshape = param_val_to_f32(value);
      break;

    default:
      break;
  }
}
