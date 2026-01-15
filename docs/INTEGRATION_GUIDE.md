# Common Utilities Integration Guide

## Quick Start Summary

You have successfully integrated unified mutex support across all embedded utilities modules. Here's what changed and how to use it:

## What Changed

### Before
- eLog had its own mutex interface
- Ring had its own mutex interface  
- Different callback structures for each module
- Code duplication in RTOS implementations

### After
- **Unified `mutex_callbacks_t` interface** in `mutex_common.h`
- **Common utility functions** in `common.c` for centralized RTOS management
- **CMake integration** automatically exposes `common.c` to all modules
- **Per-instance mutexes** for Ring buffers via unified callbacks
- **Single ThreadX implementation** shared by all modules

## File Structure

```
App/utilities/
├── mutex_common.h              # Unified interface definition
├── common.c                    # Implementation of utilities
├── common.h                    # Utilities function declarations
├── CMakeLists.txt             # Builds common.c + all submodules
│
├── eLog/
│   ├── eLog.h
│   ├── eLog.c
│   └── CMakeLists.txt         # Links against common
│
├── ring/
│   ├── ring.h
│   ├── ring.c
│   └── CMakeLists.txt         # Links against common
│
├── bit/
│   └── [bit utilities]
│
├── docs/
│   ├── README.md              # UPDATED - Overview with common.c
│   ├── COMMON.md              # NEW - Common utilities API reference
│   ├── ELOG.md                # UPDATED - Thread safety section
│   ├── RING.md                # UPDATED - Unified mutex section
│   └── UNIFIED_MUTEX_GUIDE.md # Architecture overview
│
└── examples/
    ├── common_example.c       # NEW - Common utilities usage
    ├── eLog_example_common.c  # NEW - eLog with common utilities
    ├── ring_example_common.c  # NEW - Ring with common utilities
    └── eLog/
        └── [existing eLog examples]
```

## 3-Step Integration

### Step 1: Register Callbacks

In `tx_application_define()` in your RTOS initialization:

```c
#include "mutex_common.h"

extern const mutex_callbacks_t mutex_callbacks;  // From rtos.c

void tx_application_define(VOID *first_unused_memory) {
    // ... Create threads and other RTOS objects ...
    
    // Register unified mutex callbacks
    utilities_register_cs_cbs(&mutex_callbacks);
    
    // ... rest of initialization ...
}
```

### Step 2: Signal RTOS Ready

After ThreadX scheduler setup:

```c
// Signal utilities that RTOS is ready
utilities_set_RTOS_ready(true);

ELOG_INFO(ELOG_MD_MAIN, "RTOS started - thread-safe mode enabled");
```

### Step 3: Use Utilities Normally

All thread safety is now automatic:

```c
// eLog automatically uses unified mutex
ELOG_INFO(ELOG_MD_MAIN, "This is thread-safe");

// Ring automatically gets per-instance mutex
ring_t my_ring;
ring_init_dynamic(&my_ring, 256, sizeof(uint8_t));
ring_write(&my_ring, &data);  // Automatically mutex-protected
```

## API Reference

All functions are in `mutex_common.h` and `common.c`:

### Initialization
- `utilities_register_cs_cbs()` - Register RTOS callbacks
- `utilities_set_RTOS_ready()` - Signal RTOS started
- `utilities_is_RTOS_ready()` - Check if RTOS active

### Advanced (Usually Automatic)
- `utilities_mutex_create()` - Create a mutex
- `utilities_mutex_take()` - Lock a mutex
- `utilities_mutex_give()` - Unlock a mutex
- `utilities_mutex_delete()` - Destroy a mutex

## CMake Configuration

### Updated cmake/App/CMakeLists.txt

```cmake
# Add utilities subdirectory (now includes common.c)
add_subdirectory(${CMAKE_SOURCE_DIR}/App/utilities ${CMAKE_BINARY_DIR}/App/utilities)

# Link to application
target_link_libraries(App INTERFACE
    common    # <-- NEW: Links common.c
    eLog      # Uses common.c automatically
    ring      # Uses common.c automatically
    stm32cubemx
)
```

### Updated App/utilities/CMakeLists.txt

```cmake
# Create common library
add_library(common STATIC common.c)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link common to modules
target_link_libraries(eLog PUBLIC common)
target_link_libraries(ring PUBLIC common)

# Add subdirectories
add_subdirectory(eLog)
add_subdirectory(ring)
add_subdirectory(bit)
```

