/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "ir_nec_transceiver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"
#include "ir_nec_encoder.h"

#define EXAMPLE_IR_RESOLUTION_HZ     1000000  // 1MHz resolution, 1 tick = 1us
#define EXAMPLE_IR_NEC_DECODE_MARGIN 200      // Tolerance for parsing RMT symbols into bit stream

/**
 * @brief NEC timing spec
 */
#define NEC_LEADING_CODE_DURATION_0 9000
#define NEC_LEADING_CODE_DURATION_1 4500
#define NEC_PAYLOAD_ZERO_DURATION_0 560
#define NEC_PAYLOAD_ZERO_DURATION_1 560
#define NEC_PAYLOAD_ONE_DURATION_0  560
#define NEC_PAYLOAD_ONE_DURATION_1  1690
#define NEC_REPEAT_CODE_DURATION_0  9000
#define NEC_REPEAT_CODE_DURATION_1  2250

static const char* TAG = "example";

/**
 * @brief Saving NEC decode results
 */
static uint16_t s_nec_code_address;
static uint16_t s_nec_code_command;

/**
 * @brief Check whether a duration is within expected range
 */
static inline bool nec_check_in_range(uint32_t signal_duration, uint32_t spec_duration)
{
    return (signal_duration < (spec_duration + EXAMPLE_IR_NEC_DECODE_MARGIN)) &&
           (signal_duration > (spec_duration - EXAMPLE_IR_NEC_DECODE_MARGIN));
}

/**
 * @brief Check whether a RMT symbol represents NEC logic zero
 */
static bool nec_parse_logic0(rmt_symbol_word_t* rmt_nec_symbols)
{
    return nec_check_in_range(rmt_nec_symbols->duration0, NEC_PAYLOAD_ZERO_DURATION_0) &&
           nec_check_in_range(rmt_nec_symbols->duration1, NEC_PAYLOAD_ZERO_DURATION_1);
}

/**
 * @brief Check whether a RMT symbol represents NEC logic one
 */
static bool nec_parse_logic1(rmt_symbol_word_t* rmt_nec_symbols)
{
    return nec_check_in_range(rmt_nec_symbols->duration0, NEC_PAYLOAD_ONE_DURATION_0) &&
           nec_check_in_range(rmt_nec_symbols->duration1, NEC_PAYLOAD_ONE_DURATION_1);
}

/**
 * @brief Decode RMT symbols into NEC address and command
 */
static bool nec_parse_frame(rmt_symbol_word_t* rmt_nec_symbols)
{
    rmt_symbol_word_t* cur  = rmt_nec_symbols;
    uint16_t address        = 0;
    uint16_t command        = 0;
    bool valid_leading_code = nec_check_in_range(cur->duration0, NEC_LEADING_CODE_DURATION_0) &&
                              nec_check_in_range(cur->duration1, NEC_LEADING_CODE_DURATION_1);
    if (!valid_leading_code) {
        return false;
    }
    cur++;
    for (int i = 0; i < 16; i++) {
        if (nec_parse_logic1(cur)) {
            address |= 1 << i;
        } else if (nec_parse_logic0(cur)) {
            address &= ~(1 << i);
        } else {
            return false;
        }
        cur++;
    }
    for (int i = 0; i < 16; i++) {
        if (nec_parse_logic1(cur)) {
            command |= 1 << i;
        } else if (nec_parse_logic0(cur)) {
            command &= ~(1 << i);
        } else {
            return false;
        }
        cur++;
    }
    // save address and command
    s_nec_code_address = address;
    s_nec_code_command = command;
    return true;
}

/**
 * @brief Check whether the RMT symbols represent NEC repeat code
 */
static bool nec_parse_frame_repeat(rmt_symbol_word_t* rmt_nec_symbols)
{
    return nec_check_in_range(rmt_nec_symbols->duration0, NEC_REPEAT_CODE_DURATION_0) &&
           nec_check_in_range(rmt_nec_symbols->duration1, NEC_REPEAT_CODE_DURATION_1);
}

rmt_channel_handle_t tx_channel  = NULL;
rmt_encoder_handle_t nec_encoder = NULL;

void ir_nec_transceiver_init(int txPin)
{
    ESP_LOGI(TAG, "create RMT TX channel");
    rmt_tx_channel_config_t tx_channel_cfg = {
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = EXAMPLE_IR_RESOLUTION_HZ,
        .mem_block_symbols = 64,  // amount of RMT symbols that the channel can store at a time
        .trans_queue_depth = 4,  // number of transactions that allowed to pending in the background, this example won't
                                 // queue multiple transactions, so queue depth > 1 is sufficient
        .gpio_num = (gpio_num_t)txPin,
    };
    // rmt_channel_handle_t tx_channel = NULL;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_channel_cfg, &tx_channel));

    ESP_LOGI(TAG, "modulate carrier to TX channel");
    rmt_carrier_config_t carrier_cfg = {
        .duty_cycle   = 0.33,
        .frequency_hz = 38000,  // 38KHz
    };
    ESP_ERROR_CHECK(rmt_apply_carrier(tx_channel, &carrier_cfg));

    ESP_LOGI(TAG, "install IR NEC encoder");
    ir_nec_encoder_config_t nec_encoder_cfg = {
        .resolution = EXAMPLE_IR_RESOLUTION_HZ,
    };
    // rmt_encoder_handle_t nec_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_ir_nec_encoder(&nec_encoder_cfg, &nec_encoder));

    ESP_LOGI(TAG, "enable RMT TX and RX channels");
    ESP_ERROR_CHECK(rmt_enable(tx_channel));
    // ESP_ERROR_CHECK(rmt_enable(rx_channel));
}

// this example won't send NEC frames in a loop
rmt_transmit_config_t transmit_config = {
    .loop_count = 0,  // no loop
};

void ir_nec_transceiver_send(uint16_t addr, uint16_t cmmd)
{
    const ir_nec_scan_code_t scan_code = {
        .address = addr,
        .command = cmmd,
    };
    ESP_ERROR_CHECK(rmt_transmit(tx_channel, nec_encoder, &scan_code, sizeof(scan_code), &transmit_config));
}
