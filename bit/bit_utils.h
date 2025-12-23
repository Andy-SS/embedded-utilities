#ifndef BIT_UTILS_H
#define BIT_UTILS_H

/* Utility macros for bit manipulation */
#ifndef SET_BIT
#define SET_BIT(reg, bit)   ((reg) |= (1 << (bit)))  // Set a specific bit in a register
#endif

#ifndef CLEAR_BIT
#define CLEAR_BIT(reg, bit) ((reg) &= ~(1 << (bit))) // Clear a specific bit in a register
#endif

#ifndef TOGGLE_BIT
#define TOGGLE_BIT(reg, bit) ((reg) ^= (1 << (bit))) // Toggle a specific bit in a register
#endif

#ifndef READ_BIT
#define READ_BIT(reg, bit)  ((reg) & (1 << (bit)))   // Read a specific bit from a register
#endif

#ifndef BIT
#define BIT(x) (1 << (x)) // Create a bit mask
#endif

/* Endian swap macros */
#ifndef SWAP16
#define SWAP16(x) (uint16_t)((((x) & 0xFF00) >> 8) | (((x) & 0x00FF) << 8))
#endif

#ifndef SWAP32
#define SWAP32(x) (uint32_t)((((x) & 0xFF000000) >> 24) | \
                             (((x) & 0x00FF0000) >> 8)  | \
                             (((x) & 0x0000FF00) << 8)  | \
                             (((x) & 0x000000FF) << 24))
#endif

#endif /* BIT_UTILS_H */
