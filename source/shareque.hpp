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

namespace IQ
{
  class ShareQue
  {
    mutable ShareMem mem_;

    Semaphore sem_;

    Event evt_;

  public:
    ShareQue();

    ~ShareQue();

    IQ_RESULT create(const std::string& name, uint64_t size);

    IQ_RESULT open(const std::string& name);

    void close();

    int push(const void* data, int size, bool drop_old = false);

    int pop(void* buff, int size);

    bool is_valid() const
    {
      return mem_.is_valid();
    }

    int size() const;

    int count() const;

    int remain() const;

    int ref_count() const;

    IQ_RESULT wait_for_event(uint32_t tmout);

    static void clean(const std::string& name);
  };
} /* namespace IQ */
#endif /* _SHAREQUE_HPP_ */
