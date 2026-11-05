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

namespace IQ {
namespace {
class FD {
  int fd_;

public:
  FD() : fd_{InvalidFD} {}

  FD(int fd) : fd_{fd} {}

  ~FD() { close(); }

  void close() {
    if (fd_ != InvalidFD) {
      ::close(fd_);
      fd_ = InvalidFD;
    }
  }

  int get() const { return fd_; }

  operator int() const { return fd_; }

  int operator=(int fd) {
    close();
    fd_ = fd;
    return fd_;
  }
};
} // namespace

ShareMem::ShareMem() : ptr_{nullptr}, size_{0}, dev_{0}, ino_{0} {
  DPRINTF(">> ShareMem::ShareMem\n");
}

ShareMem::~ShareMem() {
  DPRINTF(">> ShareMem::~ShareMem\n");
  close();
}

iq_result_t ShareMem::create(const std::string &name, uint64_t size) {
  DPRINTF(">> ShareMem::create\n");
  close();

  FD fd = ::shm_open(("/" + name + "_mem").c_str(), O_CREAT | O_RDWR, 0660);
  if (fd == InvalidFD) {
    return IQ_ERROR;
  }

  auto result = ::ftruncate(fd, size);
  if (result == -1) {
    return IQ_ERROR;
  }

  ptr_ = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (ptr_ == MAP_FAILED) {
    return IQ_ERROR;
  }

  struct stat st;
  if (::fstat(fd, &st) != 0) {
    return IQ_ERROR;
  }
  dev_ = st.st_dev;
  ino_ = st.st_ino;
  size_ = size;
  name_ = "/" + name + "_mem";
  return IQ_SUCCESS;
}

iq_result_t ShareMem::open(const std::string &name) {
  DPRINTF(">> ShareMem::open\n");
  close();

  FD fd = ::shm_open(("/" + name + "_mem").c_str(), O_RDWR, 0660);
  if (fd == InvalidFD) {
    return IQ_ERROR;
  }

  struct stat st;
  auto result = ::fstat(fd, &st);
  if (result != 0) {
    return IQ_ERROR;
  }

  if (st.st_size <= 0) {
    return IQ_ERROR;
  }

  if (static_cast<unsigned long long>(st.st_size) >
      std::numeric_limits<size_t>::max()) {
    return IQ_ERROR;
  }

  ptr_ = ::mmap(nullptr, static_cast<size_t>(st.st_size),
                PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (ptr_ == MAP_FAILED) {
    return IQ_ERROR;
  }
  dev_ = st.st_dev;
  ino_ = st.st_ino;
  size_ = static_cast<size_t>(st.st_size);
  name_ = "/" + name + "_mem";
  return IQ_SUCCESS;
}

bool ShareMem::close(bool unlink) {
  bool removed = false;
  if (ptr_ != nullptr && size_ > 0) {
    ::munmap(ptr_, size_);
    ptr_ = nullptr;
    size_ = 0;
  }
  if (unlink == true && name_.empty() == false) {
    /* 名前が今も「自分が開いた実体」を指すときだけ消す */
    FD fd = ::shm_open(name_.c_str(), O_RDONLY, 0);
    struct stat st;
    if (fd != InvalidFD && ::fstat(fd, &st) == 0 &&
        st.st_dev == dev_ && st.st_ino == ino_) {
      removed = (::shm_unlink(name_.c_str()) == 0);
    }
  }
  name_.clear();
  return removed;
}
} /* namespace IQ */
