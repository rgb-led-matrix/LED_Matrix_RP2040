/* 
 * File:   memory_format.h
 * Author: David Thacher
 * License: GPL 3.0
 */
 
#ifndef MEMORY_FORMAT_H
#define MEMORY_FORMAT_H

    // -- DO NOT EDIT BELOW THIS LINE --
    
    constexpr int MAX_RGB_LED_STEPS = DEFINE_MAX_RGB_LED_STEPS;       // Contrast Ratio - Min RGB constant forward current (Blue LED in my case) in uA divided by min light current in uA
    constexpr int MAX_REFRESH = DEFINE_MAX_REFRESH;
    constexpr float SERIAL_CLOCK = (DEFINE_SERIAL_CLOCK * 1000000.0);
    constexpr int BLANK_TIME = DEFINE_BLANK_TIME;
    constexpr float GCLK = (DEFINE_GCLK * 1000000.0);
    
    #include <math.h>
    constexpr int PWM_bits = round(log2((double) MAX_RGB_LED_STEPS / MULTIPLEX));
    
    typedef uint16_t test2[2 * MULTIPLEX][COLUMNS][3];

#endif

