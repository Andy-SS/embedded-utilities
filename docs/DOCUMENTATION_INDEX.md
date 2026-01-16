# App/utilities Documentation Index

## � Quick Start by Use Case

### ⚠️ **Firmware Hanging After ~10 Characters of Output?**

**[UART_DMA_STDOUT_INTEGRATION.md](UART_DMA_STDOUT_INTEGRATION.md)** - **READ THIS FIRST**
- Explains the 3 failure modes that cause "prints then stops" hangs
- Shows the safe critical section pattern (interrupts ENABLED for ring ops)
- Documents power management during UART DMA TX
- Contains exact code patterns that work

**Then verify with**: [UART_DMA_INTEGRATION_CHECKLIST.md](UART_DMA_INTEGRATION_CHECKLIST.md)
- 8-phase checklist to validate your implementation
- Test cases and diagnostics
- Troubleshooting symptom→cause mapping

**Reference implementation**: [uart_dma_ring_elog_example.c](../examples/uart_dma_ring_elog_example.c)
- Complete working example showing all 6 phases
- Early boot logging, RTOS init, thread-safe usage

---

## 📚 **Core Module Documentation**

### **eLog** - Thread-Safe Logging System  
**[ELOG.md](ELOG.md)** - 730+ lines  
- ✅ Features, configuration, log levels
- ✅ RTOS thread safety and mutex callbacks
- ✅ Multiple subscribers (console, file, buffer)
- ✅ Per-module log thresholds
- ✅ Color ANSI codes for terminal output
  - Color output and location info

#### **Ring Buffer** - Circular FIFO with Per-Instance Mutex
- **[RING.md](RING.md)** - Ring buffer API and design
  - Static and dynamic allocation
  - Per-instance thread-safe critical sections
  - Unified mutex callback interface
  - Producer-consumer patterns

#### **Common Utilities** - Shared RTOS Integration
- **[COMMON.md](COMMON.md)** - Common utility functions
  - Unified mutex callback registration
  - RTOS ready state management
  - Thread-safe memory allocation (ThreadX byte pools)

---

### 🔗 **Integration & Architecture**

- **[INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md)** - 3-step integration
  - Register callbacks
  - Signal RTOS ready
  - Use utilities normally

- **[UNIFIED_MUTEX_GUIDE.md](UNIFIED_MUTEX_GUIDE.md)** - Unified mutex architecture
  - Per-module vs. unified interface
  - Benefits of shared callbacks
  - Implementation patterns

---

### 📋 **Updates & Changes**

- **[RELEASE_NOTES_V1.0.md](RELEASE_NOTES_V1.0.md)** ⭐ *What's New*
  - Stable UART DMA + Ring Buffer pattern
  - Critical bug fixes (mutex deadlock, missed TX kicks, power management)
  - Integration checklist and examples

---

## 🎯 Use Case Navigation

### **"I want to log with eLog"**
1. Read: [ELOG.md](ELOG.md) (quick-start section)
2. Copy: Example code from [ELOG.md](ELOG.md)
3. Integrate: [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md)

### **"I want to use a ring buffer"**
1. Read: [RING.md](RING.md) (initialization section)
2. Copy: Example from [ring_example_common.c](../examples/ring_example_common.c)
3. Setup: Register callbacks via [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md)

### **"I want ring buffer + UART DMA for logging"** ⭐ **Most important**
1. Read: [UART_DMA_STDOUT_INTEGRATION.md](UART_DMA_STDOUT_INTEGRATION.md) (5-10 min)
2. Study: [uart_dma_ring_elog_example.c](../examples/uart_dma_ring_elog_example.c) (20 min)
3. Review: [UART_DMA_INTEGRATION_CHECKLIST.md](UART_DMA_INTEGRATION_CHECKLIST.md) (10 min)
4. Deploy: Use stable pattern from `rsIO.c`

### **"I want thread-safe logging"**
1. Read: [ELOG.md](ELOG.md) (RTOS threading section)
2. Setup: [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md)
3. Reference: [common_example.c](../examples/common_example.c)

### **"I want to understand the architecture"**
1. Read: [README.md](README.md) (overview)
2. Deep dive: [UNIFIED_MUTEX_GUIDE.md](UNIFIED_MUTEX_GUIDE.md)
3. Reference: [COMMON.md](COMMON.md)

---

## 📊 Document Map

```
App/utilities/docs/
├── README.md ← START HERE for overview
├── DOCUMENTATION_INDEX.md ← Navigation hub (You are here)
│
├── UART_DMA_STDOUT_INTEGRATION.md ⭐ START HERE if using printf/eLog over UART+DMA
├── UART_DMA_INTEGRATION_CHECKLIST.md ← Validation & testing
│
├── ELOG.md ← Logging system reference (730+ lines)
├── RING.md ← Ring buffer reference (567+ lines)
├── COMMON.md ← Shared utilities reference (500+ lines)
│
├── INTEGRATION_GUIDE.md ← 3-step unified mutex setup
├── UNIFIED_MUTEX_GUIDE.md ← Architecture deep-dive
│
├── RELEASE_NOTES_V1.0.md ← What's new in v1.0
├── SUBMODULE_COMMIT_NOTES.md ← Version history
└── CLEANUP_SUMMARY.md ← Documentation cleanup log
```

