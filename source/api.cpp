/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#include "debug.hpp"
#include "interq.h"
#include "shareque.hpp"
#include "version.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace
{
  enum Status
  {
    Open,
    Closing,
    Closed,
  };

  using ShareQuePtr = std::unique_ptr<IQ::ShareQue>;

  struct HandleImpl
  {
    ShareQuePtr queue_;
    std::atomic<Status> status;
    std::atomic<int> busy;
    uint32_t options;
  };

  using Handle     = std::unique_ptr<HandleImpl>;

  using Mutex      = std::mutex;

  using LockGuard  = std::lock_guard<std::mutex>;

  using UniqueLock = std::unique_lock<std::mutex>;

  class BusyLock
  {
    HandleImpl* handle_;

  public:
    BusyLock(HandleImpl* handle)
      : handle_{handle}
    {
      handle_->busy.fetch_add(1, std::memory_order_release);
    }

    ~BusyLock()
    {
      handle_->busy.fetch_sub(1, std::memory_order_release);
    }
  };

  class SharedQueue
  {
    std::atomic<bool> initialized_;

    std::vector<Handle> handles_;

    std::thread thread_;

    std::atomic<bool> thread_quit_;

    Mutex mtx_;

    std::condition_variable thread_resume_cond_;

    Mutex thread_resume_mtx_;

    bool thread_resume_;

    SharedQueue()
      : initialized_{false}
      , handles_{}
      , thread_{}
      , thread_quit_{false}
      , mtx_{}
      , thread_resume_cond_{}
      , thread_resume_mtx_{}
      , thread_resume_{}
    {
    }

    void thread_proc()
    {
      while (1)
      {
        {
          UniqueLock lock{thread_resume_mtx_};
          thread_resume_cond_.wait(lock, [this] { return thread_resume_ || thread_quit_.load(std::memory_order_acquire); });
          if (thread_quit_.load(std::memory_order_acquire) == true)
          {
            break;
          }
          thread_resume_ = false;
        }
        {
          LockGuard lock{mtx_};
          for (auto& h : handles_)
          {
            if (h->status.load(std::memory_order_acquire) == Status::Closing && h->busy.load(std::memory_order_acquire) == 0)
            {
              h->queue_.reset();
              h->status.store(Status::Closed, std::memory_order_release);
            }
          }
        }
      }
    }

  public:
    ~SharedQueue()
    {
      finalize();
    }

    IQ_RESULT initialize()
    {
      if (thread_.joinable())
      {
        return IQ_SUCCESS;
      }
      thread_quit_.store(false, std::memory_order_release);
      thread_resume_ = false;
      thread_        = std::thread([this] { thread_proc(); });
      initialized_.store(true, std::memory_order_release);
      return IQ_SUCCESS;
    }

    void finalize()
    {
      if (thread_.joinable() == true)
      {
        {
          std::lock_guard<std::mutex> lock(thread_resume_mtx_);
          thread_quit_.store(true, std::memory_order_release);
        }
        thread_resume_cond_.notify_one();
        thread_.join();
      }

      LockGuard lock{mtx_};
      for (auto& h : handles_)
      {
        if (h->status.load(std::memory_order_acquire) == Status::Closed)
        {
          continue;
        }
        while (h->busy.load(std::memory_order_acquire) > 0)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        h->status.store(Status::Closed, std::memory_order_release);
      }
      handles_.clear();

      initialized_.store(false, std::memory_order_release);
    }

    IQ_HANDLE create(const char* name, uint64_t size)
    {
      if (initialized_.load(std::memory_order_acquire) == false)
      {
        return nullptr;
      }
      if (name == nullptr || size == 0)
      {
        return nullptr;
      }
      try
      {
        auto queue  = std::make_unique<IQ::ShareQue>();
        auto result = queue->create(name, size);
        if (result != IQ_SUCCESS)
        {
          return nullptr;
        }
        auto handle = ([this] {
          LockGuard lock{mtx_};
          for (auto &h : handles_)
          {
            if (h->status.load(std::memory_order_acquire) == Status::Closed)
            {
              return h.get();
            }
          }
          auto h = std::make_unique<HandleImpl>();
          handles_.push_back(std::move(h));
          return handles_[handles_.size() - 1].get();
        }());
        handle->queue_ = std::move(queue);
        handle->status.store(Status::Open, std::memory_order_release);
        handle->busy.store(0, std::memory_order_release);
        handle->options = 0;
        return static_cast<IQ_HANDLE>(handle);
      }
      catch (...)
      {
        return nullptr;
      }
    }

    IQ_HANDLE open(const char* name)
    {
      if (initialized_.load(std::memory_order_acquire) == false)
      {
        return nullptr;
      }
      if (name == nullptr)
      {
        return nullptr;
      }
      try
      {
        auto queue  = std::make_unique<IQ::ShareQue>();
        auto result = queue->open(name);
        if (result != IQ_SUCCESS)
        {
          return nullptr;
        }
        auto handle = ([this] {
          LockGuard lock{mtx_};
          for (auto &h : handles_)
          {
            if (h->status.load(std::memory_order_acquire) == Status::Closed)
            {
              return h.get();
            }
          }
          auto h = std::make_unique<HandleImpl>();
          handles_.push_back(std::move(h));
          return handles_[handles_.size() - 1].get();
        }());
        handle->queue_ = std::move(queue);
        handle->status.store(Status::Open, std::memory_order_release);
        handle->busy.store(0, std::memory_order_release);
        handle->options = 0;
        return static_cast<IQ_HANDLE>(handle);
      }
      catch (...)
      {
        return nullptr;
      }
    }

    IQ_RESULT close(IQ_HANDLE handle)
    {
      if (handle == nullptr)
      {
        return IQ_INVALID_HANDLE;
      }
      {
        auto h = static_cast<HandleImpl*>(handle);
        h->status.store(Status::Closing, std::memory_order_release);
      }
      {
        std::lock_guard<std::mutex> lock(thread_resume_mtx_);
        thread_resume_ = true;
      }
      thread_resume_cond_.notify_one();
      return IQ_SUCCESS;
    }

    IQ_RESULT set_options(IQ_HANDLE handle, IQ_OPTIONS opt)
    {
      if (initialized_.load(std::memory_order_acquire) == false)
      {
        return IQ_UNINITIALIZATION;
      }
      if (handle == nullptr)
      {
        return IQ_INVALID_HANDLE;
      }
      auto h     = static_cast<HandleImpl*>(handle);
      h->options = opt;
      return IQ_SUCCESS;
    }

    IQ_RESULT get_options(IQ_HANDLE handle, IQ_OPTIONS* opt)
    {
      if (initialized_.load(std::memory_order_acquire) == false)
      {
        return IQ_UNINITIALIZATION;
      }
      if (handle == nullptr)
      {
        return IQ_INVALID_HANDLE;
      }
      if (opt == nullptr)
      {
        return IQ_ARGUMENT_ERROR;
      }
      auto h = static_cast<HandleImpl*>(handle);
      *opt   = static_cast<IQ_OPTIONS>(h->options);
      return IQ_SUCCESS;
    }

    int push(IQ_HANDLE handle, const void* data, int size)
    {
      if (handle == nullptr)
      {
        return IQ_INVALID_HANDLE;
      }
      auto h = static_cast<HandleImpl*>(handle);
      if ((h->options & IQ_OPT_FAST) == 0)
      {
        if (initialized_.load(std::memory_order_acquire) == false)
        {
          return IQ_UNINITIALIZATION;
        }
        if (data == nullptr || size < 0)
        {
          return IQ_ARGUMENT_ERROR;
        }
        BusyLock busy{h};
        if (h->status.load(std::memory_order_acquire) != Status::Open)
        {
          return IQ_ERROR;
        }
        return h->queue_->push(data, size, h->options & IQ_OPT_PUSH_DROP_OLD);
      }
      else
      {
        return h->queue_->push(data, size, h->options & IQ_OPT_PUSH_DROP_OLD);
      }
    }

    int pop(IQ_HANDLE handle, void* buff, int size)
    {
      if (handle == nullptr)
      {
        return IQ_INVALID_HANDLE;
      }
      auto h = static_cast<HandleImpl*>(handle);
      if ((h->options & IQ_OPT_FAST) == 0)
      {
        if (initialized_.load(std::memory_order_acquire) == false)
        {
          return IQ_UNINITIALIZATION;
        }
        if (buff == nullptr || size < 0)
        {
          return IQ_ARGUMENT_ERROR;
        }
        BusyLock busy{h};
        if (h->status.load(std::memory_order_acquire) != Status::Open)
        {
          return IQ_ERROR;
        }
        return h->queue_->pop(buff, size);
      }
      else
      {
        return h->queue_->pop(buff, size);
      }
    }

    IQ_RESULT wait_for_push(IQ_HANDLE handle, uint32_t tmout)
    {
      if (initialized_.load(std::memory_order_acquire) == false)
      {
        return IQ_UNINITIALIZATION;
      }
      if (handle == nullptr)
      {
        return IQ_INVALID_HANDLE;
      }
      auto h = static_cast<HandleImpl*>(handle);
      BusyLock busy{h};
      if (h->status.load(std::memory_order_acquire) != Status::Open)
      {
        return IQ_ERROR;
      }
      return h->queue_->wait_for_event(tmout);
    }

    int size(IQ_HANDLE handle)
    {
      if (handle == nullptr)
      {
        return IQ_INVALID_HANDLE;
      }
      auto h = static_cast<HandleImpl*>(handle);
      if ((h->options & IQ_OPT_FAST) == 0)
      {
        if (initialized_.load(std::memory_order_acquire) == false)
        {
          return IQ_UNINITIALIZATION;
        }
        BusyLock busy{h};
        if (h->status.load(std::memory_order_acquire) != Status::Open)
        {
          return IQ_INVALID_HANDLE;
        }
        return h->queue_->size();
      }
      else
      {
        return h->queue_->size();
      }
    }

    int count(IQ_HANDLE handle)
    {
      if (handle == nullptr)
      {
        return IQ_INVALID_HANDLE;
      }
      auto h = static_cast<HandleImpl*>(handle);
      if ((h->options & IQ_OPT_FAST) == 0)
      {
        if (initialized_.load(std::memory_order_acquire) == false)
        {
          return IQ_UNINITIALIZATION;
        }
        BusyLock busy{h};
        if (h->status.load(std::memory_order_acquire) != Status::Open)
        {
          return IQ_INVALID_HANDLE;
        }
        return h->queue_->count();
      }
      else
      {
        return h->queue_->count();
      }
    }

    int remain(IQ_HANDLE handle)
    {
      if (handle == nullptr)
      {
        return IQ_INVALID_HANDLE;
      }
      auto h = static_cast<HandleImpl*>(handle);
      if ((h->options & IQ_OPT_FAST) == 0)
      {
        if (initialized_.load(std::memory_order_acquire) == false)
        {
          return IQ_UNINITIALIZATION;
        }
        BusyLock busy{h};
        if (h->status.load(std::memory_order_acquire) != Status::Open)
        {
          return IQ_INVALID_HANDLE;
        }
        return h->queue_->remain();
      }
      else
      {
        return h->queue_->remain();
      }
    }

    IQ_RESULT clean(const char* name)
    {
      if (name == nullptr)
      {
        return IQ_ARGUMENT_ERROR;
      }
      IQ::ShareQue::clean(name);
      return IQ_SUCCESS;
    }

    static SharedQueue& instance()
    {
      static SharedQueue obj;
      return obj;
    }
  };

} // namespace

