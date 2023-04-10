/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

// For memcpy
#include <string.h>

// Include descriptor struct definitions
#include "Serial/serial_usb/usb_common.h"
// USB register definitions from pico-sdk
#include "hardware/regs/usb.h"
// USB hardware struct definitions from pico-sdk
#include "hardware/structs/usb.h"
// For interrupt enable and numbers
#include "hardware/irq.h"
// For resetting the USB controller
#include "hardware/resets.h"
#include "hardware/dma.h"

// Device descriptors
#include "Serial/serial_usb/dev_lowlevel.h"
#include "Serial/serial_usb/serial_usb.h"

#define usb_hw_set hw_set_alias(usb_hw)
#define usb_hw_clear hw_clear_alias(usb_hw)

// Function prototypes for our device specific endpoint handlers defined
// later on
void ep0_in_handler(uint8_t *buf, uint16_t len);
void ep0_out_handler(uint8_t *buf, uint16_t len);
void ep1_out_handler(uint8_t *buf, uint16_t len);
void ep2_out_handler(uint8_t *buf, uint16_t len);
void ep3_out_handler(uint8_t *buf, uint16_t len);
void ep4_out_handler(uint8_t *buf, uint16_t len);
void ep5_out_handler(uint8_t *buf, uint16_t len);
void ep6_out_handler(uint8_t *buf, uint16_t len);
void ep7_out_handler(uint8_t *buf, uint16_t len);
void ep8_out_handler(uint8_t *buf, uint16_t len);
void ep9_out_handler(uint8_t *buf, uint16_t len);
void ep10_out_handler(uint8_t *buf, uint16_t len);
void ep11_out_handler(uint8_t *buf, uint16_t len);
void ep12_out_handler(uint8_t *buf, uint16_t len);
void ep13_out_handler(uint8_t *buf, uint16_t len);
void ep14_out_handler(uint8_t *buf, uint16_t len);
void ep15_out_handler(uint8_t *buf, uint16_t len);

// Global device address
static bool should_set_address = false;
static uint8_t dev_addr = 0;
static volatile bool configured = false;

static serial_usb_callback func;
static int dma_chan[2];
static struct {uint32_t len; uint8_t *src; uint8_t *dst;} address_table[2][15 + 1]; // TODO: Fix this
static uint8_t table = 0;

// Global data buffer for EP0
static uint8_t ep0_buf[64];

