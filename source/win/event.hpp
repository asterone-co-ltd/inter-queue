/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#if !defined(_EVENT_HPP_)
#define _EVENT_HPP_

#include <windows.h>

#include "common.hpp"
#include "interq.h"
#include <cstdint>
#include <string>

namespace IQ {

class Event {
  HANDLE handle_;

public:
  Event() : handle_{nullptr} {}

  ~Event() { close(); }

  iq_result_t create(const std::string &name) {
    handle_ = ::CreateEvent(nullptr, false, false, (name + "_evt").c_str());
    if (handle_ == nullptr) {
      return IQ_ERROR;
    }
    return IQ_SUCCESS;
  }

  iq_result_t open(const std::string &name) {
    handle_ = ::OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, false,
                          (name + "_evt").c_str());
    if (handle_ == nullptr) {
      return IQ_ERROR;
    }
    return IQ_SUCCESS;
  }

  void close() {
    if (handle_ != nullptr) {
      ::CloseHandle(handle_);
      handle_ = nullptr;
    }
  }

  bool set() {
    if (handle_ == nullptr) {
      return false;
    }
    return ::SetEvent(handle_);
  }

  iq_result_t wait_for_event(uint32_t tmout) {
    if (handle_ == nullptr) {
      return IQ_ERROR;
    }
    if (tmout == INFINITE) {
      return IQ_ARGUMENT_ERROR;
    }
    auto result = ::WaitForSingleObject(handle_, tmout);
    switch (result) {
    case WAIT_OBJECT_0:
      return IQ_SUCCESS;
    case WAIT_TIMEOUT:
      return IQ_TMOUT;
    case WAIT_ABANDONED:
    case WAIT_FAILED:
    default:
      return IQ_ERROR;
    }
  }
};
} /* namespace IQ */
#endif /* _EVENT_HPP_ */
