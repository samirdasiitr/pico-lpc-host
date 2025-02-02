#include <stdio.h>
#include "string.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include "pico/types.h"
#include "hardware/pio.h"

#include "lpc_types.h"
#include "lpc_read_controller.pio.h"  // This is auto-generated
#include "lpc_write_controller.pio.h"  // This is auto-generated

// Initialize both state machines for LPC host
void lpc_host_init(PIO pio, uint sm_write, uint sm_read,
                   uint offset_write, uint offset_read,
                   uint pin_lframe, uint pin_lad0, uint pin_lclk,
                   uint pin_sync_ready, float clk_sys) {
    // Configure write SM
    pio_sm_config c_write = lpc_host_write_program_get_default_config(offset_write);
    float div = clk_sys / (2 * 33000000);
    sm_config_set_clkdiv(&c_write, div);
    
    sm_config_set_sideset_pins(&c_write, pin_lclk);
    sm_config_set_out_pins(&c_write, pin_lad0, 4);
    sm_config_set_in_pins(&c_write, pin_lad0);
    sm_config_set_set_pins(&c_write, pin_lframe, 1);
    sm_config_set_jmp_pin(&c_write, pin_sync_ready);
    
    sm_config_set_out_shift(&c_write, true, true, 32);
    pio_sm_init(pio, sm_write, offset_write, &c_write);
    
    // Configure read SM
    pio_sm_config c_read = lpc_host_read_program_get_default_config(offset_read);
    sm_config_set_clkdiv(&c_read, div);
    
    sm_config_set_sideset_pins(&c_read, pin_lclk);
    sm_config_set_out_pins(&c_read, pin_lad0, 4);
    sm_config_set_in_pins(&c_read, pin_lad0);
    sm_config_set_set_pins(&c_read, pin_lframe, 1);
    sm_config_set_jmp_pin(&c_read, pin_sync_ready);
    
    sm_config_set_in_shift(&c_read, true, true, 32);
    pio_sm_init(pio, sm_read, offset_read, &c_read);
    
    // Configure shared pins
    pio_gpio_init(pio, pin_lframe);
    for(int i = 0; i < 4; i++) {
        pio_gpio_init(pio, pin_lad0 + i);
    }
    pio_gpio_init(pio, pin_lclk);
    pio_gpio_init(pio, pin_sync_ready);
    
    // Start both state machines
    pio_sm_set_enabled(pio, sm_write, true);
    pio_sm_set_enabled(pio, sm_read, true);
}

// Helper function to start write transaction
void lpc_write(PIO pio, uint sm, uint8_t cycle_type, 
               uint8_t idsel, uint32_t addr, uint32_t data) {
    uint32_t cmd = (0xF << 28) | ((cycle_type & 0xF) << 24);
    if (cycle_type == LPC_CYCLE_FW) {
        cmd |= ((idsel & 0xF) << 24);  // Override CT with IDSEL for FW
    }
    pio_sm_put_blocking(pio, sm, cmd);
    pio_sm_put_blocking(pio, sm, addr);
    pio_sm_put_blocking(pio, sm, data);
}

// Helper function to start read transaction
uint32_t lpc_read(PIO pio, uint sm, uint8_t cycle_type, 
                  uint8_t idsel, uint32_t addr) {
    uint32_t cmd = (0xF << 28) | ((cycle_type & 0xF) << 24);
    if (cycle_type == LPC_CYCLE_FW) {
        cmd |= ((idsel & 0xF) << 24);  // Override CT with IDSEL for FW
    }
    pio_sm_put_blocking(pio, sm, cmd);
    pio_sm_put_blocking(pio, sm, addr);
    return pio_sm_get_blocking(pio, sm);
}

/*
// LPC host initialization function
void lpc_host_init(PIO pio, uint sm_write, uint sm_read,
                   uint offset_write, uint offset_read,
                   uint pin_lframe, uint pin_lad0, uint pin_lclk,
                   uint pin_sync_ready, float clk_sys) {
    // Configure write state machine
    pio_sm_config c_write = lpc_host_write_program_get_default_config(offset_write);
    
    // Set clock divider for 33MHz LPC bus (assuming clk_sys is system clock frequency)
    float div = clk_sys / (2 * 33000000);
    sm_config_set_clkdiv(&c_write, div);
    
    // Configure pins for write SM
    sm_config_set_sideset_pins(&c_write, pin_lclk);
    sm_config_set_out_pins(&c_write, pin_lad0, 4);  // LAD[3:0]
    sm_config_set_in_pins(&c_write, pin_lad0);      // Same pins for input
    sm_config_set_set_pins(&c_write, pin_lframe, 1); // LFRAME#
    sm_config_set_jmp_pin(&c_write, pin_sync_ready); // SYNC detection
    
    // Configure FIFO for write SM
    sm_config_set_out_shift(&c_write, true, true, 32); // Right shift, autopull
    sm_config_set_in_shift(&c_write, true, true, 32);  // Right shift, autopush
    
    // Configure read state machine
    pio_sm_config c_read = lpc_host_read_program_get_default_config(offset_read);
    sm_config_set_clkdiv(&c_read, div);  // Same clock settings
    
    // Configure pins for read SM (same pin configuration)
    sm_config_set_sideset_pins(&c_read, pin_lclk);
    sm_config_set_out_pins(&c_read, pin_lad0, 4);
    sm_config_set_in_pins(&c_read, pin_lad0);
    sm_config_set_set_pins(&c_read, pin_lframe, 1);
    sm_config_set_jmp_pin(&c_read, pin_sync_ready);
    
    // Configure FIFO for read SM
    sm_config_set_out_shift(&c_read, true, true, 32);
    sm_config_set_in_shift(&c_read, true, true, 32);
    
    // Initialize GPIO pins
    pio_gpio_init(pio, pin_lframe);
    for(int i = 0; i < 4; i++) {
        pio_gpio_init(pio, pin_lad0 + i);  // LAD[3:0]
    }
    pio_gpio_init(pio, pin_lclk);
    pio_gpio_init(pio, pin_sync_ready);
    
    // Set initial pin directions
    pio_sm_set_consecutive_pindirs(pio, sm_write, pin_lframe, 1, true);  // LFRAME output
    pio_sm_set_consecutive_pindirs(pio, sm_write, pin_lad0, 4, true);   // LAD output initially
    pio_sm_set_consecutive_pindirs(pio, sm_write, pin_lclk, 1, true);   // LCLK output
    
    // Initialize state machines
    pio_sm_init(pio, sm_write, offset_write, &c_write);
    pio_sm_init(pio, sm_read, offset_read, &c_read);
    
    // Start both state machines
    pio_sm_set_enabled(pio, sm_write, true);
    pio_sm_set_enabled(pio, sm_read, true);
}
*/