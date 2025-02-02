#include <stdio.h>
#include "string.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include "pico/types.h"
#include "hardware/pio.h"

#include "lpc_types.h"
#include "lpc_io_read_controller.pio.h"  // This is auto-generated
#include "lpc_io_write_controller.pio.h"  // This is auto-generated

// Initialize I/O controllers
static inline void lpc_io_init(PIO pio, uint sm_write, uint sm_read,
                              uint offset_write, uint offset_read,
                              uint pin_lframe, uint pin_lad0, uint pin_lclk,
                              uint pin_sync_ready, float clk_sys) {
    // Configure write SM
    pio_sm_config c_write = lpc_io_write_program_get_default_config(offset_write);
    
    // Set clock divider for 33MHz operation
    float div = clk_sys / (2 * 33000000);
    sm_config_set_clkdiv(&c_write, div);
    
    // Configure pins for write SM
    sm_config_set_sideset_pins(&c_write, pin_lclk);
    sm_config_set_out_pins(&c_write, pin_lad0, 4);
    sm_config_set_in_pins(&c_write, pin_lad0);
    sm_config_set_set_pins(&c_write, pin_lframe, 1);
    sm_config_set_jmp_pin(&c_write, pin_sync_ready);
    
    // Configure read SM
    pio_sm_config c_read = lpc_io_read_program_get_default_config(offset_read);
    sm_config_set_clkdiv(&c_read, div);
    
    sm_config_set_sideset_pins(&c_read, pin_lclk);
    sm_config_set_out_pins(&c_read, pin_lad0, 4);
    sm_config_set_in_pins(&c_read, pin_lad0);
    sm_config_set_set_pins(&c_read, pin_lframe, 1);
    sm_config_set_jmp_pin(&c_read, pin_sync_ready);
    
    // Configure shared pins
    pio_gpio_init(pio, pin_lframe);
    for(int i = 0; i < 4; i++) {
        pio_gpio_init(pio, pin_lad0 + i);
    }
    pio_gpio_init(pio, pin_lclk);
    pio_gpio_init(pio, pin_sync_ready);
    
    // Initialize and start state machines
    pio_sm_init(pio, sm_write, offset_write, &c_write);
    pio_sm_init(pio, sm_read, offset_read, &c_read);
    
    pio_sm_set_enabled(pio, sm_write, true);
    pio_sm_set_enabled(pio, sm_read, true);
}

// Helper function for I/O write
static inline void lpc_io_write_byte(PIO pio, uint sm, uint16_t port, uint8_t data) {
    pio_sm_put_blocking(pio, sm, ((uint32_t)0xF << 28) | port);
    pio_sm_put_blocking(pio, sm, data);
}

// Helper function for I/O read
static inline uint8_t lpc_io_read_byte(PIO pio, uint sm, uint16_t port) {
    pio_sm_put_blocking(pio, sm, ((uint32_t)0xF << 28) | port);
    return (uint8_t)pio_sm_get_blocking(pio, sm);
}

// Example usage
void test_lpc_io(PIO pio, uint sm_write, uint sm_read) {
    // Read keyboard controller status (port 0x64)
    uint8_t kbd_status = lpc_io_read_byte(pio, sm_read, 0x64);
    
    // Write to keyboard controller command port
    if (!(kbd_status & 0x02)) { // Check if input buffer is empty
        lpc_io_write_byte(pio, sm_write, 0x64, 0xAA); // Self-test command
    }
}