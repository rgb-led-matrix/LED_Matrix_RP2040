/* 
 * File:   isr.cpp
 * Author: David Thacher
 * License: GPL 3.0
 */

#include "pico/platform.h"
#include "hardware/irq.h"
#include "Matrix/matrix.h"
#include "Serial/serial.h"

static void __not_in_flash_func(dma_isr0)() {
    matrix_dma_isr();
}

static void __not_in_flash_func(gpio_isr1)() {
    serial_dma_isr();
}

void isr_start() {
    irq_set_exclusive_handler(DMA_IRQ_0, dma_isr0);
    irq_set_priority(DMA_IRQ_0, 1);
    irq_set_enabled(DMA_IRQ_0, true);   
    irq_set_exclusive_handler(IO_IRQ_BANK0, gpio_isr1);
    irq_set_priority(IO_IRQ_BANK0, 0);
    irq_set_enabled(IO_IRQ_BANK0, true);  
}
