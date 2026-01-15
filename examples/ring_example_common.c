/**
 * @file ring_example_common.c
 * @author Andy Chen (clgm216@gmail.com)
 * @version 0.02
 * @date 2025-01-15
 * @brief Ring buffer examples using unified mutex callbacks via common utilities
 * 
 * This example demonstrates:
 * - Ring buffer static initialization
 * - Ring buffer dynamic initialization
 * - Per-instance mutex via common utilities
 * - Producer-consumer patterns with thread safety
 * - Multiple ring buffers with independent mutexes
 * - Ring buffer transfers for batch processing
 */

#include "ring.h"
#include "mutex_common.h"
#include "eLog.h"
#include <stdint.h>
#include <string.h>

/* ========================================================================== */
/* Example 1: Basic Ring Buffer with Static Allocation */
/* ========================================================================== */

#define BUFFER_SIZE 32
uint8_t static_buffer[BUFFER_SIZE];
ring_t static_ring;

/**
 * @brief Initialize ring buffer with pre-allocated buffer
 */
void example_static_ring_init(void) {
    ring_init(&static_ring, static_buffer, BUFFER_SIZE, sizeof(uint8_t));
    ELOG_INFO(ELOG_MD_MAIN, "Static ring buffer initialized: %d elements", BUFFER_SIZE);
}

/**
 * @brief Write to static ring buffer
 */
void example_static_ring_write(void) {
    uint8_t data = 42;
    
    // Write is automatically protected by per-instance mutex
    // (registered via common utilities callbacks)
    ring_result_t result = ring_write(&static_ring, &data);
    
    if (result == RING_OK) {
        ELOG_DEBUG(ELOG_MD_MAIN, "Data written to static ring: %u", data);
    } else if (result == RING_FULL) {
        ELOG_WARNING(ELOG_MD_MAIN, "Static ring buffer is full");
    }
}

/**
 * @brief Read from static ring buffer
 */
void example_static_ring_read(void) {
    uint8_t data;
    
    // Read is automatically protected by per-instance mutex
    ring_result_t result = ring_read(&static_ring, &data);
    
    if (result == RING_OK) {
        ELOG_DEBUG(ELOG_MD_MAIN, "Data read from static ring: %u", data);
    } else if (result == RING_EMPTY) {
        ELOG_INFO(ELOG_MD_MAIN, "Static ring buffer is empty");
    }
}

/* ========================================================================== */
/* Example 2: Ring Buffer with Dynamic Allocation */
/* ========================================================================== */

ring_t dynamic_ring;

/**
 * @brief Initialize ring buffer with runtime allocation
 */
void example_dynamic_ring_init(void) {
    ring_result_t result = ring_init_dynamic(&dynamic_ring, 128, sizeof(uint32_t));
    
    if (result == RING_OK) {
        ELOG_INFO(ELOG_MD_MAIN, "Dynamic ring buffer initialized: 128 elements");
        // ring automatically creates a per-instance mutex
        // via the unified callbacks from common utilities
    } else {
        ELOG_ERROR(ELOG_MD_MAIN, "Failed to initialize dynamic ring: %d", result);
    }
}

/**
 * @brief Cleanup dynamic ring buffer
 */
void example_dynamic_ring_cleanup(void) {
    ring_result_t result = ring_destroy(&dynamic_ring);
    
    if (result == RING_OK) {
        ELOG_INFO(ELOG_MD_MAIN, "Dynamic ring buffer destroyed");
        // Mutex is automatically deleted by ring_destroy()
    } else {
        ELOG_ERROR(ELOG_MD_MAIN, "Failed to destroy ring: %d", result);
    }
}

/* ========================================================================== */
/* Example 3: Sensor Data Ring Buffer - Single Producer */
/* ========================================================================== */

typedef struct {
    uint32_t timestamp;
    float temperature;
    float humidity;
    uint16_t pressure;
} EnvironmentalData;

ring_t sensor_ring;

/**
 * @brief Initialize sensor data ring buffer
 */
void example_sensor_ring_init(void) {
    ring_result_t result = ring_init_dynamic(&sensor_ring, 64, sizeof(EnvironmentalData));
    
    if (result == RING_OK) {
        ELOG_INFO(ELOG_MD_SENSOR, "Sensor ring initialized");
    }
}

/**
 * @brief Sensor producer task - writes environmental data
 * 
 * This function would run in a separate thread.
 * The ring buffer's per-instance mutex ensures thread-safe writes.
 */
