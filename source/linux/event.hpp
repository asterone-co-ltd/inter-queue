/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#if !defined(_EVENT_HPP_)
#define _EVENT_HPP_

#include "common.hpp"
#include "interq.h"
#include <atomic>
#include <climits>
#include <cstdint>
#include <errno.h>
#include <linux/futex.h>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>

namespace IQ
{
  class Event
  {
    std::atomic<uint32_t>* event_;

    uint32_t seq_;

  public:
    Event()
      : event_{nullptr}
      , seq_{0}
    {
      static_assert(sizeof(std::atomic<uint32_t>) == 4);
      static_assert(alignof(std::atomic<uint32_t>) == 4);
    }

    ~Event()
    {
      close();
    }

    IQ_RESULT create(std::atomic<uint32_t>* event)
    {
      if (event == nullptr)
      {
        return IQ_ERROR;
      }
      event_ = event;
      event_->store(0, std::memory_order_release);
      seq_ = event_->load(std::memory_order_acquire);
      return IQ_SUCCESS;
    }

    IQ_RESULT open(std::atomic<uint32_t>* event)
    {
      if (event == nullptr)
      {
        return IQ_ERROR;
      }
      event_ = event;
      seq_   = event_->load(std::memory_order_acquire);
      return IQ_SUCCESS;
    }

    void close()
    {
      event_ = nullptr;
    }

    bool set()
    {
      if (event_ == nullptr)
      {
        return false;
      }
      event_->fetch_add(1, std::memory_order_release);
      ::syscall(SYS_futex, event_, FUTEX_WAKE, 1, nullptr, nullptr, 0);
      return true;
    }

    IQ_RESULT wait_for_event(uint32_t tmout)
    {
      if (event_ == nullptr)
      {
        return IQ_ERROR;
      }
      while (1)
      {
        auto seq = event_->load(std::memory_order_acquire);
        if (seq_ != seq)
        {
          seq_ = seq;
          return IQ_SUCCESS;
        }

        auto ts     = to_timespec(tmout);
        long result = ::syscall(SYS_futex, event_, FUTEX_WAIT, seq, &ts, nullptr, 0);
        if (result == 0)
        {
          continue;
        }
        result = errno;
        if (result == EINTR || result == EAGAIN)
        {
          continue;
        }
        if (result == ETIMEDOUT)
        {
          return IQ_TMOUT;
        }
        return IQ_ERROR;
      }
    }
  };
} /* namespace IQ */
#endif /* _EVENT_HPP_ */
