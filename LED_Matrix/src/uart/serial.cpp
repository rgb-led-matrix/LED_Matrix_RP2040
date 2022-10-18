/* 
 * File:   serial.cpp
 * Author: David Thacher
 * License: GPL 3.0
 */
 
#include <stdint.h>
#include "pico/multicore.h"
#include "hardware/dma.h"
#include "Matrix/config.h"
#include "Matrix/matrix.h"
#include "Serial/serial_uart/serial_uart.h"

static volatile bool isReady;
static packet buffers[2];
static volatile uint8_t buffer = 0;
static int serial_dma_chan;

void serial_task() {  

#ifndef TEST
    for (int x = 0; x < COLUMNS; x++) {
        for (int y = 0; y < (2 * MULTIPLEX); y++) {
            if ((x % (2 * MULTIPLEX)) == y) {
                buffers[buffer].data[y][x][0] = 0;
                buffers[buffer].data[y][x][1] = 0;
                buffers[buffer].data[y][x][2] = 0;
            }
            else {
                buffers[buffer].data[y][x][0] = 0xFF;
                buffers[buffer].data[y][x][1] = 0xFF;
                buffers[buffer].data[y][x][2] = 0xFF;
            }
        }
    }
    isReady = true;
    buffers[buffer].cmd = 0;
    buffer = (buffer + 1) % 2;
#endif

    if (isReady) {    
        multicore_fifo_push_blocking((uint32_t) &buffers[(buffer + 1) % 2]);
        isReady = false;
        serial_uart_print("Received\n");
    }
}

static void __not_in_flash_func(callback)(uint8_t **buf, uint16_t *len) {
    *buf = (uint8_t *) &buffers[(buffer + 1) % 2];
    *len = sizeof(packet);
    isReady = true;
    buffer = (buffer + 1) % 2;
}

void __not_in_flash_func(serial_dma_isr)() {
    serial_uart_isr();
}

void serial_start() {
    multicore_launch_core1(work);
    
    isReady = false;
    serial_dma_chan = dma_claim_unused_channel(true);
    serial_uart_start(&callback, serial_dma_chan); 
}

