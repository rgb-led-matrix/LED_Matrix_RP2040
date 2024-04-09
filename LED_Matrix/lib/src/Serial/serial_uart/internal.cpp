/* 
 * File:   internal.cpp
 * Author: David Thacher
 * License: GPL 3.0
 */

#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "hardware/uart.h"
#include "Serial/serial_uart/internal.h"
#include "Serial/serial_uart/machine.h"
#include "Serial/serial_uart/serial_uart.h"
#include "Matrix/matrix.h"

namespace Serial::UART::internal {
    void __not_in_flash_func(process)(uint16_t *p, uint16_t len) {
        switch (sizeof(DEFINE_SERIAL_RGB_TYPE)) {
            case 2:
            case 6:
                for (uint16_t i = 0; i < len; i += 2)
                    p[i / 2] = ntohs(p[i / 2]);
                break;
            default:
                break;
        }

        Matrix::Worker::process((void *) p);
    }

    void __not_in_flash_func(send_status)(STATUS status) {
        static Status_Message messages;

        messages.set_status(status);
        send_message(&messages);
    }

    static inline void __not_in_flash_func(write_chunk)(uint32_t v, uint8_t bits) {
        for (int i = 0; i < bits; i += 8) {
            if (uart_is_writable(uart0))
                uart_putc(uart0, (v >> i) & 0xFF);
            else
                watchdog_update();
        }
    }

    // Blocking buts keeps watchdog going
    void __not_in_flash_func(send_message)(Status_Message *buf) {
        write_chunk(buf->header, 32);
        write_chunk(buf->cmd, 8);
        write_chunk(buf->len, 16);
        write_chunk(buf->status, 32);
        write_chunk(buf->checksum, 32);
        write_chunk(buf->delimiter, 32);
    }

    Status_Message::Status_Message() {
            header = htonl(0xAAEEAAEE);
            cmd = 's';
            len = htons(4);
            delimiter = htonl(0xAEAEAEAE);
        }

        void Status_Message::set_status(STATUS s) {
            switch (s) {
                case STATUS::IDLE_0:
                    status = htonl(0);
                    // TODO: Checksum
                    checksum = htonl(0);
                    break;

                case STATUS::IDLE_1:
                    status = htonl(1);
                    // TODO: Checksum
                    checksum = htonl(0);
                    break;

                case STATUS::ACTIVE_0:
                    status = htonl(2);
                    // TODO: Checksum
                    checksum = htonl(0);
                    break;

                case STATUS::ACTIVE_1:
                    status = htonl(3);
                    // TODO: Checksum
                    checksum = htonl(0);
                    break;

                case STATUS::READY:
                    status = htonl(4);
                    // TODO: Checksum
                    checksum = htonl(0);
                    break;

                default:
                    status = htonl(0xFFFFFFFF);
                    // TODO: Checksum
                    checksum = htonl(0);
                    break;
            }
        }
}