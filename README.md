# Embedded C Utilities

A collection of lightweight, modular C utilities for embedded firmware development.

## Features

- **eLog** - Efficient logging system with configurable levels
- **Ring** - Circular buffer implementation for data streaming
- **Bit** - Bit manipulation utilities
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

### Quick Usage

**Logging with eLog:**
```c
#include "eLog.h"

int main(void) {
    LOG_INIT_WITH_CONSOLE();
    
    ELOG_INFO(ELOG_MD_MAIN, "System started");
    ELOG_DEBUG(ELOG_MD_MAIN, "Debug message");
    ELOG_ERROR(ELOG_MD_MAIN, "Error occurred");
    
    return 0;
}
```

See [eLog documentation](docs/ELOG.md) for advanced features like module-based logging, multiple log levels, subscribers, and thread safety.

**Ring Buffer:**
```c
#include "ring.h"

// Create and use circular buffer for data streaming
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
