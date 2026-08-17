/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#include <stdint.h>

#include <atomic>
#include <interq.h>
#include <iostream>
#include <thread>

namespace {
constexpr char QueueName[] = "iq-example";
constexpr size_t QueueSize = 64;

std::atomic<bool> thread_quit_;

void thread_proc(iq_handle_t handle) {
  uint32_t n;
  while (thread_quit_ == false) {
    auto result = iq_wait_for_push(handle, 100);
    if (result == IQ_TMOUT) {
      continue;
    }
    if (result != IQ_SUCCESS) {
      break;
    }
    while (iq_count(handle) > 0) {
      auto count = iq_pop(handle, &n, sizeof(n));
      if (count == sizeof(n)) {
        std::cerr << "iq_pop success: " << n << std::endl;
      } else {
        std::cerr << "iq_pop failure: " << count << std::endl;
      }
    }
  }
}
} // namespace

int main(int argc, char *argv[]) {
  auto result = iq_initialize();
  if (result != IQ_SUCCESS) {
    std::cerr << "iq_initialize failure: " << result << std::endl;
    return -1;
  }

  auto handle = iq_open(QueueName);
  if (handle == nullptr) {
    std::cerr << "iq_open failure" << std::endl;
    return -1;
  }

  thread_quit_ = false;
  auto th = std::thread(thread_proc, handle);

  std::cin.get();

  thread_quit_ = true;
  th.join();

  iq_close(handle);
  iq_finalize();
}
