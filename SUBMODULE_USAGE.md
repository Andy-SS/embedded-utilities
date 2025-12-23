# How to Use EmbeddedUtilities as a Submodule

## Project Structure

EmbeddedUtilities contains:
```
EmbeddedUtilities/
├── CMakeLists.txt          # Main build configuration
├── eLog/
│   ├── CMakeLists.txt
│   ├── eLog.c              # Logging implementation
│   └── eLog.h              # Logging header
├── ring/
│   ├── CMakeLists.txt
│   ├── ring.c              # Ring buffer implementation
│   └── ring.h              # Ring buffer header
├── bit/
│   ├── CMakeLists.txt
│   └── bit_utils.h         # Bit utilities header (header-only)
├── docs/
├── examples/
├── tests/
└── README.md
```

## Setup in Your Project

Your project structure should look like:
```
your-project/
├── CMakeLists.txt
├── src/
│   └── main.c
└── libs/
    └── EmbeddedUtilities/  (git submodule)
```

## In Your Main CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(YourFirmwareProject)

# Add EmbeddedUtilities as subdirectory
add_subdirectory(libs/EmbeddedUtilities)

# Your application target
add_executable(firmware
    src/main.c
    src/app.c
    # ... other source files
)

# Link to individual utilities
target_link_libraries(firmware PRIVATE 
    eLog           # Logging library (static)
    ring           # Ring buffer (static)
    bit            # Bit utilities (header-only)
)

# Alternative: Link to all utilities at once
# target_link_libraries(firmware PRIVATE EmbeddedUtilities)
```

## Available CMake Targets

| Target | Type | Description |
|--------|------|-------------|
| `eLog` | Static Library | Enhanced logging system with multiple subscribers |
| `ring` | Static Library | Circular buffer/ring buffer implementation |
| `bit` | Header-Only | Bit manipulation utilities |
| `EmbeddedUtilities` | Interface | Meta-target including all three utilities |

## Example Usage in Your Code

```c
#include "eLog.h"
#include "ring.h"
#include "bit_utils.h"

int main(void) {
    // Initialize logging with console output
    LOG_INIT_WITH_CONSOLE();
    
    // Log messages at different levels
    ELOG_INFO(ELOG_MD_MAIN, "Application started");
    ELOG_DEBUG(ELOG_MD_MAIN, "Debug information");
    ELOG_ERROR(ELOG_MD_MAIN, "Error occurred");
    
    // Use ring buffer
    // ring_init(&my_buffer);
    // ring_put(&my_buffer, data);
    
    // Use bit utilities
    // bit_set(value, position);
    
    return 0;
}
```

## Adding as Git Submodule

```bash
# Add the submodule to your project
git submodule add <repo-url> libs/EmbeddedUtilities

# Clone your project with submodules
git clone --recurse-submodules <your-project-url>

# Update submodule to latest version
git submodule update --remote
```

## Build Configuration

The main CMakeLists.txt:
- Sets C99 standard (required for embedded development)
- Adds all three utility subdirectories
- Creates an `EmbeddedUtilities` interface target for convenience

Each utility has its own `CMakeLists.txt`:
- **eLog**: Builds a static library from `eLog.c` and `eLog.h`
- **ring**: Builds a static library from `ring.c` and `ring.h`
- **bit**: Header-only interface library

## Notes

- eLog requires bit_utils.h header (included by eLog.h)
- Ring buffer is compiled as a static library
- Bit utilities are header-only, no compilation needed
- All targets are compatible with C99 or later
