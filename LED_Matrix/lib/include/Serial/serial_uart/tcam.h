/* 
 * File:   tcam.h
 * Author: David Thacher
 * License: GPL 3.0
 */

#include <stdint.h>

namespace Serial::TCAM {
    constexpr uint8_t num_rules = 4;
    constexpr uint8_t num_bytes = 12;

    union TCAM_entry {
        uint8_t bytes[num_bytes];
        uint16_t shorts[num_bytes / 2];
        uint32_t data[num_bytes / 4];
    };

    // Only the highest priority rule match runs

    bool TCAM_rule(uint8_t priority, TCAM_entry key, TCAM_entry enable, void (*func)());
    void TCAM_process(const TCAM_entry *data);
}