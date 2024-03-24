/* 
 * File:   matrix.h
 * Author: David Thacher
 * License: GPL 3.0
 */
 
#ifndef MATRIX_H
#define MATRIX_H

#include "Serial/config.h"

namespace Matrix {
    /**
     *  @brief Starts Matrix state machine.
     *  @details Implemented in Matrix/<implementation>/matrix.cpp
     */
    void start();

    /**
     *  @brief Matrix state machine.
     *  @details Implemented in Matrix/<implementation>/matrix.cpp
     */
    void loop();
    
    // This can be rendered into coprocessor. (Currently this is done via super loop on core 1.)
    namespace Worker {        
        /**
         *  @brief Function used to create buffers for testing.
         *  @details Implemented in Matrix/<implementation>/worker.cpp
         */
        void process(Serial::packet *arg);
    }

    // This is terrible practice!!!
    namespace Loafer {        
        /**
         *  @brief Function used to chuck data to loafer (Assumes flow control)
         *  @details Warning must toss buffer before asking for another.
         *  @details Uses zero copy, but must call to sync state.
         *  @details Implemented in Matrix/<implementation>/loafer.cpp
         */
        void toss();

        /**
         *  @brief Function used to grab a front buffer
         *  @details Warning must only have one front buffer at a time.
         *  @details Implemented in Matrix/<implementation>/loafer.cpp
         *  @return Raw memory pointer to front buffer, no type is known by API. (Not very classy)
         */
        void *get_front_buffer();

        /**
         *  @brief Function used to grab a back buffer
         *  @details Warning must only have one back buffer at a time.
         *  @details Implemented in Matrix/<implementation>/loafer.cpp
         *  @return Raw memory pointer to back buffer, no type is known by API. (Not very classy)
         */
        void *get_back_buffer();

        /**
         *  @brief Function to get size of back buffer
         *  @details Implemended in Matrix/<implementation/loafer.cpp
         *  @details Must always return a number which is a multiple of two!
         *  @return Buffer size
        */
        uint32_t get_buffer_size();
    }

    namespace Calculator {
        /**
         *  @brief Function used to verify configuration
         *  @details Implemented in Matrix/<implementation>/matrix.cpp
         */
        void verify_configuration();
    }
}

#endif

