#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"

#include "debug.h"
#include "lpc_types.h"
#include "lpc_read_controller.pio.h"  // This is auto-generated
#include "lpc_write_controller.pio.h"  // This is auto-generated

#define UART_ID uart0
#define BAUD_RATE 115200
#define BUFFER_SIZE 256  // Buffer size for each chunk

// Initialize PIO and state machine
// Define pin assignments - adjust these to match your hardware
#define PIN_LRST   1   // LRST
#define PIN_LFRAME 2   // LFRAME# signal
#define PIN_LAD0   3   // First pin of LAD[3:0] bus
#define PIN_LCLK   7   // LPC clock
#define PIN_SYNC   8   // SYNC ready signal

// Initialize both state machines for LPC host
void lpc_host_init(PIO pio, uint sm_write, uint sm_read,
                   uint offset_write, uint offset_read,
                   uint pin_lframe, uint pin_lad0, uint pin_lclk,
                   uint pin_sync_ready, float clk_sys);

// Structure to hold file transfer state
typedef struct {
    uint32_t total_size;     // Total file size
    uint32_t current_addr;   // Current write address
    uint32_t bytes_written;  // Bytes written so far
    uint8_t buffer[BUFFER_SIZE];  // Data buffer
} file_transfer_t;

// Initialize file transfer
void init_file_transfer(file_transfer_t *ft) {
    ft->total_size = 0;
    ft->current_addr = 0;
    ft->bytes_written = 0;
    memset(ft->buffer, 0, BUFFER_SIZE);
}

// Write buffer to LPC slave memory
bool write_chunk_to_lpc(PIO pio, uint sm, uint32_t addr, uint8_t *data, size_t len) {
    uint32_t *word_ptr = (uint32_t *)data;
    size_t words = (len + 3) / 4;  // Round up to nearest word

    for (size_t i = 0; i < words; i++) {
        uint32_t word = word_ptr[i];
        // Start memory write transaction
        lpc_write(pio, sm, LPC_CYCLE_MEM, 0, addr + (i * 4), word);
        
        // Optional: Verify write by reading back
        uint32_t verify = lpc_read(pio, sm, LPC_CYCLE_MEM, 0, addr + (i * 4));
        if (verify != word) {
            return false;  // Write verification failed
        }
    }
    return true;
}

uint16_t get_data_from_pc(uint8_t *buffer, int size) {
  uint16_t buffer_index= 0;

  while (buffer_index < size) {
    int c = getchar_timeout_us(100);
    if (c != PICO_ERROR_TIMEOUT) {
      buffer[buffer_index++] = (c & 0xFF);
    } else {
      break;
    }
  }

  return buffer_index;
}

// Main file transfer function
void handle_file_transfer(PIO pio, uint sm) {
    file_transfer_t ft;
    init_file_transfer(&ft);

    debug("Ready to receive file. Send size (4 bytes) followed by data.\n");
    
    // First receive file size (4 bytes)
    getchar();
    get_data_from_pc((uint8_t*)&ft.total_size, 4);
    debug("File size: %lu bytes\n", ft.total_size);

    // Main transfer loop
    while (ft.bytes_written < ft.total_size) {
        size_t chunk_size = BUFFER_SIZE;
        if (ft.total_size - ft.bytes_written < BUFFER_SIZE) {
            chunk_size = ft.total_size - ft.bytes_written;
        }

        // Read chunk from UART
        get_data_from_pc(ft.buffer, chunk_size);

        // Write chunk to LPC slave
        if (!write_chunk_to_lpc(pio, sm, ft.current_addr, ft.buffer, chunk_size)) {
            debug("Error writing at address 0x%08lx\n", ft.current_addr);
            return;
        }

        // Update progress
        ft.current_addr += chunk_size;
        ft.bytes_written += chunk_size;

        // Send acknowledgment
        putchar('K');  // OK
        
        // Print progress
        debug("Progress: %lu/%lu bytes (%.1f%%)\r", 
               ft.bytes_written, ft.total_size, 
               (float)ft.bytes_written * 100 / ft.total_size);
    }

    debug("\nTransfer complete!\n");
}

// Host script to send file (Python example)
/*
import serial
import struct
import time

def send_file(port, filename):
    with serial.Serial(port, 115200) as ser:
        with open(filename, 'rb') as f:
            # Get file size
            f.seek(0, 2)
            size = f.tell()
            f.seek(0)
            
            # Send file size
            ser.write(struct.pack('<I', size))
            
            # Send file in chunks
            while True:
                chunk = f.read(256)
                if not chunk:
                    break
                    
                ser.write(chunk)
                ack = ser.read(1)  # Wait for acknowledgment
                if ack != b'K':
                    raise Exception("Transfer error")
                    
            print("Transfer complete")
*/

char debug_message_[DEBUG_BUFFER_SIZE];

int main() {
    stdio_init_all();
    
    debug_init(115200, &debug_message_[0], true);
    debug("\n\nRP2350 LPC Host");

    // // Initialize UART
    // uart_init(UART_ID, BAUD_RATE);
    // gpio_set_function(0, GPIO_FUNC_UART);  // TX
    // gpio_set_function(1, GPIO_FUNC_UART);  // RX

    float clk_sys = clock_get_hz(clk_sys);

    // Get PIO instance (can use either pio0 or pio1)
    PIO pio = 0;
    
    // Choose state machine numbers (0-3 available per PIO)
    uint sm_write = 0;
    uint sm_read = 1;

    // Load PIO programs and get offsets
    uint offset_write = pio_add_program(pio, &lpc_host_write_program);
    uint offset_read = pio_add_program(pio, &lpc_host_read_program);
    lpc_host_init(pio, 
                  sm_write, sm_read,           // State machine numbers
                  offset_write, offset_read,    // Program offsets
                  PIN_LFRAME, PIN_LAD0, PIN_LCLK, PIN_SYNC,  // Pin assignments
                  clk_sys);                    // System clock frequency

    while (1) {
        debug("\nWaiting for file transfer...\n");
        handle_file_transfer(pio, sm_write);
    }

    return 0;
}