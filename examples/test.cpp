/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#include <stdint.h>

#include <chrono>
#include <cstring>
#include <interq.h>
#include <iostream>
#include <thread>

namespace
{
  constexpr char QueueName[]    = "iq-test";
  constexpr size_t QueueSize    = 256 * 1024;
  constexpr size_t Count        = 100000000;
  constexpr size_t Retry        = 100000000;
  constexpr size_t TimeoutRetry = 3;

  struct Data
  {
    uint8_t v[128];
  };

} // namespace

int main(int argc, char* argv[])
{
  auto result = iq_initialize();
  if (result != IQ_SUCCESS)
  {
    std::cerr << "iq_initialize failure: " << result << std::endl;
    return -1;
  }

  iq_clean(QueueName);

  auto producer = iq_create(QueueName, QueueSize);
  if (producer == nullptr)
  {
    std::cerr << "iq_create failure" << std::endl;
    return -1;
  }
  iq_set_options(producer, IQ_OPT_FAST);
  // iq_set_options(producer, IQ_OPT_PUSH_DROP_OLD);
  auto producer_success = 0;
  auto producer_failure = 0;
  auto producer_retry   = 0;

  auto consumer         = iq_open(QueueName);
  if (consumer == nullptr)
  {
    std::cerr << "iq_open failure" << std::endl;
    return -1;
  }
  auto consumer_success     = 0;
  auto consumer_failure     = 0;
  auto consumer_retry       = 0;
  auto consumer_tmout_retry = 0;

  auto start                = std::chrono::steady_clock::now();

  auto th_consumer          = std::thread(
    [&]
    {
      Data data;
      while (1)
      {
        auto result = iq_wait_for_push(consumer, 100);
        if (result == IQ_TMOUT)
        {
          ++consumer_tmout_retry;
          if (consumer_tmout_retry > TimeoutRetry)
          {
            break;
          }
          continue;
        }
        consumer_tmout_retry = 0;
        if (result != IQ_SUCCESS)
        {
          break;
        }
        while (1)
        {
          auto len = iq_pop(consumer, &data, sizeof(data));
          if (len == sizeof(data))
          {
            consumer_retry = 0;
            ++consumer_success;
            if (consumer_success == Count)
            {
              return;
            }
          }
          else
          {
            ++consumer_retry;
            ++consumer_failure;
            if (consumer_retry > Retry)
            {
              return;
            }
            break;
          }
        }
      }
    });

  auto th_producer = std::thread(
    [&]
    {
      Data data;
      while (1)
      {
        auto count = iq_push(producer, &data, sizeof(data));
        if (count == sizeof(data))
        {
          producer_retry = 0;
          ++producer_success;
          if (producer_success == Count)
          {
            return;
          }
        }
        else
        {
          ++producer_retry;
          ++producer_failure;
          if (producer_retry > Retry)
          {
            break;
          }
        }
      }
    });

  th_consumer.join();
  th_producer.join();

  auto end      = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  if (producer_success == Count && consumer_success == Count)
  {
    auto bps = (Count * sizeof(Data) * 8) / (duration.count() / 1000000.0);
    std::cout << "Success." << std::endl;
    std::cout << duration.count() << "us" << " (" << bps / 1e6 << "Mbps)" << std::endl;
  }
  else
  {
    std::cout << "Failure." << std::endl;
    std::cout << duration.count() << "us" << std::endl;
  }
  std::cout << "---" << std::endl;
  std::cout << "producer success: " << producer_success << "/" << Count << std::endl;
  std::cout << "producer failure: " << producer_failure << std::endl;
  std::cout << "producer size   : " << iq_size(producer) << std::endl;
  std::cout << "---" << std::endl;
  std::cout << "consumer success: " << consumer_success << "/" << Count << std::endl;
  std::cout << "consumer failure: " << consumer_failure << std::endl;
  std::cout << "consumer size   : " << iq_size(consumer) << std::endl;

  iq_close(producer);
  iq_close(consumer);
  iq_finalize();
}
