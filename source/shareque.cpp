/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#include "shareque.hpp"

#include "common.hpp"
#include "debug.hpp"
#include <atomic>
#include <climits>
#include <cstring>
#include <limits>
#include <new>

namespace IQ
{
  namespace
  {
    constexpr std::size_t CacheLineSize = std::hardware_destructive_interference_size;

    struct alignas(CacheLineSize) Atomic32
    {
      std::atomic<uint32_t> v;
    };

    struct QueHeader
    {
      uint32_t build_hash;
      std::atomic<uint32_t> references;
      std::atomic<uint32_t> waiters;
      uint32_t size;
      uint32_t mask;
      std::atomic<uint32_t> event;
      /* Declare wpos and rpos so that they are separate cache lines. */
      Atomic32 wpos;
      Atomic32 rpos;
      uint8_t padding[CacheLineSize];
    };

    /*
     * Calculate powers of 2
     */
    uint64_t roundup_pow2(uint64_t x)
    {
#if 0
      return std::bit_ceil(x <= 1 ? 1 : x);
#else
      if (x <= 1)
      {
        return 1;
      }
      --x;
      x |= x >> 1;
      x |= x >> 2;
      x |= x >> 4;
      x |= x >> 8;
      x |= x >> 16;
      x |= x >> 32;
      return x + 1;
#endif
    }
  } // namespace

  ShareQue::ShareQue()
    : mem_{}
    , sem_{}
    , evt_{}
  {
    DPRINTF(">> ShareQue::ShareQue\n");
  }

  ShareQue::~ShareQue()
  {
    DPRINTF(">> ShareQue::~ShareQue\n");
    close();
  }

  IQ_RESULT ShareQue::create(const std::string& name, uint64_t size)
  {
    DPRINTF(">> ShareQue::create\n");
    close();

    size = roundup_pow2(size);
    if (size > static_cast<size_t>(INT_MAX))
    {
      return IQ_ARGUMENT_ERROR;
    }

    auto result = ([&] {
      auto result = sem_.create(name);
      if (result != IQ_SUCCESS)
      {
        return result;
      }

      SemLockGuard lock{sem_};
      result = mem_.create(name, size + sizeof(QueHeader));
      if (result != IQ_SUCCESS)
      {
        return result;
      }

      auto *header = static_cast<QueHeader *>(mem_(0));
      if (header->build_hash != get_build_hash() || header->references.load(std::memory_order_acquire) == 0)
      {
        header->build_hash = get_build_hash();
        header->references = 1;
        header->waiters = 0;
        header->size = static_cast<uint32_t>(size);
        header->mask = header->size - 1;
        header->event = 0;
        header->wpos.v = 0;
        header->rpos.v = 0;
      }
      else
      {
        header->references.fetch_add(1);
        if (header->references.load(std::memory_order_acquire) > 2) {
          return IQ_ERROR;
        }
      }

#if defined(IQ_PLATFORM_WINDOWS)
      result = evt_.create(name);
#else /* defined(IQ_PLATFORM_LINUX) */
      result = evt_.create(&header->event);
#endif
      if (result != IQ_SUCCESS)
      {
        return result;
      }
      return IQ_SUCCESS;
    })();

    if (result != IQ_SUCCESS)
    {
      close();
    }
    return result;
  }

  IQ_RESULT ShareQue::open(const std::string& name)
  {
    DPRINTF(">> ShareQue::open\n");
    close();

    auto result = ([&] {
      auto result = sem_.open(name);
      if (result != IQ_SUCCESS)
      {
        return result;
      }

      SemLockGuard lock{sem_};
      result = mem_.open(name);
      if (result != IQ_SUCCESS)
      {
        return result;
      }

      auto *header = static_cast<QueHeader *>(mem_(0));
      if (header->build_hash != get_build_hash())
      {
        return IQ_ERROR;
      }

      header->references.fetch_add(1);
      if (header->references.load(std::memory_order_acquire) > 2)
      {
        return IQ_ERROR;
      }

#if defined(IQ_PLATFORM_WINDOWS)
      result = evt_.open(name);
#else /* defined(IQ_PLATFORM_LINUX) */
      result = evt_.open(&header->event);
#endif
      if (result != IQ_SUCCESS)
      {
        return result;
      }
      return IQ_SUCCESS;
    })();

    if (result != IQ_SUCCESS)
    {
      close();
    }
    return result;
  }

  void ShareQue::close()
  {
    DPRINTF(">> ShareQue::close\n");
    bool unlink = false;
    {
      SemLockGuard lock{sem_};
      if (mem_.is_valid() == true)
      {
        auto* header = static_cast<QueHeader*>(mem_(0));
        if (header->waiters.load(std::memory_order_relaxed) > 0)
        {
          evt_.set();
        }
        auto ref = header->references.load(std::memory_order_acquire);
        if (ref >= 1)
        {
          header->references.fetch_sub(1);
        }
        unlink = header->references.load(std::memory_order_acquire) == 0;
        DPRINTF("ref = %d\n", header->references.load(std::memory_order_acquire));
      }
      mem_.close(unlink);
      evt_.close();
    }
    sem_.close(unlink);
  }

