// Shared helpers for decoding BC1/BC2/BC3 (DXT1/3/5) blocks on the GPU.
// Used on devices that do not support textureCompressionBC (most mobile GPUs).

vec3 dxt_rgb565(uint c) {
  return vec3(float((c >> 11) & 0x1Fu) / 31.0,
              float((c >>  5) & 0x3Fu) / 63.0,
              float( c        & 0x1Fu) / 31.0);
}

// colors:  c0 | (c1 << 16)
// indices: 2 bits per texel, row-major, LSB first
// allowAlpha: BC1 punch-through mode (c0 <= c1). BC2/BC3 always use 4-color mode.
vec4 dxt_decode_color(uint colors, uint indices, uint texel, bool allowAlpha) {
  uint c0 = colors & 0xFFFFu;
  uint c1 = colors >> 16;

  vec3 p0 = dxt_rgb565(c0);
  vec3 p1 = dxt_rgb565(c1);

  uint idx = (indices >> (2u * texel)) & 3u;

  if (idx == 0u)
    return vec4(p0, 1.0);
  if (idx == 1u)
    return vec4(p1, 1.0);

  if (c0 > c1 || !allowAlpha) {
    return idx == 2u
      ? vec4((2.0 * p0 + p1) / 3.0, 1.0)
      : vec4((p0 + 2.0 * p1) / 3.0, 1.0);
  }

  return idx == 2u
    ? vec4((p0 + p1) * 0.5, 1.0)
    : vec4(0.0);
}

// 3 bits at bit position p of a 64-bit value stored as (lo, hi)
uint dxt_get_bits3(uvec2 v, uint p) {
  uint res = p < 32u ? (v.x >> p) : (v.y >> (p - 32u));

  // Index straddles the 32-bit boundary
  if (p > 29u && p < 32u)
    res |= v.y << (32u - p);

  return res & 7u;
}

// BC3 alpha block: v.x bits 0-7 = a0, 8-15 = a1, remaining 48 bits = 3-bit indices
float dxt5_decode_alpha(uvec2 v, uint texel) {
  float a0 = float(v.x & 0xFFu);
  float a1 = float((v.x >> 8u) & 0xFFu);

  uint idx = dxt_get_bits3(v, 16u + 3u * texel);

  float a;

  if (idx == 0u) {
    a = a0;
  } else if (idx == 1u) {
    a = a1;
  } else if (a0 > a1) {
    a = (float(8u - idx) * a0 + float(idx - 1u) * a1) / 7.0;
  } else if (idx < 6u) {
    a = (float(6u - idx) * a0 + float(idx - 1u) * a1) / 5.0;
  } else {
    a = idx == 6u ? 0.0 : 255.0;
  }

  return a / 255.0;
}
