/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#if !defined(_SHAREMEM_HPP_)
#define _SHAREMEM_HPP_

#include <windows.h>

#include "interq.h"
#include <cstdint>
#include <string>

namespace IQ {
class ShareMem {
  HANDLE handle_;

  void *ptr_;

public:
  ShareMem();

  ~ShareMem();

  iq_result_t create(const std::string &name, uint64_t size);

  iq_result_t open(const std::string &name);

  bool close(bool unlink = false);

  bool is_valid() const { return handle_ != nullptr && ptr_ != nullptr; }

  void *operator()(size_t n) { return static_cast<void *>(&(*this)[n]); }

  const void *operator()(size_t n) const {
    return static_cast<const void *>(&(*this)[n]);
  }

  uint8_t &operator[](size_t n) { return static_cast<uint8_t *>(ptr_)[n]; }

  const uint8_t &operator[](size_t n) const {
    return static_cast<uint8_t *>(ptr_)[n];
  }
};
} /* namespace IQ */
#endif /* _SHAREMEM_HPP_ */
