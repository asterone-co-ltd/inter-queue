/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#if !defined(_SEMAPHORE_HPP_)
#define _SEMAPHORE_HPP_

#include <windows.h>

#include "common.hpp"
#include "interq.h"
#include <cstdint>
#include <string>

namespace IQ
{

  class Semaphore
  {
    HANDLE handle_;

  public:
    Semaphore()
      : handle_{nullptr}
    {
    }

    ~Semaphore()
    {
      close();
    }

    IQ_RESULT create(const std::string& name)
    {
      handle_ = ::CreateSemaphore(nullptr, 1, 1, (name + "_sem").c_str());
      if (handle_ == nullptr)
      {
        return IQ_ERROR;
      }
      return IQ_SUCCESS;
    }

    IQ_RESULT open(const std::string& name)
    {
      handle_ = ::OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, false, (name + "_sem").c_str());
      if (handle_ == nullptr)
      {
        return IQ_ERROR;
      }
      return IQ_SUCCESS;
    }

    void close(bool unlink = false)
    {
      (void)unlink;
      if (handle_ != nullptr)
      {
        ::CloseHandle(handle_);
        handle_ = nullptr;
      }
    }

    IQ_RESULT lock(uint32_t tmout = INFINITE)
    {
      if (handle_ == nullptr)
      {
        return IQ_ARGUMENT_ERROR;
      }
      auto result = ::WaitForSingleObject(handle_, tmout);
      switch (result)
      {
        case WAIT_OBJECT_0:
          return IQ_SUCCESS;
        case WAIT_TIMEOUT:
          return IQ_TMOUT;
        case WAIT_FAILED:
          return IQ_ERROR;
        default:
          return IQ_ERROR;
      }
    }

    void unlock()
    {
      if (handle_ != nullptr)
      {
        ::ReleaseSemaphore(handle_, 1, nullptr);
      }
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
