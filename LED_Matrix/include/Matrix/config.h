/* 
 * File:   config.h
 * Author: David Thacher
 * License: GPL 3.0
 */
 
#ifndef CONFIG_H
#define CONFIG_H    
    
    // -- DO NOT EDIT BELOW THIS LINE --

    #ifndef DEFINE_FPS
    #define DEFINE_FPS 30
    #endif
    #ifndef DEFINE_RED_GAIN
    #define DEFINE_RED_GAIN 1
    #endif
    #ifndef DEFINE_GREEN_GAIN
    #define DEFINE_GREEN_GAIN 1
    #endif
    #ifndef DEFINE_BLUE_GAIN
    #define DEFINE_BLUE_GAIN 1
    #endif
    #ifndef DEFINE_SERIAL_RGB_TYPE
    #define DEFINE_SERIAL_RGB_TYPE uint16_t
    #endif
    
    constexpr int MULTIPLEX = DEFINE_MULTIPLEX;
    constexpr int MAX_RGB_LED_STEPS = DEFINE_MAX_RGB_LED_STEPS;       // Contrast Ratio - Min RGB constant forward current (Blue LED in my case) in uA divided by min light current in uA
    constexpr int MAX_REFRESH = DEFINE_MAX_REFRESH;
    constexpr int FPS = DEFINE_FPS;
    constexpr int COLUMNS = DEFINE_COLUMNS;
    constexpr float SERIAL_CLOCK = (DEFINE_SERIAL_CLOCK * 1000000.0);
    constexpr int BLANK_TIME = DEFINE_BLANK_TIME;
    constexpr float RED_GAIN = DEFINE_RED_GAIN;
    constexpr float GREEN_GAIN = DEFINE_GREEN_GAIN;
    constexpr float BLUE_GAIN = DEFINE_BLUE_GAIN;
    constexpr float GCLK = (DEFINE_GCLK * 1000000.0);
    
    #include <math.h>
    constexpr int PWM_bits = round(log2((double) MAX_RGB_LED_STEPS / MULTIPLEX));
    
    typedef DEFINE_SERIAL_RGB_TYPE test[2 * MULTIPLEX][COLUMNS][3];
    
    typedef union {
        test data;
        uint8_t mem[((sizeof(test) / 64) + 1) * 64];
    } packet;

#endif