extern "C"
{
  const char* iq_version()
  {
    return IQ::Version;
  }

  IQ_RESULT iq_initialize()
  {
    return SharedQueue::instance().initialize();
  }

  void iq_finalize()
  {
    return SharedQueue::instance().finalize();
  }

  IQ_HANDLE iq_create(const char* name, uint64_t size)
  {
    return SharedQueue::instance().create(name, size);
  }

  IQ_HANDLE iq_open(const char* name)
  {
    return SharedQueue::instance().open(name);
  }

  IQ_RESULT iq_close(IQ_HANDLE handle)
  {
    return SharedQueue::instance().close(handle);
  }

  IQ_RESULT iq_set_options(IQ_HANDLE handle, IQ_OPTIONS opt)
  {
    return SharedQueue::instance().set_options(handle, opt);
  }

  IQ_RESULT iq_get_options(IQ_HANDLE handle, IQ_OPTIONS* opt)
  {
    return SharedQueue::instance().get_options(handle, opt);
  }

  int iq_push(IQ_HANDLE handle, const void* data, int size)
  {
    return SharedQueue::instance().push(handle, data, size);
  }

  int iq_pop(IQ_HANDLE handle, void* buff, int size)
  {
    return SharedQueue::instance().pop(handle, buff, size);
  }

  IQ_RESULT iq_wait_for_push(IQ_HANDLE handle, uint32_t tmout)
  {
    return SharedQueue::instance().wait_for_push(handle, tmout);
  }

  int iq_size(IQ_HANDLE handle)
  {
    return SharedQueue::instance().size(handle);
  }

  int iq_count(IQ_HANDLE handle)
  {
    return SharedQueue::instance().count(handle);
  }

  int iq_remain(IQ_HANDLE handle)
  {
    return SharedQueue::instance().remain(handle);
  }

  IQ_RESULT iq_clean(const char* name)
  {
    return SharedQueue::instance().clean(name);
  }
} /* extern "C" */
