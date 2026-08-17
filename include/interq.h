/*
 * Inter-process Queue
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 ASTERONE Co.Ltd. <https://asterone.co.jp/>
 */
#ifndef _INTER_Q_H_
#define _INTER_Q_H_

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(IQ_STATIC)
#define IQ_API
#elif defined(IQ_BUILD)
#define IQ_API __declspec(dllexport)
#else
#define IQ_API __declspec(dllimport)
#endif
#define IQ_LOCAL
#else
/* GCC/Clang on ELF */
#if defined(IQ_STATIC)
#define IQ_API
#define IQ_LOCAL
#else
#define IQ_API __attribute__((visibility("default")))
#define IQ_LOCAL __attribute__((visibility("hidden")))
#endif
#endif

/*
 * inter-queue result code.
 */
typedef enum {
  IQ_SUCCESS = 0,
  IQ_ERROR = -1,
  IQ_UNINITIALIZATION = -2,
  IQ_ARGUMENT_ERROR = -3,
  IQ_INVALID_HANDLE = -4,
  IQ_TMOUT = -5,
  IQ_FULL = -6,
} iq_result_t;

/*
 * inter-queue option flags.
 */
typedef enum {
  IQ_OPT_FAST =
      0x01, /* If this option is specified,
             * Some error checks will be omitted and the following functions
             * will run slightly faster: push/pop/size/count/remain
             */
  IQ_OPT_PUSH_DROP_OLD =
      0x02, /* If you specify this option,
             * even if there is no space in the queue when pushing, old data
             * will be deleted to make room.
             */
} iq_options_t;

/*
 * inter-queue object handle.
 */
typedef void *iq_handle_t;

/*
 * inter-queue version
 */
IQ_API const char *iq_version();

/*
 * Initializing the inter-queue library
 */
IQ_API iq_result_t iq_initialize();

/*
 * Finalize the inter-queue library.
 */
IQ_API void iq_finalize();

/*
 * Create an inter-queue object.
 */
IQ_API iq_handle_t iq_create(const char *name, uint64_t size);

/*
 * Open an inter-queue object.
 */
IQ_API iq_handle_t iq_open(const char *name);

/*
 * Close an inter-queue object.
 */
IQ_API iq_result_t iq_close(iq_handle_t handle);

/*
 * Sets options on the inter-queue object.
 */
IQ_API iq_result_t iq_set_options(iq_handle_t handle, iq_options_t opt);

/*
 * Gets the options set on the inter-queue object.
 */
IQ_API iq_result_t iq_get_options(iq_handle_t handle, iq_options_t *opt);

/*
 * Pushes a data into an inter-queue object.
 */
IQ_API int iq_push(iq_handle_t handle, const void *data, int size);

/*
 * Pops a data from an inter-queue object.
 */
IQ_API int iq_pop(iq_handle_t handle, void *buff, int size);

/*
 * Waits for values ​​to be pushed into the inter-queue object.
 */
IQ_API iq_result_t iq_wait_for_push(iq_handle_t handle, uint32_t tmout);

/*
 * Gets the queue size set in the inter-queue object.
 */
IQ_API int iq_size(iq_handle_t handle);

/*
 * Gets the number of bytes of data pushed to the inter-queue object.
 */
IQ_API int iq_count(iq_handle_t handle);

/*
 * Gets the free space in the queue of an inter-queue object.
 */
IQ_API int iq_remain(iq_handle_t handle);

/*
 * Deletes the shared memory with the specified name.
 */
IQ_API iq_result_t iq_clean(const char *name);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _INTER_Q_H_ */
