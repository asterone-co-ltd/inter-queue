
# inter-queue

inter-queue (library name: **libiqueue**) is an **SPSC (Single Producer / Single Consumer) IPC (inter-process communication) library**.
By assuming **one-to-one communication**, it provides low-overhead and high-speed data transfer.

---

## Overview

inter-queue is a queue library designed to satisfy the following requirements:

- **One-to-one (SPSC) IPC**
- High-speed stream-style data transfer
- Data exchange between processes and threads
- Simple API without explicit locking in user code

---

## Features

- ✅ **Low latency** through SPSC-focused design
- ✅ **IPC-ready** via named queues
- ✅ Shared-memory-based queue
- ✅ Variable-length data push / pop
- ✅ Wait-for-push API (with timeout)
- ✅ FAST mode for lower overhead
- ✅ Option to discard old data when the queue is full
- ✅ C / C++ compatible API

---

## Requirements

- C99 or later
- Linux / Windows

---

## Build

Linux
``` sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cd build
make
```

Windows
``` sh
cmake -S . -B build -G "Visual Studio 17 2022"
cd build
cmake --build . --config Release
```

---

## Install

inter-queue can be installed with CMake.

Linux (install prefix is optional):
``` sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DIQUEUE_BUILD_EXAMPLES=OFF
cmake --build build
cmake --install build --prefix /usr/local
```

Windows:
``` sh
cmake -S . -B build -G "Visual Studio 17 2022" -DIQUEUE_BUILD_EXAMPLES=OFF
cmake --build build --config Release
cmake --install build --config Release
```

---

## Use with CMake (find_package)

Consumer-side example:

``` cmake
find_package(IQueue CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE IQueue::iqueue)
```

---

## Quick Start

Producer side:

``` c
#include <interq.h>

int main(void)
{
	if (iq_initialize() != IQ_SUCCESS) {
		return 1;
	}

	iq_handle_t h = iq_create("demo-queue", 64 * 1024);
	if (h == NULL) {
		iq_finalize();
		return 1;
	}

	int value = 1234;
	(void)iq_push(h, &value, sizeof(value));

	iq_close(h);
	iq_finalize();
	return 0;
}
```

Consumer side:

``` c
#include <interq.h>

int main(void)
{
	if (iq_initialize() != IQ_SUCCESS) {
		return 1;
	}

	iq_handle_t h = iq_open("demo-queue");
	if (h == NULL) {
		iq_finalize();
		return 1;
	}

	if (iq_wait_for_push(h, 1000) == IQ_SUCCESS) {
		int value = 0;
		int n = iq_pop(h, &value, (int)sizeof(value));
		(void)n;
	}

	iq_close(h);
	iq_finalize();
	return 0;
}
```

---

## API Summary

- Lifecycle: `iq_initialize`, `iq_finalize`
- Queue management: `iq_create`, `iq_open`, `iq_close`, `iq_clean`
- Data path: `iq_push`, `iq_pop`, `iq_wait_for_push`
- State query: `iq_size`, `iq_count`, `iq_remain`
- Options: `iq_set_options`, `iq_get_options`
- Version: `iq_version`

Result code highlights:

- `IQ_SUCCESS`: success
- `IQ_TMOUT`: timeout (`iq_wait_for_push`)
- `IQ_FULL`: queue full (`iq_push` without drop-old option)
- Negative values: library errors (invalid arguments/handle/state)

---

## Concurrency Model And Notes

- inter-queue is designed for SPSC (single producer, single consumer).
- Multiple producers or multiple consumers for one queue are out of scope.
- Call `iq_initialize` before any queue operation.
- Call `iq_close` for each handle before `iq_finalize`.

### FAST mode (`IQ_OPT_FAST`)

`IQ_OPT_FAST` skips part of argument/state checks for lower overhead.

- Use only when input validity and call ordering are fully controlled by the caller.
- In FAST mode, misuse may cause undefined behavior.
- In particular, avoid calling `iq_close` concurrently with `iq_push`/`iq_pop`/query APIs on the same handle.

---

## License

MIT. See [LICENSE](./LICENSE).
