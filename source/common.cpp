/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#include "common.hpp"

#include "version.hpp"

namespace IQ {
consteval uint32_t fnv1a_32(const char *s) {
  uint32_t h = 2166136261u;
  for (; *s; ++s) {
    h ^= static_cast<uint8_t>(*s);
    h *= 16777619u;
  }
  return h;
}

consteval uint32_t xorshift32(uint32_t x) {
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x;
}

consteval uint32_t build_random_u32() {
  constexpr uint32_t Seed = fnv1a_32(Version);
  return xorshift32(Seed);
}

constexpr uint32_t BuildHash = build_random_u32();

uint32_t get_build_hash() { return BuildHash; }
} // namespace IQ