---

## 🚀 Recommended Reading Paths

### **Path 1: Quick Integration (30 min)**
1. [UART_DMA_STDOUT_INTEGRATION.md](UART_DMA_STDOUT_INTEGRATION.md) (10 min)
2. [uart_dma_ring_elog_example.c](../examples/uart_dma_ring_elog_example.c) (10 min)
3. [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) (10 min)

### **Path 2: Production Ready (60 min)**
1. [UART_DMA_STDOUT_INTEGRATION.md](UART_DMA_STDOUT_INTEGRATION.md) (15 min)
2. [uart_dma_ring_elog_example.c](../examples/uart_dma_ring_elog_example.c) (20 min)
3. [UART_DMA_INTEGRATION_CHECKLIST.md](UART_DMA_INTEGRATION_CHECKLIST.md) (20 min)
4. Verify all checklist items before deployment

### **Path 3: Deep Dive (2 hours)**
1. [README.md](README.md) (15 min)
2. [UNIFIED_MUTEX_GUIDE.md](UNIFIED_MUTEX_GUIDE.md) (20 min)
3. [UART_DMA_STDOUT_INTEGRATION.md](UART_DMA_STDOUT_INTEGRATION.md) (20 min)
4. [ELOG.md](ELOG.md) (20 min)
5. [RING.md](RING.md) (20 min)
6. [uart_dma_ring_elog_example.c](../examples/uart_dma_ring_elog_example.c) (20 min)

---

## 🔍 Problem Finder

### **Symptom: "Prints 10 chars then stops"**
→ [UART_DMA_STDOUT_INTEGRATION.md](UART_DMA_STDOUT_INTEGRATION.md#why-this-works)  
→ [UART_DMA_INTEGRATION_CHECKLIST.md](UART_DMA_INTEGRATION_CHECKLIST.md#symptom-prints-10-characters-then-stops)

### **Symptom: "Hangs on early ELOG_INFO"**
→ [UART_DMA_STDOUT_INTEGRATION.md](UART_DMA_STDOUT_INTEGRATION.md#why-this-fixes-prints-10-chars-then-stops)  
→ [uart_dma_ring_elog_example.c](../examples/uart_dma_ring_elog_example.c#phase-5-demonstrating-problem-scenarios-what-not-to-do)

### **Symptom: "Deadlock or system freeze"**
→ [UART_DMA_STDOUT_INTEGRATION.md](UART_DMA_STDOUT_INTEGRATION.md#gotchas-to-avoid)  
→ [uart_dma_ring_elog_example.c](../examples/uart_dma_ring_elog_example.c#wrong-pattern-1-calling-ring--with-primask1)

### **Symptom: "Output corrupted / half-messages"**
→ [UART_DMA_INTEGRATION_CHECKLIST.md](UART_DMA_INTEGRATION_CHECKLIST.md#symptom-output-is-corrupted--half-messages)

### **Symptom: "High power consumption"**
→ [UART_DMA_INTEGRATION_CHECKLIST.md](UART_DMA_INTEGRATION_CHECKLIST.md#symptom-high-power-consumption-during-logging)

---

## 📝 File Purposes

| File | Purpose | Audience | Time |
|------|---------|----------|------|
| README.md | Module overview | Everyone | 5 min |
| DOCUMENTATION_INDEX.md | Navigation hub | Everyone | 5 min |
| UART_DMA_STDOUT_INTEGRATION.md | Safe UART+Ring pattern ⭐ | UART DMA users | 10 min |
| UART_DMA_INTEGRATION_CHECKLIST.md | Pre-deployment validation | UART DMA users | 15 min |
| uart_dma_ring_elog_example.c | Complete working example | Developers | 20 min |
| ELOG.md | Logging API reference | Logging users | 20 min |
| RING.md | Ring buffer API reference | Buffer users | 20 min |
| COMMON.md | Common utilities API | RTOS integrators | 10 min |
| INTEGRATION_GUIDE.md | 3-step RTOS setup | Everyone | 5 min |
| UNIFIED_MUTEX_GUIDE.md | Architecture details | Architects | 20 min |
| RELEASE_NOTES_V1.0.md | What's new in v1.0 | Maintainers | 10 min |
| SUBMODULE_COMMIT_NOTES.md | Version history | Maintainers | 5 min |
| CLEANUP_SUMMARY.md | Doc cleanup log | Maintainers | 5 min |

---

## ✅ Version Info

- **Current Version**: 1.0 Stable Pattern
- **Release Date**: 2025-01-16
- **Status**: ✅ Production Ready
- **Key Fix**: Safe UART DMA + Ring Buffer integration pattern

---

## 🔗 External References

- **Ring Buffer API**: ISO C, no external dependencies
- **eLog**: Standard C library (stdio.h, stdarg.h, string.h)
- **Common**: ThreadX RTOS integration (optional, falls back to no-op if not available)
- **UART/DMA**: STM32 HAL library