## ThreadX Implementation

Your ThreadX mutex callbacks (typically in `rtos.c`):

```c
#include "mutex_common.h"
#include "tx_api.h"

// ThreadX mutex implementation
const mutex_callbacks_t mutex_callbacks = {
    .create = threadx_mutex_create,      // Returns new TX_MUTEX handle
    .destroy = threadx_mutex_destroy,    // Deletes TX_MUTEX
    .acquire = threadx_mutex_acquire,    // tx_mutex_get with timeout
    .release = threadx_mutex_release     // tx_mutex_put
};
```

See [COMMON.md](COMMON.md) for the complete implementation example.

## Examples

### Common Utilities Usage
See `examples/common_example.c` for:
- Manual mutex creation
- RTOS ready state management
- Error handling
- Bare-metal fallback

### eLog Integration
See `examples/eLog_example_common.c` for:
- Thread-safe multi-task logging
- Per-module log levels
- Multiple subscribers
- Format strings

### Ring Buffer Integration
See `examples/ring_example_common.c` for:
- Per-instance mutex
- Producer-consumer pattern
- Ring transfers
- Independent synchronization

## Benefits

| Feature | Benefit |
|---------|---------|
| **Unified Interface** | Single `mutex_callbacks_t` for all modules |
| **Code Reuse** | One ThreadX implementation for everything |
| **Per-Instance Mutexes** | Each ring buffer has its own mutex |
| **Automatic** | Thread safety is transparent to application |
| **Efficient** | Zero overhead when RTOS not ready |
| **Reliable** | Clean, ungarbled output from multiple threads |
| **Flexible** | Works with or without RTOS |
| **Compatible** | No breaking changes to existing code |

## Documentation Files

| File | Purpose |
|------|---------|
| [README.md](README.md) | Overview and quick start |
| [COMMON.md](COMMON.md) | Common utilities API reference |
| [ELOG.md](ELOG.md) | eLog detailed documentation |
| [RING.md](RING.md) | Ring buffer documentation |
| [UNIFIED_MUTEX_GUIDE.md](UNIFIED_MUTEX_GUIDE.md) | Architecture details |

## Troubleshooting

### Mutex operations fail
**Cause:** Callbacks not registered  
**Solution:** Call `utilities_register_cs_cbs(&mutex_callbacks)` in `tx_application_define()`

### RTOS_READY always false
**Cause:** Ready flag not set  
**Solution:** Call `utilities_set_RTOS_ready(true)` after ThreadX setup

### Ring initialization fails
**Cause:** Callbacks registered before RTOS ready  
**Solution:** Ensure both registration and ready flag set before creating ring buffers

### Timeouts on mutex
**Cause:** Deadlock or high contention  
**Solution:** Increase timeout or reduce critical section time

## Best Practices

1. **Register callbacks early** - In `tx_application_define()` before creating threads
2. **Set RTOS ready flag** - Immediately after ThreadX scheduler setup
3. **Use existing examples** - Common utilities handle mutex management automatically
4. **Check RTOS status** - Use `utilities_is_RTOS_ready()` when needed
5. **Test bare-metal** - Ensure graceful degradation without RTOS

## Migration from Old Interface

If you had separate implementations:

### Old eLog Interface
```c
elog_register_mutex_callbacks(&old_elog_callbacks);
elog_update_RTOS_ready(true);
```

### New Unified Interface
```c
utilities_register_cs_cbs(&mutex_callbacks);    // Covers all modules
utilities_set_RTOS_ready(true);
```

That's it! eLog and Ring now both use the same callbacks.

## Next Steps

1. Review [COMMON.md](COMMON.md) for complete API reference
2. Check `examples/common_example.c` for usage patterns
3. Update your initialization code (3 steps above)
4. Rebuild and test

## Support & Resources

- **Common Utilities API**: See [COMMON.md](COMMON.md)
- **Architecture**: See [UNIFIED_MUTEX_GUIDE.md](UNIFIED_MUTEX_GUIDE.md)
- **eLog Details**: See [ELOG.md](ELOG.md)
- **Ring Details**: See [RING.md](RING.md)
- **Examples**: See `examples/` directory

---

**Summary**: You now have a unified, efficient, thread-safe foundation for all embedded utilities. All modules share the same mutex implementation with zero code duplication and automatic per-instance synchronization.
