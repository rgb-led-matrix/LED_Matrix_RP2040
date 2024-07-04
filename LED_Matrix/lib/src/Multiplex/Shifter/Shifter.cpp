/* 
 * File:   Shifter.cpp
 * Author: David Thacher
 * License: GPL 3.0
 */

#include <math.h>
#include "Multiplex/Multiplex.h"
#include "Multiplex/config.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

namespace Multiplex {
    static int aux[3];
    static constexpr int ADDR_A = 16;
    static constexpr int ADDR_B = 17;
    static constexpr int ADDR_C = 18;
    static constexpr int ADDR_D = 19;
    static constexpr int ADDR_E = 20;

    void init(int rows) {
        int clk, data, lat;
        uint8_t type = 0;

        for (int i = 0; i < 5; i++) {
            gpio_init(i + 16);
            gpio_set_dir(i + 16, GPIO_OUT);
        }
        gpio_clr_mask(0x1F0000);

        switch (multiplex_type) {
            case 1:
                clk = ADDR_A;
                data = ADDR_B;
                lat = ADDR_C;
                break;
            case 2:
                clk = ADDR_A;
                data = ADDR_B;
                type = 1;
                break;
            case 3:
                clk = ADDR_A;
                data = ADDR_B;
                lat = ADDR_C;
                aux[0] = ADDR_D;
                aux[1] = ADDR_E;
                break;
            case 0:
                // Do not use!
            default:
                break;
        }

        switch (type) {
            case 0:
                {
                    // TODO:
                    //  Update PIO code
                    //      Make blocking logic to allow syncing

                    gpio_set_function(clk, GPIO_FUNC_PIO1);
                    gpio_set_function(data, GPIO_FUNC_PIO1);
                    gpio_set_function(lat, GPIO_FUNC_PIO1);

                    // PIO
                    const uint16_t instructions[] = {
                        (uint16_t) (pio_encode_pull(false, true) | pio_encode_sideset(2, 0)),   // PIO SM
                        (uint16_t) (pio_encode_out(pio_x, 8) | pio_encode_sideset(2, 0)),
                        (uint16_t) (pio_encode_out(pio_y, 8) | pio_encode_sideset(2, 0)),
                        (uint16_t) (pio_encode_out(pio_pins, 6) | pio_encode_sideset(2, 0)),    // PMP Program
                        (uint16_t) (pio_encode_jmp_y_dec(3) | pio_encode_sideset(2, 1)),
                        (uint16_t) (pio_encode_nop() | pio_encode_sideset(2, 2)),
                        (uint16_t) (pio_encode_nop() | pio_encode_sideset(2, 2)),
                        (uint16_t) (pio_encode_nop() | pio_encode_sideset(2, 0)),
                        (uint16_t) (pio_encode_jmp_x_dec(2) | pio_encode_sideset(2, 0)),
                        (uint16_t) (pio_encode_jmp(0) | pio_encode_sideset(2, 0))
                    };
                    static const struct pio_program pio_programs = {
                        .instructions = instructions,
                        .length = count_of(instructions),
                        .origin = 0,
                    };
                    pio_add_program(pio1, &pio_programs);
                    pio_sm_set_consecutive_pindirs(pio1, 0, 16, 5, true);
                
                    // Verify Serial Clock
                    constexpr float x = 125000000.0 / (multiplex_clock * 2.0);
                    static_assert(x >= 1.0, "Unabled to configure PIO for MULTIPLEX_CLOCK");

                    // PMP / SM
                    pio1->sm[0].clkdiv = ((uint32_t) floor(x) << PIO_SM0_CLKDIV_INT_LSB) | ((uint32_t) round((x - floor(x)) * 255.0) << PIO_SM0_CLKDIV_FRAC_LSB);
                    pio1->sm[0].pinctrl = (5 << PIO_SM0_PINCTRL_SIDESET_COUNT_LSB) | (16 << PIO_SM0_PINCTRL_SIDESET_BASE_LSB);
                    pio1->sm[0].shiftctrl = (1 << PIO_SM0_SHIFTCTRL_AUTOPULL_LSB) | (8 << PIO_SM0_SHIFTCTRL_PULL_THRESH_LSB);
                    pio1->sm[0].execctrl = (12 << PIO_SM0_EXECCTRL_WRAP_TOP_LSB);
                    pio1->sm[0].instr = pio_encode_jmp(0);
                    hw_set_bits(&pio1->ctrl, 1 << PIO_CTRL_SM_ENABLE_LSB);
                    pio_sm_claim(pio1, 0);
                }
                break;
            case 1:
                {
                    // TODO:
                    //  Update PIO code
                    //      Make blocking logic to allow syncing

                    gpio_set_function(clk, GPIO_FUNC_PIO1);
                    gpio_set_function(data, GPIO_FUNC_PIO1);
                    gpio_set_function(lat, GPIO_FUNC_PIO1);

                    // PIO
                    const uint16_t instructions[] = {
                        (uint16_t) (pio_encode_pull(false, true) | pio_encode_sideset(2, 0)),   // PIO SM
                        (uint16_t) (pio_encode_out(pio_x, 8) | pio_encode_sideset(2, 0)),
                        (uint16_t) (pio_encode_out(pio_y, 8) | pio_encode_sideset(2, 0)),
                        (uint16_t) (pio_encode_out(pio_pins, 6) | pio_encode_sideset(2, 0)),    // PMP Program
                        (uint16_t) (pio_encode_jmp_y_dec(3) | pio_encode_sideset(2, 1)),
                        (uint16_t) (pio_encode_nop() | pio_encode_sideset(2, 2)),
                        (uint16_t) (pio_encode_nop() | pio_encode_sideset(2, 2)),
                        (uint16_t) (pio_encode_nop() | pio_encode_sideset(2, 0)),
                        (uint16_t) (pio_encode_jmp_x_dec(2) | pio_encode_sideset(2, 0)),
                        (uint16_t) (pio_encode_jmp(0) | pio_encode_sideset(2, 0))
                    };
                    static const struct pio_program pio_programs = {
                        .instructions = instructions,
                        .length = count_of(instructions),
                        .origin = 0,
                    };
                    pio_add_program(pio1, &pio_programs);
                    pio_sm_set_consecutive_pindirs(pio1, 0, 16, 5, true);
                
                    // Verify Serial Clock
                    constexpr float x = 125000000.0 / (multiplex_clock * 2.0);
                    static_assert(x >= 1.0, "Unabled to configure PIO for MULTIPLEX_CLOCK");

                    // PMP / SM
                    pio1->sm[0].clkdiv = ((uint32_t) floor(x) << PIO_SM0_CLKDIV_INT_LSB) | ((uint32_t) round((x - floor(x)) * 255.0) << PIO_SM0_CLKDIV_FRAC_LSB);
                    pio1->sm[0].pinctrl = (5 << PIO_SM0_PINCTRL_SIDESET_COUNT_LSB) | (16 << PIO_SM0_PINCTRL_SIDESET_BASE_LSB);
                    pio1->sm[0].shiftctrl = (1 << PIO_SM0_SHIFTCTRL_AUTOPULL_LSB) | (8 << PIO_SM0_SHIFTCTRL_PULL_THRESH_LSB);
                    pio1->sm[0].execctrl = (12 << PIO_SM0_EXECCTRL_WRAP_TOP_LSB);
                    pio1->sm[0].instr = pio_encode_jmp(0);
                    hw_set_bits(&pio1->ctrl, 1 << PIO_CTRL_SM_ENABLE_LSB);
                    pio_sm_claim(pio1, 0);
                }
                break;
            default:
                break;
        }
    }
    
    void __not_in_flash_func(SetRow)(int row) {
        // TODO:

        switch (multiplex_type) {

            case 0:
                // Do not use!
            default:
                break;
        }
    }
}
