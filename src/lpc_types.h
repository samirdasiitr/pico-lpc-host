#ifndef _LPC_TYPES_H
#define _LPC_TYPES_H

// LPC Cycle Type definitions
#define LPC_CYCLE_MEM      0x0    // Memory cycle
#define LPC_CYCLE_IO       0x1    // I/O cycle
#define LPC_CYCLE_DMA      0x2    // DMA cycle
#define LPC_CYCLE_FW       0x3    // Firmware cycle

// Transfer Size definitions
#define LPC_SIZE_BYTE      0x0    // 8-bit
#define LPC_SIZE_SHORT     0x1    // 16-bit
#define LPC_SIZE_LONG      0x2    // 32-bit

// Sync codes
#define LPC_SYNC_READY     0x0    // Ready
#define LPC_SYNC_SHORT     0x5    // Short Wait Required
#define LPC_SYNC_LONG      0x6    // Long Wait Required
#define LPC_SYNC_ERROR     0x7    // Error

// Direction
#define LPC_DIR_READ       0x0
#define LPC_DIR_WRITE      0x1

uint32_t lpc_read(PIO pio, uint sm, uint8_t cycle_type, 
                  uint8_t idsel, uint32_t addr);

void lpc_write(PIO pio, uint sm, uint8_t cycle_type, 
               uint8_t idsel, uint32_t addr, uint32_t data);
#endif