void example_sensor_producer_task(ULONG arg) {
    EnvironmentalData data;
    uint32_t sensor_id = (uint32_t)arg;
    
    ELOG_INFO(ELOG_MD_SENSOR, "Sensor producer task %lu started", sensor_id);
    
    while (1) {
        // Collect environmental data
        data.timestamp = tx_time_get();
        data.temperature = 22.5f + (rand() % 50) / 10.0f;  // 22.5 - 27.5°C
        data.humidity = 55.0f + (rand() % 40) / 10.0f;     // 55 - 59% RH
        data.pressure = 1013 + (rand() % 20) - 10;         // 1003 - 1023 hPa
        
        // Write is automatically protected by ring's per-instance mutex
        ring_result_t result = ring_write(&sensor_ring, &data);
        
        if (result == RING_OK) {
            ELOG_DEBUG(ELOG_MD_SENSOR, "Data: T=%.1f°C, H=%.1f%%, P=%u hPa",
                      data.temperature, data.humidity, data.pressure);
        } else if (result == RING_FULL) {
            ELOG_WARNING(ELOG_MD_SENSOR, "Sensor ring buffer full - data loss!");
        }
        
        // Wait 500ms before next reading
        tx_thread_sleep(TX_MS_TO_TICKS(500));
    }
}

/**
 * @brief Data logger task - reads from sensor ring
 * 
 * This function would run in a separate thread.
 * The ring buffer's per-instance mutex ensures thread-safe reads.
 */
void example_sensor_logger_task(ULONG arg) {
    EnvironmentalData data;
    
    ELOG_INFO(ELOG_MD_MAIN, "Sensor logger task started");
    
    tx_thread_sleep(TX_MS_TO_TICKS(100));  // Wait for producer
    
    while (1) {
        // Read is automatically protected by ring's per-instance mutex
        ring_result_t result = ring_read(&sensor_ring, &data);
        
        if (result == RING_OK) {
            ELOG_INFO(ELOG_MD_MAIN, 
                     "LOG[%lu]: T=%.1f°C, H=%.1f%%, P=%u hPa",
                     data.timestamp, data.temperature, data.humidity, data.pressure);
        } else if (result == RING_EMPTY) {
            ELOG_DEBUG(ELOG_MD_MAIN, "Sensor ring empty");
        }
        
        // Read every 1 second
        tx_thread_sleep(TX_MS_TO_TICKS(1000));
    }
}

/* ========================================================================== */
/* Example 4: Multiple Independent Ring Buffers */
/* ========================================================================== */

ring_t command_ring;        // Commands from host
ring_t response_ring;       // Responses to host
ring_t telemetry_ring;      // Sensor telemetry

typedef struct {
    uint8_t cmd_id;
    uint8_t param1;
    uint8_t param2;
} CommandData;

typedef struct {
    uint8_t status;
    uint8_t result;
} ResponseData;

/**
 * @brief Initialize multiple independent ring buffers
 * 
 * Each ring buffer gets its own per-instance mutex via common utilities
 */
void example_multiple_rings_init(void) {
    ring_init_dynamic(&command_ring, 32, sizeof(CommandData));
    ring_init_dynamic(&response_ring, 32, sizeof(ResponseData));
    ring_init_dynamic(&telemetry_ring, 128, sizeof(uint32_t));
    
    ELOG_INFO(ELOG_MD_MAIN, "Created 3 independent ring buffers:");
    ELOG_INFO(ELOG_MD_MAIN, "  - command_ring   (32 items, independent mutex)");
    ELOG_INFO(ELOG_MD_MAIN, "  - response_ring  (32 items, independent mutex)");
    ELOG_INFO(ELOG_MD_MAIN, "  - telemetry_ring (128 items, independent mutex)");
}

/**
 * @brief Demonstrate independent synchronization
 * 
 * Each ring buffer protects its own data independently.
 * No cross-buffer locking needed.
 */
void example_independent_synchronization(void) {
    CommandData cmd;
    ResponseData resp;
    uint32_t telemetry;
    
    // Command ring operates independently
    cmd.cmd_id = 0x01;
    cmd.param1 = 0x10;
    cmd.param2 = 0x20;
    ring_write(&command_ring, &cmd);
    
    // Response ring operates independently
    resp.status = 0;
    resp.result = 1;
    ring_write(&response_ring, &resp);
    
    // Telemetry ring operates independently
    telemetry = 1234567;
    ring_write(&telemetry_ring, &telemetry);
    
    ELOG_INFO(ELOG_MD_MAIN, "Wrote to all 3 rings independently - no cross-mutex contention");
}

