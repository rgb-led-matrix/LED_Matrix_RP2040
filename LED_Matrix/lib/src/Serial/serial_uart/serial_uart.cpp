/*
 * File:   serial_uart.cpp
 * Author: David Thacher
 * License: GPL 3.0
 */

#include <stdint.h>
#include <machine/endian.h>
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "Serial/serial_uart/serial_uart.h"
#include "Matrix/matrix.h"

namespace Serial {
    static uint8_t *buf = 0;
    static uint16_t len = 0;

    enum class DATA_STATES {
        SETUP,
        PREAMBLE,
        CMD_LEN,
        PAYLOAD,
        CHECKSUM,
        DELIMITER,
        PROCESS
    };

    enum class CONTROL_STATES {
        SETUP,
        LISTEN,
        TRIGGER
    };

    union random_type {
        uint8_t bytes[4];
        uint16_t shorts[2];
        uint32_t val;
    };

    static DATA_STATES state_data = DATA_STATES::SETUP;
    static CONTROL_STATES state_control = CONTROL_STATES::SETUP;

    static void uart_process();

    #if __BYTE_ORDER == __LITTLE_ENDIAN
        #define ntohs(x) __bswap16(x)
        #define ntohl(x) __bswap32(x)
    #else
        #define ntohs(x) ((uint16_t)(x))
        #define ntoh1(x) ((uint32_t)(x))
    #endif

    void uart_start() {
        // IO
        gpio_init(0);
        gpio_init(1);
        gpio_init(5);
        gpio_set_dir(0, GPIO_OUT);
        gpio_set_function(0, GPIO_FUNC_UART);
        gpio_set_function(1, GPIO_FUNC_UART);
        gpio_set_function(5, GPIO_FUNC_UART);

        // UART
        static_assert(SERIAL_UART_BAUD <= 7800000, "Baud rate must be less than 7.8MBaud");
        uart_init(uart0, SERIAL_UART_BAUD);
        uart_init(uart1, 115200);
    }

    // Warning host is required to obey flow control and handle bus recovery
    void __not_in_flash_func(uart_task)() {
        static uint32_t index;
        static random_type data;
        static uint8_t state;

        // Check for errors
        if (!((uart0_hw->ris & 0x380) == 0)) {
            uart0_hw->icr = 0x7FF;
        }

        // Check for errors
        if (!((uart1_hw->ris & 0x380) == 0)) {
            uart1_hw->icr = 0x7FF;
        }

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

        // Currently we drop the frame and wait for the next valid header.
        //  Because we drop this may have unstable path which never really recovers.
        // Currently we do not implement a valid control header for status messages.
        switch (state_data) {
            case DATA_STATES::SETUP:
                index = 0;
                state = 'r';
                uart_callback(&buf, &len);
                state_data = DATA_STATES::PREAMBLE;
                break;

            case DATA_STATES::PREAMBLE:
                while (index < 4) {
                    if (uart_is_readable(uart0)) {
                        data.bytes[index] = uart_getc(uart0);
                        index++;
                    }
                    else
                        break;
                }

                if (ntohl(data.val) == 0xAAEEAAEE) {
                    state_data = DATA_STATES::CMD_LEN;
                    index = 0;
                    state = 'a';
                }
                else {
                    data.bytes[0] = data.bytes[1];
                    data.bytes[1] = data.bytes[2];
                    data.bytes[2] = data.bytes[3];
                    index--;
                }
                break;

            case DATA_STATES::CMD_LEN:
                while (index < 4) {
                    if (uart_is_readable(uart0)) {
                        data.bytes[index] = uart_getc(uart0);
                        index++;
                    }
                    else
                        break;
                }

                if (index == 4) {
                    index = 0;
                
                    // Currently we only support the one command
                    if (data.bytes[1] == 'd' && data.bytes[0] == 'd' && ntohs(data.shorts[1]) == len) {
                        state_data = DATA_STATES::PAYLOAD;
                    }
                    else {
                        state_data = DATA_STATES::PREAMBLE;
                        state = 'r';
                    }
                }
                break;

            case DATA_STATES::PAYLOAD:
                while (index < len) {
                    if (uart_is_readable(uart0)) {
                        buf[index] = uart_getc(uart0);
                        index++;
                    }
                    else
                        break;
                }

                if (len == index) {
                    state_data = DATA_STATES::CHECKSUM;
                    index = 0;
                }
                break;

            case DATA_STATES::CHECKSUM:
                while (index < 4) {
                    if (uart_is_readable(uart0)) {
                        data.bytes[index] = uart_getc(uart0);
                        index++;
                    }
                    else
                        break;
                }

                // TODO:
                if (index == 4) {
                    index = 0;
                    
                    if (ntohl(data.val) == 0xAAEEAAEE) {
                        state_data = DATA_STATES::DELIMITER;
                    }
                    else {
                        state_data = DATA_STATES::PREAMBLE;
                        state = 'r';
                    }
                }
                break;

            case DATA_STATES::DELIMITER:
                while (index < 4) {
                    if (uart_is_readable(uart0)) {
                        data.bytes[index] = uart_getc(uart0);
                        index++;
                    }
                    else
                        break;
                }

                if (index == 4) {
                    index = 0;

                    if (ntohl(data.val) == 0xAEAEAEAE) {
                        state_data = DATA_STATES::PROCESS;
                    }
                    else {
                        state_data = DATA_STATES::PREAMBLE;
                        state = 'r';
                    }
                }
                break;

            case DATA_STATES::PROCESS:
                uart_process();
                state_data = DATA_STATES::SETUP;
                break;

            default:
                state_data = DATA_STATES::SETUP;
                break;
        }

        if (uart_is_writable(uart0)) {
            uart_putc(uart0, state);
        }
    }

    void __not_in_flash_func(uart_process)() {
        uint16_t *p = (uint16_t *) buf;

        switch (sizeof(DEFINE_SERIAL_RGB_TYPE)) {
            case 2:
            case 6:
                for (uint16_t i = 0; i < len; i += 2)
                    p[i / 2] = ntohs(p[i / 2]);
                break;
            default:
                break;
        }

        Matrix::Worker::process((void *) buf);
    }
}
