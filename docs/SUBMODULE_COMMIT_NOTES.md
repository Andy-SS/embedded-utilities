# Submodule Commit Notes

## App/utilities - Ring Buffer v0.02 Unified Mutex Interface

**Commit Date:** January 15, 2026  
**Status:** Updated - Ring Buffer v0.01 → v0.02  
**Related Changes:** dspProcess.c, rtos.c, rtos.h, CMakeLists.txt

### Summary

Major update to the utilities submodule introducing unified mutex callback interface across eLog and Ring buffer modules, along with ThreadX byte pool memory integration.

### Key Changes

#### 1. Unified Mutex Callback Interface
- **What:** Replaced separate `ring_cs_callbacks_t` and `elog_mutex_callbacks_t` with single `mutex_callbacks_t`
- **Location:** `mutex_common.h`
- **Benefits:**
  - Consistent interface between eLog and Ring
  - Code reuse - single callback implementation for both modules
  - Per-instance mutex support for each ring buffer
  - Timeout support in `acquire()` function

#### 2. Ring Buffer v0.02 Updates
**File:** `ring/ring.h`

**Breaking Changes:**
- `ring_cs_callbacks_t` → `mutex_callbacks_t`
- Callback functions renamed:
  - `ring_cs_enter()` → `acquire(void *mutex, uint32_t timeout_ms)`
  - `ring_cs_exit()` → `release(void *mutex)`
  - `ring_cs_create()` → `create(void)`
  - `ring_cs_destroy()` → `destroy(void *mutex)`

**Features:**
- Per-instance mutexes - each ring buffer gets independent synchronization
- Timeout support for mutex acquisition
- Shared with eLog - no duplicate callback code

#### 3. ThreadX Memory Integration
**Files:** `rtos.h`, `rtos.c`

**New Functions:**
```c
void* byte_allocate(size_t size);      // Allocate from ThreadX byte pool
void byte_release(void *ptr);           // Release to ThreadX byte pool
```

**Implementation Details:**
- Allocations managed through `tx_app_byte_pool`
- Provides predictable memory usage for embedded systems
- Integrated error handling and logging

#### 4. Documentation Updates
**New/Updated Files:**
- `UNIFIED_MUTEX_GUIDE.md` - Updated with unified interface details
- `RING_USAGE.md` - New comprehensive ring buffer usage guide (v0.02)
- `README.md` - Updated with ThreadX integration and memory allocation info
- `docs/RING.md` - Updated with unified callback examples

### Application Integration Changes

#### App/rtos.c
**Changes:**
- Replaced separate eLog and ring-specific mutex implementations with unified `mutex_callbacks_t`
- Updated `byte_allocate()` and `byte_release()` implementations to use ThreadX byte pool
- Changed from `malloc()/free()` to `byte_allocate()/byte_release()`
- Simplified callback registration - one `mutex_callbacks_t` for both eLog and Ring

**Callbacks Provided:**
```c
const mutex_callbacks_t mutex_callbacks = {
    .create = threadx_mutex_create,
    .destroy = threadx_mutex_destroy,
    .acquire = threadx_mutex_acquire,      // NEW: with timeout
    .release = threadx_mutex_release
};
```

#### App/Tasks/dspProcess.c
**Changes:**
- Replaced `malloc()` → `byte_allocate()`
- Replaced `free()` → `byte_release()`
- Added `#include "rtos.h"` for memory functions
- Applied to:
  - Median filter window allocation
  - Polynomial fitting matrix allocations (7 instances)

#### cmake/App/CMakeLists.txt
**Changes:**
- Added `$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/App/utilities>` to include paths
- Enables direct inclusion of `mutex_common.h` from utilities

### Migration Path (for existing code)

**Old Code (v0.01):**
```c
ring_register_cs_callbacks(&threadx_cs_callbacks);
```

**New Code (v0.02):**
```c
extern const mutex_callbacks_t mutex_callbacks;
ring_register_cs_callbacks(&mutex_callbacks);
```

**Memory Allocation (Old):**
```c
float32_t *buffer = (float32_t *)malloc(size * sizeof(float32_t));
// use buffer
free(buffer);
```

**Memory Allocation (New):**
```c
float32_t *buffer = (float32_t *)byte_allocate(size * sizeof(float32_t));
// use buffer
byte_release(buffer);
```

### Testing Recommendations

1. **Ring Buffer Operations:**
   - Verify per-instance mutex creation/destruction
   - Test concurrent access from multiple threads
   - Validate timeout behavior on mutex acquisition

2. **Memory Allocation:**
   - Check ThreadX byte pool usage with `debug_memory_statistics()`
   - Verify all `byte_allocate()` calls have matching `byte_release()`
   - Test under various memory pressure scenarios

3. **DSP Processing:**
   - Verify polynomial fitting with new memory allocation
   - Validate median filter operations
   - Check for memory leaks with ThreadX debugging enabled

4. **Integration:**
   - Verify eLog and Ring share callbacks without conflicts
   - Test simultaneous operations on multiple ring buffers
   - Validate system stability under load

### Backward Compatibility

⚠️ **Breaking Changes:**
- Code using `ring_cs_callbacks_t` must be updated to use `mutex_callbacks_t`
- Applications using old callback names need function name updates
- `malloc()` / `free()` replaced with ThreadX byte pool functions

### Files Changed in Submodule

```
App/utilities/
├── mutex_common.h              (Updated - unified interface)
├── UNIFIED_MUTEX_GUIDE.md      (Updated - detailed guide)
├── RING_USAGE.md               (NEW - comprehensive usage guide)
├── README.md                   (Updated - ThreadX integration info)
├── ring/
│   ├── ring.h                  (Updated - v0.02 changes)
│   └── ring.c                  (Updated - unified callbacks)
└── docs/
    ├── RING.md                 (Updated - unified callback examples)
    └── ELOG.md                 (Updated - shared interface)
```

### Performance Impact

**Improvements:**
- No global mutex contention - each ring buffer has independent lock
- Timeout support prevents indefinite blocking
- ThreadX byte pool integration improves memory fragmentation

**Memory Overhead:**
- Per-instance mutex: ~40-60 bytes (platform-dependent)
- No additional overhead vs v0.01 per ring buffer

### Related Issues/Features

- Thread-safe ring buffer with per-instance mutexes
- Unified RTOS callback interface
- ThreadX memory pool integration
- DSP pipeline improvements

### Notes

- The submodule is marked with `-dirty` due to untracked/modified content
- Review changes within submodule before finalizing commit
- Consider cherry-picking if only specific utilities are needed
- Ensure all dependent code updated to use new interface
