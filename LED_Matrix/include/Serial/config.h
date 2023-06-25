/* 
 * File:   config.h
 * Author: David Thacher
 * License: GPL 3.0
 */
 
#ifndef SERIAL_CONFIG_H
#define SERIAL_CONFIG_H    
    
    // -- DO NOT EDIT BELOW THIS LINE --

    #include <stdint.h>
    #include "../Matrix/config.h"

    #ifndef DEFINE_SERIAL_RGB_TYPE
        #define DEFINE_SERIAL_RGB_TYPE uint8_t
    #endif
    
    typedef DEFINE_SERIAL_RGB_TYPE test[2 * MULTIPLEX][COLUMNS][3];
    
    typedef union {
        test data;
        uint8_t mem[((sizeof(test) / 64) + 1) * 64];
    } packet;

#endif

