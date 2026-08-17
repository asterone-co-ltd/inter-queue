/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#if !defined(_COMMON_HPP_)
#define _COMMON_HPP_

#include "interq.h"
#include <string>

#if defined(IQ_PLATFORM_LINUX)
#include <sys/time.h>
#endif

namespace IQ
{
  uint32_t get_build_hash();

#if defined(IQ_PLATFORM_LINUX)
  constexpr int InvalidFD = -1;

  inline struct timespec to_timespec(uint32_t tm)
  {
    struct timespec ts;
    ts.tv_sec  = tm / 1000;
    ts.tv_nsec = (tm % 1000) * 1000000;
    return ts;
  }
#endif
} // namespace IQ
#endif /* _COMMON_HPP_ */
