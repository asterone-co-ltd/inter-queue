/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#include <stdint.h>

#include <interq.h>
#include <iostream>
#include <string>

namespace
{
  constexpr char QueueName[] = "iq-example";
  constexpr size_t QueueSize = 64;
} // namespace

int main(int argc, char* argv[])
{
  auto result = iq_initialize();
  if (result != IQ_SUCCESS)
  {
    std::cerr << "iq_initialize failure: " << result << std::endl;
    return -1;
  }

  auto handle = iq_create(QueueName, QueueSize);
  if (handle == nullptr)
  {
    std::cerr << "iq_create failure" << std::endl;
    return -1;
  }

  int n = 0;
  std::string cmd;
  while (1)
  {
    if (!std::getline(std::cin, cmd))
    {
      break;
    }
    if (!cmd.empty())
    {
      break;
    }
    auto count = iq_push(handle, &n, sizeof(n));
    if (count == sizeof(n))
    {
      std::cerr << "iq_push success: " << iq_count(handle) << "/" << iq_size(handle) << std::endl;
    }
    else
    {
      std::cerr << "iq_push failure: " << iq_count(handle) << "/" << iq_size(handle) << std::endl;
    }
    ++n;
  }

  iq_close(handle);
  iq_finalize();
}