// Struct defining the device configuration
static struct usb_device_configuration dev_config = {
        .device_descriptor = &device_descriptor,
        .interface_descriptor = &interface_descriptor,
        .config_descriptor = &config_descriptor,
        .lang_descriptor = lang_descriptor,
        .descriptor_strings = descriptor_strings,
        .endpoints = {
                {
                        .descriptor = &ep0_out,
                        .handler = &ep0_out_handler,
                        .endpoint_control = NULL, // NA for EP0
                        .buffer_control = &usb_dpram->ep_buf_ctrl[0].out,
                        // EP0 in and out share a data buffer
                        .data_buffer = &usb_dpram->ep0_buf_a[0],
                },
                {
                        .descriptor = &ep0_in,
                        .handler = &ep0_in_handler,
                        .endpoint_control = NULL, // NA for EP0,
                        .buffer_control = &usb_dpram->ep_buf_ctrl[0].in,
                        // EP0 in and out share a data buffer
                        .data_buffer = &usb_dpram->ep0_buf_a[0],
                },
                {
                        .descriptor = &ep1_out,
                        .handler = &ep1_out_handler,
                        .endpoint_control = &usb_dpram->ep_ctrl[0].out,
                        .buffer_control = &usb_dpram->ep_buf_ctrl[1].out,
                        .data_buffer = &usb_dpram->epx_data[0 * 64],
                },
                {
                        .descriptor = &ep2_out,
                        .handler = &ep2_out_handler,
                        .endpoint_control = &usb_dpram->ep_ctrl[1].out,
                        .buffer_control = &usb_dpram->ep_buf_ctrl[2].out,
                        .data_buffer = &usb_dpram->epx_data[1 * 64],
                },
                {
                        .descriptor = &ep3_out,
                        .handler = &ep3_out_handler,
                        .endpoint_control = &usb_dpram->ep_ctrl[2].out,
                        .buffer_control = &usb_dpram->ep_buf_ctrl[3].out,
                        .data_buffer = &usb_dpram->epx_data[2 * 64],
                },
                {
                        .descriptor = &ep4_out,
                        .handler = &ep4_out_handler,
                        .endpoint_control = &usb_dpram->ep_ctrl[3].out,
                        .buffer_control = &usb_dpram->ep_buf_ctrl[4].out,
                        .data_buffer = &usb_dpram->epx_data[3 * 64],
                },
                {
                        .descriptor = &ep5_out,
                        .handler = &ep5_out_handler,
                        .endpoint_control = &usb_dpram->ep_ctrl[4].out,
                        .buffer_control = &usb_dpram->ep_buf_ctrl[5].out,
                        .data_buffer = &usb_dpram->epx_data[4 * 64],
                },
                {
                        .descriptor = &ep6_out,
                        .handler = &ep6_out_handler,
                        .endpoint_control = &usb_dpram->ep_ctrl[5].out,
                        .buffer_control = &usb_dpram->ep_buf_ctrl[6].out,
                        .data_buffer = &usb_dpram->epx_data[5 * 64],
                },
                {
                        .descriptor = &ep7_out,
                        .handler = &ep7_out_handler,
                        .endpoint_control = &usb_dpram->ep_ctrl[6].out,
                        .buffer_control = &usb_dpram->ep_buf_ctrl[7].out,
                        .data_buffer = &usb_dpram->epx_data[6 * 64],
                },
                {
                        .descriptor = &ep8_out,
                        .handler = &ep8_out_handler,
                        .endpoint_control = &usb_dpram->ep_ctrl[7].out,
                        .buffer_control = &usb_dpram->ep_buf_ctrl[8].out,
                        .data_buffer = &usb_dpram->epx_data[7 * 64],
                },
                {
                        .descriptor = &ep9_out,
                        .handler = &ep9_out_handler,
                        .endpoint_control = &usb_dpram->ep_ctrl[8].out,
                        .buffer_control = &usb_dpram->ep_buf_ctrl[9].out,
                        .data_buffer = &usb_dpram->epx_data[8 * 64],
                },
                {
                        .descriptor = &ep10_out,
                        .handler = &ep10_out_handler,
                        .endpoint_control = &usb_dpram->ep_ctrl[9].out,
                        .buffer_control = &usb_dpram->ep_buf_ctrl[10].out,
                        .data_buffer = &usb_dpram->epx_data[9 * 64],
                },
                {
                        .descriptor = &ep11_out,
                        .handler = &ep11_out_handler,
                        .endpoint_control = &usb_dpram->ep_ctrl[10].out,
                        .buffer_control = &usb_dpram->ep_buf_ctrl[11].out,
                        .data_buffer = &usb_dpram->epx_data[10 * 64],
                },
                {
                        .descriptor = &ep12_out,
                        .handler = &ep12_out_handler,
                        .endpoint_control = &usb_dpram->ep_ctrl[11].out,
                        .buffer_control = &usb_dpram->ep_buf_ctrl[12].out,
                        .data_buffer = &usb_dpram->epx_data[11 * 64],
                },
                {
                        .descriptor = &ep13_out,
                        .handler = &ep13_out_handler,
                        .endpoint_control = &usb_dpram->ep_ctrl[12].out,
                        .buffer_control = &usb_dpram->ep_buf_ctrl[13].out,
                        .data_buffer = &usb_dpram->epx_data[12 * 64],
                },
                {
                        .descriptor = &ep14_out,
                        .handler = &ep14_out_handler,
                        .endpoint_control = &usb_dpram->ep_ctrl[13].out,
                        .buffer_control = &usb_dpram->ep_buf_ctrl[14].out,
                        .data_buffer = &usb_dpram->epx_data[13 * 64],
                },
                {
                        .descriptor = &ep15_out,
                        .handler = &ep15_out_handler,
                        .endpoint_control = &usb_dpram->ep_ctrl[14].out,
                        .buffer_control = &usb_dpram->ep_buf_ctrl[15].out,
                        .data_buffer = &usb_dpram->epx_data[14 * 64],
                }
        }
};

