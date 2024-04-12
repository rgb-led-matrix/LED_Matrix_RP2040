/* 
 * File:   internal.h
 * Author: David Thacher
 * License: GPL 3.0
 */
 
#ifndef SERIAL_UART_INTERNAL_H
#define SERIAL_UART_INTERNAL_H
    
#include <stdint.h>

namespace Serial::UART::internal {
    enum class STATUS {
        IDLE_0,
        IDLE_1,
        ACTIVE_0,
        ACTIVE_1,
        READY
    };

    struct Status_Message {
        Status_Message();

        void set_status(STATUS s);

        uint32_t header;
        uint8_t cmd;
        uint16_t len;
        uint32_t status;
        uint32_t checksum;
        uint32_t delimiter;
    };

    void process(uint16_t *buf, uint16_t len);
    void send_status(STATUS status);
    void send_message(Status_Message *message);
    uint32_t crc(uint32_t crc, uint8_t data);
}

#endif