/* ========================================================================== */
/* Example 5: Ring Buffer Peek Operations */
/* ========================================================================== */

/**
 * @brief Demonstrate peek without removing data
 * 
 * Peek is also protected by the per-instance mutex
 */
void example_ring_peek(void) {
    uint8_t data;
    ring_t test_ring;
    
    ring_init_dynamic(&test_ring, 16, sizeof(uint8_t));
    
    // Write some data
    uint8_t value = 99;
    ring_write(&test_ring, &value);
    
    // Peek without removing (protected by mutex)
    ring_result_t result = ring_peek(&test_ring, &data);
    
    if (result == RING_OK) {
        ELOG_INFO(ELOG_MD_MAIN, "Peeked data: %u (still in buffer)", data);
    }
    
    // Read removes it (protected by mutex)
    result = ring_read(&test_ring, &data);
    
    if (result == RING_OK) {
        ELOG_INFO(ELOG_MD_MAIN, "Read data: %u (removed from buffer)", data);
    }
    
    ring_destroy(&test_ring);
}

/* ========================================================================== */
/* Example 6: Ring Buffer Status Queries */
/* ========================================================================== */

/**
 * @brief Demonstrate status query operations
 * 
 * All operations are protected by per-instance mutexes via common utilities
 */
void example_ring_status(void) {
    ring_t status_ring;
    ring_init_dynamic(&status_ring, 10, sizeof(uint8_t));
    
    // Query empty status
    if (ring_is_empty(&status_ring)) {
        ELOG_INFO(ELOG_MD_MAIN, "Ring is empty");
    }
    
    // Query full status
    if (ring_is_full(&status_ring)) {
        ELOG_INFO(ELOG_MD_MAIN, "Ring is full");
    }
    
    // Write some data
    for (int i = 0; i < 5; i++) {
        uint8_t val = i;
        ring_write(&status_ring, &val);
    }
    
    // Get current count
    uint32_t count = ring_count(&status_ring);
    ELOG_INFO(ELOG_MD_MAIN, "Ring contains %lu elements", count);
    
    // Get available space
    uint32_t available = ring_available(&status_ring);
    ELOG_INFO(ELOG_MD_MAIN, "Ring has space for %lu more elements", available);
    
    ring_destroy(&status_ring);
}

/* ========================================================================== */
/* Example 7: Ring Buffer Transfers */
/* ========================================================================== */

/**
 * @brief Demonstrate ring-to-ring transfers
 * 
 * Transfer multiple elements between rings in batch
 * Both rings use independent per-instance mutexes
 */
void example_ring_transfer(void) {
    ring_t src_ring, dst_ring;
    
    ring_init_dynamic(&src_ring, 32, sizeof(uint32_t));
    ring_init_dynamic(&dst_ring, 32, sizeof(uint32_t));
    
    // Write to source
    for (int i = 0; i < 10; i++) {
        uint32_t value = 1000 + i;
        ring_write(&src_ring, &value);
    }
    
    ELOG_INFO(ELOG_MD_MAIN, "Source ring: %lu elements", ring_count(&src_ring));
    ELOG_INFO(ELOG_MD_MAIN, "Destination ring: %lu elements", ring_count(&dst_ring));
    
    // Transfer multiple items
    uint32_t transferred = ring_transfer(&src_ring, &dst_ring, 5);
    
    ELOG_INFO(ELOG_MD_MAIN, "Transferred %lu elements", transferred);
    ELOG_INFO(ELOG_MD_MAIN, "Source ring now: %lu elements", ring_count(&src_ring));
    ELOG_INFO(ELOG_MD_MAIN, "Destination ring now: %lu elements", ring_count(&dst_ring));
    
    ring_destroy(&src_ring);
    ring_destroy(&dst_ring);
}

/* ========================================================================== */
/* Example 8: Error Handling and Edge Cases */
/* ========================================================================== */

/**
 * @brief Demonstrate error handling
 */