/**
 * @brief Given an endpoint address, return the usb_endpoint_configuration of that endpoint. Returns NULL
 * if an endpoint of that address is not found.
 *
 * @param addr
 * @return struct usb_endpoint_configuration*
 */
struct usb_endpoint_configuration *usb_get_endpoint_configuration(uint8_t addr) {
    struct usb_endpoint_configuration *endpoints = dev_config.endpoints;
    for (int i = 0; i < USB_NUM_ENDPOINTS_NEW; i++) {
        if (endpoints[i].descriptor && (endpoints[i].descriptor->bEndpointAddress == addr)) {
            return &endpoints[i];
        }
    }
    return NULL;
}

/**
 * @brief Given a C string, fill the EP0 data buf with a USB string descriptor for that string.
 *
 * @param C string you would like to send to the USB host
 * @return the length of the string descriptor in EP0 buf
 */
uint8_t usb_prepare_string_descriptor(const unsigned char *str) {
    // 2 for bLength + bDescriptorType + strlen * 2 because string is unicode. i.e. other byte will be 0
    uint8_t bLength = 2 + (strlen((const char *)str) * 2);
    static const uint8_t bDescriptorType = 0x03;

    volatile uint8_t *buf = &ep0_buf[0];
    *buf++ = bLength;
    *buf++ = bDescriptorType;

    uint8_t c;

    do {
        c = *str++;
        *buf++ = c;
        *buf++ = 0;
    } while (c != '\0');

    return bLength;
}

/**
 * @brief Take a buffer pointer located in the USB RAM and return as an offset of the RAM.
 *
 * @param buf
 * @return uint32_t
 */
static inline uint32_t usb_buffer_offset(volatile uint8_t *buf) {
    return (uint32_t) buf ^ (uint32_t) usb_dpram;
}

/**
 * @brief Set up the endpoint control register for an endpoint (if applicable. Not valid for EP0).
 *
 * @param ep
 */
void usb_setup_endpoint(const struct usb_endpoint_configuration *ep) {
    // EP0 doesn't have one so return if that is the case
    if (!ep->endpoint_control) {
        return;
    }

    // Get the data buffer as an offset of the USB controller's DPRAM
    uint32_t dpram_offset = usb_buffer_offset(ep->data_buffer);
    uint32_t reg = EP_CTRL_ENABLE_BITS
                   | EP_CTRL_INTERRUPT_PER_BUFFER
                   | (ep->descriptor->bmAttributes << EP_CTRL_BUFFER_TYPE_LSB)
                   | dpram_offset;
    *ep->endpoint_control = reg;
}

/**
 * @brief Set up the endpoint control register for each endpoint.
 *
 */
void usb_setup_endpoints() {
    const struct usb_endpoint_configuration *endpoints = dev_config.endpoints;
    for (int i = 0; i < USB_NUM_ENDPOINTS_NEW; i++) {
        if (endpoints[i].descriptor && endpoints[i].handler) {
            usb_setup_endpoint(&endpoints[i]);
        }
    }
}

/**
 * @brief Set up the USB controller in device mode, clearing any previous state.
 *
 */