  int ShareQue::push(const void* data, int dsize, bool drop_old)
  {
    auto* ptr    = static_cast<const uint8_t*>(data);
    auto* header = static_cast<QueHeader*>(mem_(0));
    auto* qbuff  = static_cast<uint8_t*>(mem_(sizeof(QueHeader)));

    if (dsize <= 0 || dsize > size())
    {
      return IQ_ERROR;
    }

    /* If you want to create space by discarding old items, loop until you can create space. */
    while (1)
    {
      auto wpos = header->wpos.v.load(std::memory_order_relaxed);
      auto rpos = header->rpos.v.load(std::memory_order_acquire);
      auto used = wpos - rpos;
      auto free = header->size - used;

      if (static_cast<uint32_t>(dsize) <= free)
      {
        break;
      }

      if (drop_old == false)
      {
        return IQ_FULL;
      }

      /*
       * Check rpos to see if the queue is being operated on the consumer side.
       * If it is not being operated, free up space on the producer side.
       */
      auto result = header->rpos.v.compare_exchange_strong(
        rpos, rpos + static_cast<uint32_t>(dsize) - free, std::memory_order_acq_rel, std::memory_order_acquire);
      if (result == true)
      {
        break;
      }

      /* Retry! */
    }

    auto wpos      = header->wpos.v.load(std::memory_order_relaxed);
    auto wpos_mask = wpos & header->mask;
    if ((wpos_mask + dsize) < header->size) [[likely]]
    {
      ::memcpy(&qbuff[wpos_mask], ptr, dsize);
    }
    else
    {
      auto n = header->size - wpos_mask;
      auto m = dsize - n;
      ::memcpy(&qbuff[wpos_mask], ptr, n);
      ::memcpy(&qbuff[0], &ptr[n], m);
    }
    header->wpos.v.store(wpos + static_cast<uint32_t>(dsize), std::memory_order_release);

    /* Notify consumer */
    if (drop_old == true || header->waiters.load(std::memory_order_relaxed) > 0)
    {
      evt_.set();
    }
    return dsize;
  }

  int ShareQue::pop(void* buff, int bsize)
  {
    while (1)
    {
      auto* ptr    = static_cast<uint8_t*>(buff);
      auto* header = static_cast<QueHeader*>(mem_(0));
      auto* qbuff  = static_cast<uint8_t*>(mem_(sizeof(QueHeader)));
      auto wpos    = header->wpos.v.load(std::memory_order_acquire);
      auto rpos    = header->rpos.v.load(std::memory_order_acquire);
      if (bsize > static_cast<int>(wpos - rpos))
      {
        bsize = static_cast<int>(wpos - rpos);
      }
      if (bsize <= 0)
      {
        return 0;
      }

      auto rpos_mask = rpos & header->mask;
      if ((rpos_mask + bsize) < header->size) [[likely]]
      {
        ::memcpy(ptr, &qbuff[rpos_mask], bsize);
      }
      else
      {
        auto n = header->size - rpos_mask;
        auto m = bsize - n;
        ::memcpy(ptr, &qbuff[rpos_mask], n);
        ::memcpy(&ptr[n], &qbuff[0], m);
      }

      /*
       * Check rpos to see if the queue is being manipulated on the producer side.
       * If so, retry.
       */
      auto result =
        header->rpos.v.compare_exchange_strong(rpos, rpos + bsize, std::memory_order_release, std::memory_order_acquire);
      if (result == true)
      {
        break;
      }

      /* Retry! */
    }
    return bsize;
  }

  int ShareQue::size() const
  {
    const auto* header = static_cast<QueHeader*>(mem_(0));
    return static_cast<int>(header->size);
  }

  int ShareQue::count() const
  {
    auto* header = static_cast<QueHeader*>(mem_(0));
    auto wpos    = header->wpos.v.load(std::memory_order_acquire);
    auto rpos    = header->rpos.v.load(std::memory_order_acquire);
    return static_cast<int>(wpos - rpos);
  }

  int ShareQue::remain() const
  {
    return size() - count();
  }

  int ShareQue::ref_count() const
  {
    auto* header    = static_cast<QueHeader*>(mem_(0));
    auto references = header->references.load(std::memory_order_acquire);
    return static_cast<int>(references);
  }

  IQ_RESULT ShareQue::wait_for_event(uint32_t tmout)
  {
    auto* header = static_cast<QueHeader*>(mem_(0));
    header->waiters.fetch_add(1, std::memory_order_relaxed);
    auto result = evt_.wait_for_event(tmout);
    header->waiters.fetch_sub(1, std::memory_order_relaxed);
    return result;
  }

  void ShareQue::clean(const std::string& name)
  {
    DPRINTF(">> ShareQue::clean\n");
    ShareMem mem;
    if (mem.open(name) == IQ_SUCCESS)
    {
      mem.close(true);
    }
    Semaphore sem;
    if (sem.open(name) == IQ_SUCCESS)
    {
      sem.close(true);
    }
  }
} /* namespace IQ */
