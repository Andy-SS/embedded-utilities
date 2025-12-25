# Embedded C Utilities

A collection of lightweight, modular C utilities for embedded firmware development with unified RTOS synchronization support.

## Features

- **eLog** - Thread-safe logging system with configurable levels and multiple subscribers
- **Ring** - Per-instance circular buffer with RTOS-aware critical sections
- **Bit** - Bit manipulation utilities
- **Mutex Common** - Unified mutex callback interface for consistent RTOS integration
- Additional utilities for common firmware tasks

## Getting Started

### Building

Using CMake:
```bash
mkdir build
cd build
cmake ..
make
```

### Unified Mutex Support (NEW!)

All modules now use a unified `mutex_callbacks_t` interface defined in `mutex_common.h`:

```c
/* Unified mutex callback structure - shared by eLog and Ring */
typedef struct {
  void* (*create)(void);                           /* Create mutex */
  void (*destroy)(void *mutex);                    /* Destroy mutex */
  mutex_result_t (*acquire)(void *mutex, uint32_t timeout_ms);  /* Lock with timeout */
  mutex_result_t (*release)(void *mutex);          /* Unlock */
} mutex_callbacks_t;
```

This allows both eLog and Ring to share the same callback implementation!

### Quick Usage

**Logging with eLog (Thread-Safe):**
```c
#include "eLog.h"
#include "mutex_common.h"

// Register ThreadX mutex callbacks
extern const mutex_callbacks_t mutex_callbacks;
elog_register_mutex_callbacks(&mutex_callbacks);
elog_update_RTOS_ready(true);

int main(void) {
    LOG_INIT_WITH_CONSOLE();
    
    ELOG_INFO(ELOG_MD_MAIN, "System started");
    ELOG_DEBUG(ELOG_MD_MAIN, "Debug message");
    ELOG_ERROR(ELOG_MD_MAIN, "Error occurred");
    
    return 0;
}
```

**Ring Buffer (Per-Instance Mutex):**
```c
#include "ring.h"
#include "mutex_common.h"

// Register same callbacks for ring buffer
extern const mutex_callbacks_t mutex_callbacks;
ring_register_cs_callbacks(&mutex_callbacks);

int main(void) {
    // Create ring buffers - each gets its own mutex
    ring_t uart_rx_ring;
    ring_init_dynamic(&uart_rx_ring, 256, sizeof(uint8_t));
    
    // Use ring buffer - synchronization is automatic
    uint8_t byte = 0xAB;
    ring_write(&uart_rx_ring, &byte);
    
    return 0;
}
```

**Bit Operations:**
```c
#include "bit.h"

// Efficient bit manipulation utilities
```

## Documentation

- [eLog Documentation](docs/ELOG.md)
- [Ring Documentation](docs/RING.md)
- [Bit Documentation](docs/BIT.md)

## Directory Structure

```
├── eLog/             # Logging utilities
├── ring/             # Ring buffer implementation
├── bit/              # Bit manipulation utilities
├── docs/             # Documentation
├── examples/         # Usage examples
├── tests/            # Unit tests
└── README.md         # This file
```

## License

This project is dual-licensed under MIT and Apache 2.0 licenses. You may use this software under either license.

- [MIT License](LICENSE)
- [Apache 2.0 License](LICENSE-APACHE)

## Contributing

Contributions are welcome! Please ensure:
- Code follows the existing style
- New utilities include tests
- Documentation is updated
- License headers are maintained

## Requirements

- C99 or later compiler
- CMake 3.10+ (optional, for building)

## Author

Created for embedded firmware development.