void usb_device_init() {
    // Reset usb controller
    reset_block(RESETS_RESET_USBCTRL_BITS);
    unreset_block_wait(RESETS_RESET_USBCTRL_BITS);

    // Clear any previous state in dpram just in case
    memset(usb_dpram, 0, sizeof(*usb_dpram)); // <1>

    // Enable USB interrupt at processor
    irq_set_enabled(USBCTRL_IRQ, true);

    // Mux the controller to the onboard usb phy
    usb_hw->muxing = USB_USB_MUXING_TO_PHY_BITS | USB_USB_MUXING_SOFTCON_BITS;

    // Force VBUS detect so the device thinks it is plugged into a host
    usb_hw->pwr = USB_USB_PWR_VBUS_DETECT_BITS | USB_USB_PWR_VBUS_DETECT_OVERRIDE_EN_BITS;

    // Enable the USB controller in device mode.
    usb_hw->main_ctrl = USB_MAIN_CTRL_CONTROLLER_EN_BITS;

    // Enable an interrupt per EP0 transaction
    usb_hw->sie_ctrl = USB_SIE_CTRL_EP0_INT_1BUF_BITS; // <2>

    // Enable interrupts for when a buffer is done, when the bus is reset,
    // and when a setup packet is received
    usb_hw->inte = USB_INTS_BUFF_STATUS_BITS |
                   USB_INTS_BUS_RESET_BITS |
                   USB_INTS_SETUP_REQ_BITS;

    // Set up endpoints (endpoint control registers)
    // described by device configuration
    usb_setup_endpoints();

    // Present full speed device by enabling pull up on DP
    usb_hw_set->sie_ctrl = USB_SIE_CTRL_PULLUP_EN_BITS;
}

/**
 * @brief Given an endpoint configuration, returns true if the endpoint
 * is transmitting data to the host (i.e. is an IN endpoint)
 *
 * @param ep, the endpoint configuration
 * @return true
 * @return false
 */
static inline bool ep_is_tx(struct usb_endpoint_configuration *ep) {
    return ep->descriptor->bEndpointAddress & USB_DIR_IN;
}

/**
 * @brief Starts a transfer on a given endpoint.
 *
 * @param ep, the endpoint configuration.
 * @param buf, the data buffer to send. Only applicable if the endpoint is TX
 * @param len, the length of the data in buf (this example limits max len to one packet - 64 bytes)
 */
void usb_start_transfer(struct usb_endpoint_configuration *ep, uint8_t *buf, uint16_t len) {
    // We are asserting that the length is <= 64 bytes for simplicity of the example.
    // For multi packet transfers see the tinyusb port.
    assert(len <= 64);

    // Prepare buffer control register value
    uint32_t val = len | USB_BUF_CTRL_AVAIL;

    if (ep_is_tx(ep)) {
        // Need to copy the data from the user buffer to the usb memory
        memcpy((void *) ep->data_buffer, (void *) buf, len);
        // Mark as full
        val |= USB_BUF_CTRL_FULL;
    }

    // Set pid and flip for next transfer
    val |= ep->next_pid ? USB_BUF_CTRL_DATA1_PID : USB_BUF_CTRL_DATA0_PID;
    ep->next_pid ^= 1u;

    *ep->buffer_control = val;
}

/**
 * @brief Send device descriptor to host
 *
 */
void usb_handle_device_descriptor(volatile struct usb_setup_packet *pkt) {
    const struct usb_device_descriptor *d = dev_config.device_descriptor;
    // EP0 in
    struct usb_endpoint_configuration *ep = usb_get_endpoint_configuration(EP0_IN_ADDR);
    // Always respond with pid 1
    ep->next_pid = 1;
    usb_start_transfer(ep, (uint8_t *) d, MIN(sizeof(struct usb_device_descriptor), pkt->wLength));
}

/**
 * @brief Send the configuration descriptor (and potentially the configuration and endpoint descriptors) to the host.
 *
 * @param pkt, the setup packet received from the host.
 */
