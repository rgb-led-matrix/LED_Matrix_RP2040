/* 
 * File:   control_node.cpp
 * Author: David Thacher
 * License: GPL 3.0
 */

#include "hardware/uart.h"
#include "pico/multicore.h"
#include "Serial/serial_uart/control_node.h"

namespace Serial::UART::CONTROL_NODE {
    static uint8_t id = 0;
    
    void __not_in_flash_func(control_node)() {
        static CONTROL_STATES state_control = CONTROL_STATES::SETUP;

        // TODO:
        // Reply via uart0 using correct control header (if required)
        switch (state_control) {
            case CONTROL_STATES::SETUP:
                // TODO:
                state_control = CONTROL_STATES::LISTEN;
                break;
            case CONTROL_STATES::LISTEN:
                // TODO:
                //state_control = CONTROL_STATES::TRIGGER;
                break;
            case CONTROL_STATES::TRIGGER:
                // TODO:
                state_control = CONTROL_STATES::SETUP;
                break;
            default:
                state_control = CONTROL_STATES::SETUP;
                break;
        }
    }

    void __not_in_flash_func(set_id)(uint8_t num) {
        id = num;
    }
}