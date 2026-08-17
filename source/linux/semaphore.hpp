/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#if !defined(_SEMAPHORE_HPP_)
#define _SEMAPHORE_HPP_

#include "common.hpp"
#include "debug.hpp"
#include "interq.h"
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string>
#include <sys/stat.h>
#include <time.h>

namespace IQ
{

  class Semaphore
  {
    sem_t* sem_;

    std::string name_;

    std::atomic<bool> locked_;

  public:
    Semaphore()
      : sem_{SEM_FAILED}
      , name_{}
      , locked_{false}
    {
      DPRINTF(">> Semaphore::Semaphore\n");
    }

    ~Semaphore()
    {
      DPRINTF(">> Semaphore::~Semaphore\n");
      close();
    }

    IQ_RESULT create(const std::string& name)
    {
      DPRINTF(">> Semaphore::create\n");
      close();
      sem_ = ::sem_open(("/" + name + "_sem").c_str(), O_CREAT | O_EXCL, 0644, 1);
      if (sem_ == SEM_FAILED)
      {
        EPRINTF("%s\n", ::strerror(errno));
        return IQ_ERROR;
      }
      name_ = "/" + name + "_sem";
      return IQ_SUCCESS;
    }

    IQ_RESULT open(const std::string& name)
    {
      DPRINTF(">> Semaphore::open\n");
      close();
      sem_ = ::sem_open(("/" + name + "_sem").c_str(), 0);
      if (sem_ == SEM_FAILED)
      {
        EPRINTF("%s\n", ::strerror(errno));
        return IQ_ERROR;
      }
      name_ = "/" + name + "_sem";
      return IQ_SUCCESS;
    }

    void close(bool unlink = false)
    {
      unlock();
      if (sem_ != SEM_FAILED)
      {
        ::sem_close(sem_);
        sem_ = SEM_FAILED;
        DPRINTF(">> Semaphore::close\n");
      }
      if (unlink == true && name_.empty() == false)
      {
        ::sem_unlink(name_.c_str());
        DPRINTF(">> Semaphore::unlink\n");
      }
      name_.clear();
    }

    IQ_RESULT lock()
    {
      if (sem_ == SEM_FAILED)
      {
        return IQ_INVALID_HANDLE;
      }
      if (locked_.load(std::memory_order_acquire) == true)
      {
        return IQ_SUCCESS;
      }
      while (1)
      {
        auto result = ::sem_wait(sem_);
        if (result == 0)
        {
          locked_.store(true, std::memory_order_release);
          return IQ_SUCCESS;
        }
        if (errno == EINTR)
        {
          continue;
        }
        return IQ_ERROR;
      }
    }

    IQ_RESULT lock(uint32_t tmout)
    {
      if (sem_ == SEM_FAILED)
      {
        return IQ_INVALID_HANDLE;
      }
      if (locked_.load(std::memory_order_acquire) == true)
      {
        return IQ_SUCCESS;
      }
      struct timespec now_ts;
      clock_gettime(CLOCK_REALTIME, &now_ts);
      int64_t now_ms = (now_ts.tv_sec * 1000) + (now_ts.tv_nsec / 1000000) + tmout;
      auto tmout_ts  = to_timespec(now_ms);
      while (1)
      {
        auto result = ::sem_timedwait(sem_, &tmout_ts);
        if (result == 0)
        {
          locked_.store(true, std::memory_order_release);
          return IQ_SUCCESS;
        }
        if (errno == EINTR)
        {
          continue;
        }
        return IQ_ERROR;
      }
    }

    void unlock()
    {
      if (sem_ == SEM_FAILED)
      {
        return;
      }
      if (locked_.load(std::memory_order_acquire) == false)
      {
        return;
      }
      ::sem_post(sem_);
      locked_.store(false, std::memory_order_release);
    }
  };

  class SemLockGuard
  {
    Semaphore& sem_;

    bool locked_;

  public:
    SemLockGuard(Semaphore& sem)
      : sem_{sem}
      , locked_{false}
    {
      locked_ = sem_.lock() == IQ_SUCCESS;
    }

    ~SemLockGuard()
    {
      if (locked_ == true)
      {
        sem_.unlock();
      }
    }
  };
} /* namespace IQ */
#endif /* _SEMAPHORE_HPP_ */
