# How to Use EmbeddedUtilities as a Submodule

## Project Structure

EmbeddedUtilities contains:
```
EmbeddedUtilities/
├── CMakeLists.txt              # Main build configuration
├── mutex_common.h              # Unified mutex callback interface (NEW!)
├── eLog/
│   ├── CMakeLists.txt
│   ├── eLog.c                  # Logging implementation
│   └── eLog.h                  # Logging header
├── ring/
│   ├── CMakeLists.txt
│   ├── ring.c                  # Ring buffer implementation
│   └── ring.h                  # Ring buffer header
├── bit/
│   ├── CMakeLists.txt
│   └── bit_utils.h             # Bit utilities header (header-only)
├── docs/
├── examples/
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
| `eLog` | Static Library | Enhanced logging with callback-based mutex support |
| `ring` | Static Library | Ring buffer with per-instance critical sections |
| `bit` | Header-Only | Bit manipulation utilities |
| `eLog_mutex_threadx` | Optional | ThreadX mutex callbacks for eLog |
| `ring_critical_section_threadx` | Optional | ThreadX critical section callbacks for ring |
| `eLog_mutex_freertos` | Optional | FreeRTOS mutex callbacks for eLog |
| `ring_critical_section_freertos` | Optional | FreeRTOS critical section callbacks for ring |
| `ring_critical_section_esp32` | Optional | ESP32 spinlock callbacks for ring |
| `EmbeddedUtilities` | Interface | Meta-target including core utilities |

## Example Usage in Your Code

### Unified Mutex Callbacks (NEW!)

Both eLog and Ring now use the same `mutex_callbacks_t` interface from `mutex_common.h`:

```c
#include "mutex_common.h"

/* Example implementation for ThreadX */
mutex_callbacks_t threadx_callbacks = {
  .create = threadx_mutex_create,
  .destroy = threadx_mutex_destroy,
  .acquire = threadx_mutex_acquire,  /* Supports timeout_ms parameter */
  .release = threadx_mutex_release
};
```

### eLog - Enhanced Logging with Unified Mutex

```c
#include "eLog.h"
#include "mutex_common.h"

/* ThreadX mutex implementation */
extern const mutex_callbacks_t threadx_callbacks;

int main(void) {
    // Register mutex callbacks (both eLog and Ring use same callbacks!)
    elog_register_mutex_callbacks(&threadx_callbacks);
    elog_update_RTOS_ready(true);
    
    // Initialize logging
    LOG_INIT_WITH_CONSOLE();
    
    // Log messages at different levels
    ELOG_INFO(ELOG_MD_MAIN, "Application started");
    ELOG_DEBUG(ELOG_MD_MAIN, "Debug information");
    ELOG_ERROR(ELOG_MD_MAIN, "Error occurred");
    
    return 0;
}
```

### Ring Buffer - Per-Instance Synchronization with Unified Mutex

```c
#include "ring.h"
#include "mutex_common.h"

/* ThreadX mutex implementation - same as eLog! */
extern const mutex_callbacks_t threadx_callbacks;

int main(void) {
    // Register critical section callbacks (same interface!)
    ring_register_cs_callbacks(&threadx_callbacks);
    
    // Create ring buffers - each gets its own mutex automatically
    ring_t uart_rx_ring;
    ring_t sensor_data_ring;
    
    ring_init_dynamic(&uart_rx_ring, 256, sizeof(uint8_t));
    ring_init_dynamic(&sensor_data_ring, 128, sizeof(sensor_t));
    
    // Use ring buffers - synchronization is automatic
    uint8_t byte = 0xAB;
    ring_write(&uart_rx_ring, &byte);
    
    sensor_t reading = {0};
    ring_write(&sensor_data_ring, &reading);
    
    // Cleanup
    ring_destroy(&uart_rx_ring);
    ring_destroy(&sensor_data_ring);
    
    return 0;
}
```

### Bare Metal Example (No RTOS)

```c
#include "eLog.h"
#include "ring.h"

int main(void) {
    // Skip mutex registration for bare metal
    // or pass NULL if function allows
    LOG_INIT_WITH_CONSOLE();
    
    // Both modules work without thread safety
    ELOG_INFO(ELOG_MD_MAIN, "Single-threaded application");
    
    ring_t data_ring;
    ring_init_dynamic(&data_ring, 256, sizeof(uint8_t));
    
    return 0;
}
```

### Multi-Module Integration Example

```c
#include "eLog.h"
#include "ring.h"
#include "mutex_common.h"
#include "tx_api.h"  /* ThreadX only */

/* ThreadX mutex callbacks implementation */
extern const mutex_callbacks_t threadx_callbacks;

void application_init(void) {
    // Register callbacks once - used by both modules!
    elog_register_mutex_callbacks(&threadx_callbacks);
    ring_register_cs_callbacks(&threadx_callbacks);
    
    // Mark eLog ready for RTOS operations
    elog_update_RTOS_ready(true);
    
    // Initialize both modules
    LOG_INIT_WITH_CONSOLE();
    
    // Create ring buffers
    ring_t uart_buffer;
    ring_t sensor_buffer;
    
    ring_init_dynamic(&uart_buffer, 512, sizeof(uint8_t));
    ring_init_dynamic(&sensor_buffer, 256, sizeof(uint16_t));
    
    ELOG_INFO(ELOG_MD_MAIN, "Both eLog and Ring are thread-safe!");
    
    // Both modules now use ThreadX mutexes automatically
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

Core utilities:
- **eLog**: Static library with platform-independent mutex callback API
- **ring**: Static library with per-instance critical section callbacks
- **bit**: Header-only interface library

Platform-specific helpers (optional):
- **eLog mutex callbacks**: Separate files for ThreadX and FreeRTOS
- **Ring critical section callbacks**: Separate files for ThreadX, FreeRTOS, and ESP32
- Include only the callbacks your platform needs

## Key Changes from Previous Version

### eLog Refactoring
- ✅ Removed hard RTOS dependencies (no tx_api.h in eLog.c)
- ✅ Added `elog_register_mutex_callbacks()` API
- ✅ Mutex operations now via callbacks
- ✅ Each eLog instance can have its own mutex
- ⚠️ Must register callbacks before calling `elog_init()`

### Ring Buffer Refactoring
- ✅ Removed global critical sections
- ✅ Each ring buffer gets its own per-instance mutex
- ✅ Added `ring_register_cs_callbacks()` API
- ✅ Automatic mutex creation/destruction during Init/Destroy
- ✅ ISR-safe operations (especially on ESP32)
- ⚠️ Must register callbacks before creating ring buffers

## Notes

- eLog and ring are now completely platform-independent
- Callback registration is optional for bare metal (pass NULL)
- Each module can support multiple platforms at the same time
- Platform-specific callback implementations are in separate files
- Thread safety is automatic once callbacks are registered
- No mutex contention between different eLog instances or ring buffers