void example_error_handling(void) {
    ring_t test_ring;
    ring_init_dynamic(&test_ring, 8, sizeof(uint16_t));
    
    // Fill the ring
    for (int i = 0; i < 8; i++) {
        uint16_t value = i;
        ring_write(&test_ring, &value);
    }
    
    // Try to write to full ring
    uint16_t overflow_value = 999;
    ring_result_t result = ring_write(&test_ring, &overflow_value);
    
    if (result == RING_FULL) {
        ELOG_WARNING(ELOG_MD_MAIN, "Ring is full - cannot write more data");
    }
    
    // Read all data
    for (int i = 0; i < 8; i++) {
        uint16_t value;
        ring_read(&test_ring, &value);
    }
    
    // Try to read from empty ring
    uint16_t read_value;
    result = ring_read(&test_ring, &read_value);
    
    if (result == RING_EMPTY) {
        ELOG_INFO(ELOG_MD_MAIN, "Ring is empty - no more data to read");
    }
    
    ring_destroy(&test_ring);
}

/* ========================================================================== */
/* Example 9: Ring Buffer Thread Safety Verification */
/* ========================================================================== */

volatile uint32_t data_written = 0;
volatile uint32_t data_read = 0;
ring_t stress_ring;

/**
 * @brief High-frequency producer task
 */
void example_producer_stress(ULONG arg) {
    for (uint32_t i = 0; i < 10000; i++) {
        uint32_t value = i;
        if (ring_write(&stress_ring, &value) == RING_OK) {
            data_written++;
        }
    }
    ELOG_INFO(ELOG_MD_MAIN, "Producer: wrote %lu items", data_written);
}

/**
 * @brief High-frequency consumer task
 */
void example_consumer_stress(ULONG arg) {
    uint32_t value;
    for (;;) {
        if (ring_read(&stress_ring, &value) == RING_OK) {
            data_read++;
        }
        
        if (data_read >= 10000) {
            break;
        }
    }
    ELOG_INFO(ELOG_MD_MAIN, "Consumer: read %lu items", data_read);
}

/**
 * @brief Run stress test
 */
void example_stress_test(void) {
    ring_init_dynamic(&stress_ring, 256, sizeof(uint32_t));
    
    ELOG_INFO(ELOG_MD_MAIN, "Starting ring buffer stress test");
    ELOG_INFO(ELOG_MD_MAIN, "Producer and consumer will transfer 10k items");
    ELOG_INFO(ELOG_MD_MAIN, "Per-instance mutex ensures data integrity");
    
    // In real application, these would be separate threads
    // Both use the same per-instance mutex automatically
}

/* ========================================================================== */
/* Example 10: Ring Buffer with Different Data Types */
/* ========================================================================== */

typedef struct {
    uint8_t command_type;
    uint16_t parameter;
    uint32_t timestamp;
    char message[32];
} ComplexMessage;

/**
 * @brief Ring buffer with complex data structures
 */
void example_complex_data_ring(void) {
    ring_t msg_ring;
    ring_init_dynamic(&msg_ring, 16, sizeof(ComplexMessage));
    
    // Write complex message
    ComplexMessage msg = {
        .command_type = 1,
        .parameter = 0x1234,
        .timestamp = tx_time_get(),
    };
    strncpy(msg.message, "Hello Ring Buffer!", sizeof(msg.message) - 1);
    
    // Write is protected by per-instance mutex
    ring_write(&msg_ring, &msg);
    ELOG_INFO(ELOG_MD_MAIN, "Complex message written to ring");
    
    // Read complex message
    ComplexMessage read_msg;
    if (ring_read(&msg_ring, &read_msg) == RING_OK) {
        ELOG_INFO(ELOG_MD_MAIN, "Message: %s (param=0x%04X)", 
                 read_msg.message, read_msg.parameter);
    }
    
    ring_destroy(&msg_ring);
}

/* ========================================================================== */
/* Key Takeaways */
/* ========================================================================== */

/**
 * @brief Summary of ring buffer + common utilities integration
 * 
 * 1. Each ring buffer gets its own per-instance mutex automatically
 * 
 * 2. The mutex is created using callbacks registered in common utilities:
 *    - utilities_register_cs_cbs(&mutex_callbacks) registers the ThreadX implementation
 * 
 * 3. All ring operations (read, write, peek, transfer) are automatically protected
 * 
 * 4. Multiple ring buffers operate independently with no cross-buffer contention
 * 
 * 5. Thread safety is transparent to the application code
 * 
 * 6. Graceful fallback to bare-metal mode if RTOS not available
 * 
 * 7. Zero overhead when RTOS not ready or thread safety disabled
 */

#endif  // RING_EXAMPLE_COMMON_C_