void usb_handle_config_descriptor(volatile struct usb_setup_packet *pkt) {
    uint8_t *buf = &ep0_buf[0];

    // First request will want just the config descriptor
    const struct usb_configuration_descriptor *d = dev_config.config_descriptor;
    memcpy((void *) buf, d, sizeof(struct usb_configuration_descriptor));
    buf += sizeof(struct usb_configuration_descriptor);

    // If we more than just the config descriptor copy it all
    if (pkt->wLength >= d->wTotalLength) {
        memcpy((void *) buf, dev_config.interface_descriptor, sizeof(struct usb_interface_descriptor));
        buf += sizeof(struct usb_interface_descriptor);
        const struct usb_endpoint_configuration *ep = dev_config.endpoints;

        // Copy all the endpoint descriptors starting from EP1
        for (uint i = 2; i < USB_NUM_ENDPOINTS_NEW; i++) {
            if (ep[i].descriptor) {
                memcpy((void *) buf, ep[i].descriptor, sizeof(struct usb_endpoint_descriptor));
                buf += sizeof(struct usb_endpoint_descriptor);
            }
        }

    }

    // Send data
    // Get len by working out end of buffer subtract start of buffer
    uint32_t len = (uint32_t) buf - (uint32_t) &ep0_buf[0];
    usb_start_transfer(usb_get_endpoint_configuration(EP0_IN_ADDR), &ep0_buf[0], MIN(len, pkt->wLength));
}

/**
 * @brief Handle a BUS RESET from the host by setting the device address back to 0.
 *
 */
void usb_bus_reset(void) {
    // Set address back to 0
    dev_addr = 0;
    should_set_address = false;
    usb_hw->dev_addr_ctrl = 0;
    configured = false;
}

/**
 * @brief Send the requested string descriptor to the host.
 *
 * @param pkt, the setup packet from the host.
 */
void usb_handle_string_descriptor(volatile struct usb_setup_packet *pkt) {
    uint8_t i = pkt->wValue & 0xff;
    uint8_t len = 0;

    if (i == 0) {
        len = 4;
        memcpy(&ep0_buf[0], dev_config.lang_descriptor, len);
    } else {
        // Prepare fills in ep0_buf
        len = usb_prepare_string_descriptor(dev_config.descriptor_strings[i - 1]);
    }

    usb_start_transfer(usb_get_endpoint_configuration(EP0_IN_ADDR), &ep0_buf[0], MIN(len, pkt->wLength));
}

/**
 * @brief Sends a zero length status packet back to the host.
 */
void usb_acknowledge_out_request(void) {
    usb_start_transfer(usb_get_endpoint_configuration(EP0_IN_ADDR), NULL, 0);
}

/**
 * @brief Handles a SET_ADDR request from the host. The actual setting of the device address in
 * hardware is done in ep0_in_handler. This is because we have to acknowledge the request first
 * as a device with address zero.
 *
 * @param pkt, the setup packet from the host.
 */
void usb_set_device_address(volatile struct usb_setup_packet *pkt) {
    // Set address is a bit of a strange case because we have to send a 0 length status packet first with
    // address 0
    dev_addr = (pkt->wValue & 0xff);
    // Will set address in the callback phase
    should_set_address = true;
    usb_acknowledge_out_request();
}

/**
 * @brief Handles a SET_CONFIGRUATION request from the host. Assumes one configuration so simply
 * sends a zero length status packet back to the host.
 *
 * @param pkt, the setup packet from the host.
 */
void usb_set_device_configuration(volatile struct usb_setup_packet *pkt) {
    // Only one configuration so just acknowledge the request
    usb_acknowledge_out_request();
    configured = true;
}

/**
 * @brief Respond to a setup packet from the host.
 *
 */
void usb_handle_setup_packet(void) {
    volatile struct usb_setup_packet *pkt = (volatile struct usb_setup_packet *) &usb_dpram->setup_packet;
    uint8_t req_direction = pkt->bmRequestType;
    uint8_t req = pkt->bRequest;

    // Reset PID to 1 for EP0 IN
    usb_get_endpoint_configuration(EP0_IN_ADDR)->next_pid = 1u;

    if (req_direction == USB_DIR_OUT) {
        if (req == USB_REQUEST_SET_ADDRESS) {
            usb_set_device_address(pkt);
        } else if (req == USB_REQUEST_SET_CONFIGURATION) {
            usb_set_device_configuration(pkt);
        } else {
            usb_acknowledge_out_request();
        }
    } else if (req_direction == USB_DIR_IN) {
        if (req == USB_REQUEST_GET_DESCRIPTOR) {
            uint16_t descriptor_type = pkt->wValue >> 8;

            switch (descriptor_type) {
                case USB_DT_DEVICE:
                    usb_handle_device_descriptor(pkt);
                    break;

                case USB_DT_CONFIG:
                    usb_handle_config_descriptor(pkt);
                    break;

                case USB_DT_STRING:
                    usb_handle_string_descriptor(pkt);
                    break;

                default:
                    // Do nothing
                    break;
            }
        } else {
            // Do nothing
        }
    }
}

