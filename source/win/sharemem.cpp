/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#include "sharemem.hpp"

#include "common.hpp"

namespace IQ
{
  ShareMem::ShareMem()
    : handle_{nullptr}
    , ptr_{nullptr}
  {
  }

  ShareMem::~ShareMem()
  {
    close();
  }

  IQ_RESULT ShareMem::create(const std::string& name, uint64_t size)
  {
    handle_ = ::CreateFileMapping(INVALID_HANDLE_VALUE,      // use paging file
                                  nullptr,                   // default security
                                  PAGE_READWRITE,            // read/write access
                                  (size >> 32) & 0xFFFFFFFF, // maximum object size (high-order DWORD)
                                  (size >> 0) & 0xFFFFFFFF,  // maximum object size (low-order DWORD)
                                  (name + "_mem").c_str());  // name of mapping object
    if (handle_ == nullptr)
    {
      return IQ_ERROR;
    }

    ptr_ = ::MapViewOfFile(handle_, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (ptr_ == nullptr)
    {
      close();
      return IQ_ERROR;
    }
    return IQ_SUCCESS;
  }

  IQ_RESULT ShareMem::open(const std::string& name)
  {
    handle_ = ::OpenFileMapping(FILE_MAP_ALL_ACCESS,      // read/write access
                                false,                    // do not inherit the name
                                (name + "_mem").c_str()); // name of mapping object
    if (handle_ == nullptr)
    {
      return IQ_ERROR;
    }

    ptr_ = ::MapViewOfFile(handle_, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (ptr_ == nullptr)
    {
      close();
      return IQ_ERROR;
    }
    return IQ_SUCCESS;
  }

  void ShareMem::close(bool unlink)
  {
    (void)unlink;
    if (ptr_)
    {
      ::UnmapViewOfFile(ptr_);
      ptr_ = nullptr;
    }
    if (handle_)
    {
      ::CloseHandle(handle_);
      handle_ = nullptr;
    }
  }

} /* namespace IQ */
