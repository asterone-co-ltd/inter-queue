/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#if !defined(_SHAREQUE_HPP_)
#define _SHAREQUE_HPP_

#include "event.hpp"
#include "interq.h"
#include "semaphore.hpp"
#include "sharemem.hpp"
#include <cstdint>
#include <string>

namespace IQ {
class ShareQue {
  mutable ShareMem mem_;

  Semaphore sem_;

  Event evt_;

public:
  ShareQue();

  ~ShareQue();

  iq_result_t create(const std::string &name, uint64_t size);

  iq_result_t open(const std::string &name);

  void close();

  int push(const void *data, int size, bool drop_old = false);

  int pop(void *buff, int size);

  bool is_valid() const { return mem_.is_valid(); }

  int size() const;

  int count() const;

  int remain() const;

  int ref_count() const;

  iq_result_t wait_for_event(uint32_t tmout);

  static void clean(const std::string &name);
};
} /* namespace IQ */
#endif /* _SHAREQUE_HPP_ */
