/* 
 * File:   worker.cpp
 * Author: David Thacher
 * License: GPL 3.0
 */
 
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "pico/multicore.h"
#include "hardware/irq.h"
#include "Matrix/config.h"
#include "Matrix/matrix.h"
#include "Matrix/TLC5958/memory_format.h"

static uint8_t bank = 1;
volatile bool vsync = false;
static uint16_t index_table[256][3];

// Copied from pico-sdk/src/rp2_common/pico_multicore/multicore.c
//  Allows inlining to RAM func. (Currently linker is copy-to-RAM)
//  May be better to use -ffunction-sections and -fdata-sections with custom linker script
//   I do not want to worry with custom linker script, if possible.
//      .ram_text : {
//          *(.text.multicore_fifo_pop_blocking)
//          *(.text.multicore_fifo_push_blocking)
//          . = ALIGN(4);
//      } > RAM AT> FLASH
//      .text : {
static inline uint32_t __not_in_flash_func(multicore_fifo_pop_blocking_inline)(void) {
    while (!multicore_fifo_rvalid())
        __wfe();
    return sio_hw->fifo_rd;
}

// Copied from pico-sdk/src/rp2_common/pico_multicore/multicore.c
static inline void __not_in_flash_func(multicore_fifo_push_blocking_inline)(uint32_t data) {
    // We wait for the fifo to have some space
    while (!multicore_fifo_wready())
        tight_loop_contents();

    sio_hw->fifo_wr = data;

    // Fire off an event to the other core
    __sev();
}

static void __not_in_flash_func(set_pixel)(uint8_t x, uint8_t y, uint8_t r0, uint8_t g0, uint8_t b0) {
    extern test2 buf[];
    
    buf[bank][y][x % 16][x / 16][0] = index_table[r0][0];
    buf[bank][y][x % 16][x / 16][1] = index_table[g0][1];
    buf[bank][y][x % 16][x / 16][2] = index_table[b0][2];
}

// Rewritten to use super loop. Interrupt rate can be very high, so dedicating a whole CPU core to it.
void __not_in_flash_func(work)() {
    extern void isr_start_core1();
    extern void matrix_gclk_task();
    extern void matrix_fifo_isr_1();
    isr_start_core1();
    
    // This code base consumes the FIFO! (No one else can have it/use it!)
    //  This is true for all Matrix implementations!
    irq_set_exclusive_handler(SIO_IRQ_PROC1, matrix_fifo_isr_1);
    irq_set_priority(SIO_IRQ_PROC1, 0xFF);                                      // Let anything preempt this!
    irq_set_enabled(SIO_IRQ_PROC1, true);   
    
    while(1) {
        matrix_gclk_task();
    }
}

// Serial implementation ISR fires and yields. Core 0 super loop fires, pushes to core 1 FIFO and yields.
//  Core 1 FIFO ISR fires, pushes to core 0 FIFO and yields. This ISR processes the data and yields.
// It is important for this entire process to happen in between serial packets. (Otherwise this is out of spec!)
//  Expectation here is that the FPS rate is very low (30-120) and that the processing time is very low per
//  serial packet. There is a decent amount of overhead here which could be optimized out, however for
//  simplicity and compatibility, I did this barbaric hack.
void __not_in_flash_func(matrix_fifo_isr_0)() {
    if (multicore_fifo_rvalid()) {
        packet *p = (packet *) multicore_fifo_pop_blocking_inline();
        switch (p->cmd) {
            case 0:
                for (int y = 0; y < MULTIPLEX; y++)
                    for (int x = 0; x < COLUMNS; x++)
                        set_pixel(x, y, p->data[y][x][0], p->data[y][x][1], (p->data[y][x][2]);
                bank = (bank + 1) % 3;
                vsync = true;
                break;
            case 1:
                update_index_table((uint8_t *) p->data, p->size, p->offset);
                break;
            default:
                break;
        }
        multicore_fifo_clear_irq();
    }
}

// Barbaric however it allows this to work within the existing code structure!
void __not_in_flash_func(matrix_fifo_isr_1)() {
    if (multicore_fifo_rvalid())
        multicore_fifo_push_blocking_inline(multicore_fifo_pop_blocking_inline());
    multicore_fifo_clear_irq();
}

void update_index_table(uint8_t *buf, uint32_t size, uint32_t offset) {
    if ((size + offset) <= sizeof(index_table))
        memcpy(index_table + offset, buf, size);
}

