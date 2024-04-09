/* 
 * File:   data_node.cpp
 * Author: David Thacher
 * License: GPL 3.0
 */

#include "hardware/uart.h"
#include "Serial/serial_uart/serial_uart.h"
#include "Serial/serial_uart/internal.h"
#include "Serial/serial_uart/control_node.h"
#include "Serial/serial_uart/data_node.h"

namespace Serial::UART::DATA_NODE {
    static uint8_t *buf = 0;
    static uint16_t len = 0;

    static DATA_STATES state_data = DATA_STATES::SETUP;
    static uint8_t idle_num = 0;
    static COMMAND command;
    static uint32_t index;
    static random_type data;
    static uint32_t checksum;
    static STATUS status;

    static void get_data(uint8_t *buf, uint16_t len, bool checksum);
    static void process_command();
    static void process_payload();
    static void process_frame();
    
    STATUS __not_in_flash_func(data_node)() {
        // Currently we drop the frame and wait for the next valid header.
        //  Host app will do the right thing using status messages.
        switch (state_data) {
            case DATA_STATES::SETUP:
                index = 0;
                Serial::UART::uart_callback(&buf, &len);
                state_data = DATA_STATES::PREAMBLE_CMD_LEN;

                if (idle_num == 0)
                    status = STATUS::IDLE_0;
                else
                    status = STATUS::IDLE_1;
                break;

            // Host protocol should create bubble waiting for status after sending data.
            //  Half duplex like currently for simplicity. We should have the bandwidth.
            //  Host needs to be on the ball though. Performance loss is possible from OS!
            case DATA_STATES::PREAMBLE_CMD_LEN:                 // Host should see IDLE_0/1 to ACTIVE_0
                get_data(data.bytes, 8, false);
                process_command();                
                break;

            // Host protocol should create bubble waiting for status after sending data.
            //  Half duplex like currently for simplicity. We should have the bandwidth.
            //  Host needs to be on the ball though. Performance loss is possible from OS!
            case DATA_STATES::PAYLOAD:                          // Host should see ACTIVE_0 to ACTIVE_1
                process_payload();
                break;

            // Host protocol should create bubble waiting for status after sending data.
            //  Half duplex like currently for simplicity. We should have the bandwidth.
            //  Host needs to be on the ball though. Performance loss is possible from OS!
            case DATA_STATES::CHECKSUM_DELIMITER_PROCESS:       // Host should see ACTIVE_1 to IDLE_1/0
                get_data(data.bytes, 8, false);
                process_frame();
                break;

            default:
                state_data = DATA_STATES::SETUP;
                break;
        }

        return status;
    }

    void __not_in_flash_func(get_data)(uint8_t *buf, uint16_t len, bool checksum) {
        if (checksum) {
            // TODO: Compute checksum in parallel (via DMA?)
        }
        else {
            while (index < len) {
                if (uart_is_readable(uart0)) {
                    buf[index] = uart_getc(uart0);
                    index++;
                }
                else
                    break;
            }
        }
    }

    inline void __not_in_flash_func(process_command)() {
        bool escape = true;

        if (index == 8) {
            if (ntohl(data.longs[0]) == 0xAAEEAAEE) {
                switch (data.bytes[5]) {
                    case 'd':
                        switch (data.bytes[4]) {
                            case 'd':
                                if (ntohs(data.shorts[3]) == len) {
                                    state_data = DATA_STATES::PAYLOAD;
                                    command = COMMAND::DATA;
                                    status = STATUS::ACTIVE_0;
                                    index = 0;
                                    checksum = 0;
                                    escape = false;
                                }
                                break;

                            case 'r':
                                if (ntohs(data.shorts[3]) == len) {
                                    state_data = DATA_STATES::PAYLOAD;
                                    command = COMMAND::RAW_DATA;
                                    status = STATUS::ACTIVE_0;
                                    index = 0;
                                    checksum = 0;
                                    escape = false;
                                }
                                break;

                            default:
                                break;
                        }
                        break;

                    case 'c':
                        switch (data.bytes[4]) {
                            case 'i':
                                if (ntohs(data.shorts[3]) == 1) {
                                    state_data = DATA_STATES::PAYLOAD;
                                    command = COMMAND::SET_ID;
                                    status = STATUS::ACTIVE_0;
                                    index = 0;
                                    checksum = 0;
                                    escape = false;
                                }
                                break;

                            default:
                                break;
                        }
                        break;

                    default:
                        break;
                }
            }
        }

        // Reseed and try again.
        //  Host app will do the right thing. (Did not see IDLE_0/1 to ACTIVE_0)
        if (escape) {
            data.bytes[0] = data.bytes[1];
            data.bytes[1] = data.bytes[2];
            data.bytes[2] = data.bytes[3];
            data.bytes[3] = data.bytes[4];
            data.bytes[4] = data.bytes[5];
            data.bytes[5] = data.bytes[6];
            data.bytes[6] = data.bytes[7];
            index--;
        }
    }

    inline void __not_in_flash_func(process_payload)() {
        switch (command) {
            case COMMAND::DATA:
                get_data(buf, len, true);

                if (len == index) {
                    state_data = DATA_STATES::CHECKSUM_DELIMITER_PROCESS;
                    index = 0;
                    status = STATUS::ACTIVE_1;
                }
                break;

            case COMMAND::RAW_DATA:
                get_data(buf, len, false);

                if (len == index) {
                    state_data = DATA_STATES::CHECKSUM_DELIMITER_PROCESS;
                    index = 0;
                    status = STATUS::ACTIVE_1;
                }
                break;

            case COMMAND::SET_ID:
                get_data(data.bytes, 1, true);

                if (index == 1) {
                    state_data = DATA_STATES::CHECKSUM_DELIMITER_PROCESS;
                    index = 0;
                    status = STATUS::ACTIVE_1;
                }
                break;

            default:
                state_data = DATA_STATES::SETUP;
                break;
        }
    }

    inline void __not_in_flash_func(process_frame)() {
        bool error = true;

        if (index == 8) {
            index = 0;
                    
            if (ntohl(data.longs[1]) == 0xAEAEAEAE) {
                switch (command) {
                    case COMMAND::DATA:
                        if (ntohl(data.longs[0]) == checksum) {
                            Serial::UART::internal::process((uint16_t *) buf, len);
                            error = false;
                        }
                        break;

                    case COMMAND::SET_ID:
                        if (ntohl(data.longs[0]) == checksum) {
                            Serial::UART::CONTROL_NODE::set_id(data.bytes[0]);
                            error = false;
                        }
                        break;
                    
                    case COMMAND::RAW_DATA:
                        Serial::UART::internal::process((uint16_t *) buf, len);
                        error = false;
                        break;

                    default:
                        break;
                }
            }
            
            if (!error) 
                idle_num = (idle_num + 1) % 2;

            state_data = DATA_STATES::SETUP;
        }
    }
}