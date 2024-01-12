/* 
 * File:   loafer.cpp
 * Author: David Thacher
 * License: GPL 3.0
 */
 
#include <stdint.h>
#include "pico/multicore.h"
#include "Serial/config.h"
#include "Matrix/matrix.h"
#include "Matrix/S_PWM/memory_format.h"

namespace Matrix {
    extern test2 buf[];
}

namespace Matrix::Worker {
    extern volatile bool vsync;
}

namespace Matrix::Loafer {
    void __not_in_flash_func(toss)(void *arg) {
        while(Worker::vsync);
        Worker::vsync = true;
    }

    void *__not_in_flash_func(get_back_buffer)() {
        static uint8_t bank = 1;  
        
        // TODO:
        
        bank = (bank + 1) % Serial::num_framebuffers;
        return nullptr;
    }
}
