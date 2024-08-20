/* 
 * File:   serial.h
 * Author: David Thacher
 * License: GPL 3.0
 */
 
#ifndef SERIAL_NODE_SERIAL_H
#define SERIAL_NODE_SERIAL_H

#include "Serial/serial.h"
#include "Serial/config.h"

namespace Serial {
    void callback(Serial::packet **buf);
    uint16_t get_len();
}

#endif

