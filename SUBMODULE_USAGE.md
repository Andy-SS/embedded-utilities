# How to Use EmbeddedUtilities as a Submodule

## Setup

Assuming the utils repository is in a subdirectory like:
```
your-project/
├── CMakeLists.txt
├── src/
└── libs/
    └── utils/  (EmbeddedUtilities submodule)
```

## In Your Main CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(YourFirmwareProject)

# Add utilities as subdirectory
add_subdirectory(libs/utils)

# Your application target
add_executable(firmware
    src/main.c
    src/app.c
    # ... other source files
)

# Link to individual utilities
target_link_libraries(firmware PRIVATE 
    eLog    # Logging library
    ring    # Ring buffer
    bit     # Bit utilities
)

# Or link to all utilities at once
# target_link_libraries(firmware PRIVATE EmbeddedUtilities)
```

## Available Targets

- `eLog` - Logging library (static)
- `ring` - Ring buffer (header-only)
- `bit` - Bit utilities (header-only)
- `EmbeddedUtilities` - Interface target that includes all three

## Example Usage in Your Code

```c
#include "eLog.h"
#include "ring.h"
#include "bit_utils.h"

int main(void) {
    LOG_INIT_WITH_CONSOLE();
    ELOG_INFO(ELOG_MD_MAIN, "Application started");
    
    return 0;
}
```

## Git Submodule Setup

```bash
# Add as submodule
git submodule add <repo-url> libs/utils

# Clone with submodule
git clone --recurse-submodules <your-project-url>

# Update submodule
git submodule update --remote
```
