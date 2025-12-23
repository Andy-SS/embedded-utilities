# Embedded C Utilities

A collection of lightweight, modular C utilities for embedded firmware development.

## Features

- **eLog** - Efficient logging system with configurable levels
- **Ring Buffer** - Circular buffer implementation for data streaming
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

### Usage Example

```c
#include "elog.h"
#include "ringbuffer.h"

int main(void) {
    elog_init();
    elog_info("System started");
    
    // Your code here
    
    return 0;
}
```

## Documentation

- [eLog Documentation](docs/ELOG.md)
- [Ring Buffer Documentation](docs/RINGBUFFER.md)

## Directory Structure

```
├── src/              # Source implementations
├── include/          # Public headers
├── tests/            # Unit tests
├── examples/         # Usage examples
└── docs/             # Documentation
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
