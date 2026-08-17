/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#if !defined(_SHAREMEM_HPP_)
#define _SHAREMEM_HPP_

#include "interq.h"
#include <cstdint>
#include <string>

namespace IQ
{
  class ShareMem
  {
    std::string name_;

    void* ptr_;

    size_t size_;

  public:
    ShareMem();

    ~ShareMem();

    IQ_RESULT create(const std::string& name, uint64_t size);

    IQ_RESULT open(const std::string& name);

    void close(bool unlink = false);

    bool is_valid() const
    {
      return ptr_ != nullptr && size_ > 0;
    }

    void* operator()(size_t n)
    {
      return static_cast<void*>(&(*this)[n]);
    }

    const void* operator()(size_t n) const
    {
      return static_cast<const void*>(&(*this)[n]);
    }

    uint8_t& operator[](size_t n)
    {
      return static_cast<uint8_t*>(ptr_)[n];
    }

    const uint8_t& operator[](size_t n) const
    {
      return static_cast<const uint8_t*>(ptr_)[n];
    }
  };
} /* namespace IQ */
#endif /* _SHAREMEM_HPP_ */
