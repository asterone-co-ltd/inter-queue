/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#if !defined(_DEBUG_HPP_)
#define _DEBUG_HPP_

#include <cstdarg>
#include <cstdio>

namespace IQ {
#if defined(IQ_DEBUG_BUILD)
#define DPRINTF(fmt, ...) IQ::debug_printf(fmt, ##__VA_ARGS__)
#else
#define DPRINTF(fmt, ...) ((void)0)
#endif

#if defined(IQ_DEBUG_BUILD)
#define EPRINTF(fmt, ...) IQ::debug_printf(fmt, ##__VA_ARGS__)
#else
#define EPRINTF(fmt, ...) ((void)0)
#endif

void debug_printf(const char *fmt, ...);
} // namespace IQ
#endif /* _DEBUG_HPP_ */