/**
 * @brief Notify an endpoint that a transfer has completed.
 *
 * @param ep, the endpoint to notify.
 */
static void usb_handle_ep_buff_done(struct usb_endpoint_configuration *ep) {
    uint32_t buffer_control = *ep->buffer_control;
    // Get the transfer length for this endpoint
    uint16_t len = buffer_control & USB_BUF_CTRL_LEN_MASK;

    // Call that endpoints buffer done handler
    ep->handler((uint8_t *) ep->data_buffer, len);
}

/**
 * @brief Find the endpoint configuration for a specified endpoint number and
 * direction and notify it that a transfer has completed.
 *
 * @param ep_num
 * @param in
 */
static void usb_handle_buff_done(uint ep_num, bool in) {
    uint8_t ep_addr = ep_num | (in ? USB_DIR_IN : 0);
    for (uint i = 0; i < USB_NUM_ENDPOINTS_NEW; i++) {
        struct usb_endpoint_configuration *ep = &dev_config.endpoints[i];
        if (ep->descriptor && ep->handler) {
            if (ep->descriptor->bEndpointAddress == ep_addr) {
                usb_handle_ep_buff_done(ep);
                return;
            }
        }
    }
}

/**
 * @brief Handle a "buffer status" irq. This means that one or more
 * buffers have been sent / received. Notify each endpoint where this
 * is the case.
 */
static void usb_handle_buff_status() {
    uint32_t buffers = usb_hw->buf_status;
    uint32_t remaining_buffers = buffers;

    uint bit = 1u;
    for (uint i = 0; remaining_buffers && i < USB_NUM_ENDPOINTS_NEW * 2; i++) {
        if (remaining_buffers & bit) {
            // clear this in advance
            usb_hw_clear->buf_status = bit;
            // IN transfer for even i, OUT transfer for odd i
            usb_handle_buff_done(i >> 1u, !(i & 1u));
            remaining_buffers &= ~bit;
        }
        bit <<= 1u;
    }
}

/**
 * @brief USB interrupt handler
 *
 */
/// \tag::isr_setup_packet[]
void isr_usbctrl(void) {
    // USB interrupt handler
    uint32_t status = usb_hw->ints;
    uint32_t handled = 0;

    // Setup packet received
    if (status & USB_INTS_SETUP_REQ_BITS) {
        handled |= USB_INTS_SETUP_REQ_BITS;
        usb_hw_clear->sie_status = USB_SIE_STATUS_SETUP_REC_BITS;
        usb_handle_setup_packet();
    }
/// \end::isr_setup_packet[]

    // Buffer status, one or more buffers have completed
    if (status & USB_INTS_BUFF_STATUS_BITS) {
        handled |= USB_INTS_BUFF_STATUS_BITS;
        usb_handle_buff_status();
    }

    // Bus is reset
    if (status & USB_INTS_BUS_RESET_BITS) {
        handled |= USB_INTS_BUS_RESET_BITS;
        usb_hw_clear->sie_status = USB_SIE_STATUS_BUS_RESET_BITS;
        usb_bus_reset();
    }

    if (status ^ handled) {
        panic("Unhandled IRQ 0x%x\n", (uint) (status ^ handled));
    }
}

/**
 * @brief EP0 in transfer complete. Either finish the SET_ADDRESS process, or receive a zero
 * length status packet from the host.
 *
 * @param buf the data that was sent
 * @param len the length that was sent
 */
