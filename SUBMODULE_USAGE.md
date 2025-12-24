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

### eLog - Enhanced Logging with Mutex Callbacks

```c
#include "eLog.h"
#include "eLog_mutex_threadx.h"

int main(void) {
    // Register mutex callbacks (ThreadX example)
    elog_register_mutex_callbacks(&threadx_mutex_callbacks);
    
    // Initialize logging
    elog_init();
    
    // Subscribe console output
    elog_subscribe(elog_console_subscriber, ELOG_DEFAULT_THRESHOLD);
    
    // Log messages at different levels
    ELOG_INFO(ELOG_MD_MAIN, "Application started");
    ELOG_DEBUG(ELOG_MD_MAIN, "Debug information");
    ELOG_ERROR(ELOG_MD_MAIN, "Error occurred");
    
    return 0;
}
```

### Ring Buffer - Per-Instance Critical Sections

```c
#include "ring.h"
#include "ring_critical_section_threadx.h"

int main(void) {
    // Register critical section callbacks (ThreadX example)
    ring_register_cs_callbacks(&threadx_cs_callbacks);
    
    // Create ring buffers - each gets its own mutex
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

### ESP32 Application Example

```c
#include "eLog.h"
#include "eLog_mutex_freertos.h"
#include "ring.h"
#include "ring_critical_section_esp32.h"

void app_main(void) {
    // Register eLog mutex callbacks (FreeRTOS)
    elog_register_mutex_callbacks(&freertos_mutex_callbacks);
    elog_init();
    elog_subscribe(elog_console_subscriber, ELOG_DEFAULT_THRESHOLD);
    
    // Register ring buffer callbacks (ESP32 spinlocks)
    ring_register_cs_callbacks(&esp32_cs_callbacks);
    
    ring_t ble_rx_ring;
    ring_init_dynamic(&ble_rx_ring, 512, sizeof(uint8_t));
    
    ELOG_INFO(ELOG_MD_MAIN, "ESP32 app initialized");
    
    ring_destroy(&ble_rx_ring);
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
