/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#include "sharemem.hpp"

#include "common.hpp"
#include "debug.hpp"
#include <fcntl.h>
#include <limits>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace IQ
{
  namespace
  {
    class FD
    {
      int fd_;

    public:
      FD()
        : fd_{InvalidFD}
      {
      }

      FD(int fd)
        : fd_{fd}
      {
      }

      ~FD()
      {
        close();
      }

      void close()
      {
        if (fd_ != InvalidFD)
        {
          ::close(fd_);
          fd_ = InvalidFD;
        }
      }

      int get() const
      {
        return fd_;
      }

      operator int() const
      {
        return fd_;
      }

      int operator=(int fd)
      {
        close();
        fd_ = fd;
        return fd_;
      }
    };
  } // namespace

  ShareMem::ShareMem()
    : ptr_{nullptr}
    , size_{0}
  {
    DPRINTF(">> ShareMem::ShareMem\n");
  }

  ShareMem::~ShareMem()
  {
    DPRINTF(">> ShareMem::~ShareMem\n");
    close();
  }

  IQ_RESULT ShareMem::create(const std::string& name, uint64_t size)
  {
    DPRINTF(">> ShareMem::create\n");
    close();

    FD fd = ::shm_open(("/" + name + "_mem").c_str(), O_CREAT | O_RDWR, 0660);
    if (fd == InvalidFD)
    {
      return IQ_ERROR;
    }

    auto result = ::ftruncate(fd, size);
    if (result == -1)
    {
      return IQ_ERROR;
    }

    ptr_ = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr_ == MAP_FAILED)
    {
      return IQ_ERROR;
    }
    size_ = size;
    name_ = "/" + name + "_mem";
    return IQ_SUCCESS;
  }

  IQ_RESULT ShareMem::open(const std::string& name)
  {
    DPRINTF(">> ShareMem::open\n");
    close();

    FD fd = ::shm_open(("/" + name + "_mem").c_str(), O_RDWR, 0660);
    if (fd == InvalidFD)
    {
      return IQ_ERROR;
    }

    struct stat st;
    auto result = ::fstat(fd, &st);
    if (result != 0)
    {
      return IQ_ERROR;
    }

    if (st.st_size <= 0)
    {
      return IQ_ERROR;
    }

    if (static_cast<unsigned long long>(st.st_size) > std::numeric_limits<size_t>::max())
    {
      return IQ_ERROR;
    }

    ptr_ = ::mmap(nullptr, static_cast<size_t>(st.st_size), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr_ == MAP_FAILED)
    {
      return IQ_ERROR;
    }
    size_ = static_cast<size_t>(st.st_size);
    name_ = "/" + name + "_mem";
    return IQ_SUCCESS;
  }

  void ShareMem::close(bool unlink)
  {
    if (ptr_ != nullptr && size_ > 0)
    {
      ::munmap(ptr_, size_);
      ptr_  = nullptr;
      size_ = 0;
      DPRINTF(">> ShareMem::close\n");
    }
    if (unlink == true && name_.empty() == false)
    {
      ::shm_unlink(name_.c_str());
      DPRINTF(">> ShareMem::unlink\n");
    }
    name_.clear();
  }
} /* namespace IQ */