void ep0_in_handler(uint8_t *buf, uint16_t len) {
    if (should_set_address) {
        // Set actual device address in hardware
        usb_hw->dev_addr_ctrl = dev_addr;
        should_set_address = false;
    } else {
        // Receive a zero length status packet from the host on EP0 OUT
        struct usb_endpoint_configuration *ep = usb_get_endpoint_configuration(EP0_OUT_ADDR);
        usb_start_transfer(ep, NULL, 0);
    }
}

void ep0_out_handler(uint8_t *buf, uint16_t len) {
    ;
}


// We block on late packets, which is not ideal. (Low bandwidth and high latency)
//  To get around this the host application will drop frame(s), if desired/required.
static void my_handler(uint8_t num) {
    static uint32_t state = 0;
    static const uint32_t limit = (1 << serial_get_chan_count()) - 1;

    num %= 15;
    state |= 1 << (num - 1);

    if (state == limit) {
        // TODO: Make sure DMA has finished!
        // TODO: DMA
        //  Use serial_get_chan_count

        state = 0;
    }

}

// Device specific functions
void ep1_out_handler(uint8_t *buf, uint16_t len) {
    my_handler(1);
}

void ep2_out_handler(uint8_t *buf, uint16_t len) {
    my_handler(2);
}

void ep3_out_handler(uint8_t *buf, uint16_t len) {
    my_handler(3);
}

void ep4_out_handler(uint8_t *buf, uint16_t len) {
    my_handler(4);
}

void ep5_out_handler(uint8_t *buf, uint16_t len) {
    my_handler(5);
}

void ep6_out_handler(uint8_t *buf, uint16_t len) {
    my_handler(6);
}

void ep7_out_handler(uint8_t *buf, uint16_t len) {
    my_handler(7);
}

void ep8_out_handler(uint8_t *buf, uint16_t len) {
    my_handler(8);
}

void ep9_out_handler(uint8_t *buf, uint16_t len) {
    my_handler(9);
}

void ep10_out_handler(uint8_t *buf, uint16_t len) {
    my_handler(10);
}

void ep11_out_handler(uint8_t *buf, uint16_t len) {
    my_handler(11);
}

void ep12_out_handler(uint8_t *buf, uint16_t len) {
    my_handler(12);
}

void ep13_out_handler(uint8_t *buf, uint16_t len) {
    my_handler(13);
}

void ep14_out_handler(uint8_t *buf, uint16_t len) {
    my_handler(14);
}

void ep15_out_handler(uint8_t *buf, uint16_t len) {
    my_handler(15);
}

static void kickoff() {
    uint8_t *ptr;
    uint16_t len;

    table = (table + 1) % 2;    // We do not actually use double buffer

    for (uint32_t i = 0; i < serial_get_chan_count(); i++)
        usb_start_transfer(usb_get_endpoint_configuration(USB_DIR_OUT + i), ep0_buf, 64);   // We do not actually use ep0_buf here!

    // Do not wait till USB finishes to do this!
    for (uint32_t i = 0; i < serial_get_chan_count(); i++) {    // TODO: Consider fixing this to no use loop (parallel load)
        func(&ptr, &len, i);
        address_table[table][i].len = len;
        address_table[table][i].dst = ptr;
        address_table[table][i].src = &usb_dpram->epx_data[i * 64];
    }
}

// TODO: Finish
void serial_usb_isr() {
     if (dma_channel_get_irq1_status(dma_chan[0])) {
        // TODO:
        //  Use serial_get_chan_count

        kickoff();
     }
}

// TODO: Finish
void serial_usb_start(serial_usb_callback callback, int dma_chan_num0, int dma_chan_num1) {
    usb_device_init();

    func = callback;
    dma_chan[0] = dma_chan_num0;
    dma_chan[1] = dma_chan_num1;

    // Wait until configured
    while (!configured) {
        tight_loop_contents();
    }

    // TODO: Prepare DMA
    //  Use serial_get_chan_count

    // Get ready to rx from host
    kickoff();
}
