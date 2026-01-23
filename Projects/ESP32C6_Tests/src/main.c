#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"

#define YELLOW_LED_PIN  GPIO_NUM_15
#define RGB_LED_PIN     GPIO_NUM_8

static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t led_encoder = NULL;

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} ws2812_encoder_t;

static size_t ws2812_encode(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
                            const void *data, size_t size, rmt_encode_state_t *ret_state)
{
    ws2812_encoder_t *ws = __containerof(encoder, ws2812_encoder_t, base);
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded = 0;

    switch (ws->state) {
    case 0:
        encoded += ws->bytes_encoder->encode(ws->bytes_encoder, channel, data, size, &state);
        if (state & RMT_ENCODING_COMPLETE) {
            ws->state = 1;
        }
        if (state & RMT_ENCODING_MEM_FULL) {
            *ret_state = state;
            return encoded;
        }
        // fall through
    case 1:
        encoded += ws->copy_encoder->encode(ws->copy_encoder, channel, &ws->reset_code,
                                            sizeof(ws->reset_code), &state);
        if (state & RMT_ENCODING_COMPLETE) {
            ws->state = RMT_ENCODING_RESET;
            *ret_state = RMT_ENCODING_COMPLETE;
        }
        break;
    }
    return encoded;
}

static esp_err_t ws2812_del(rmt_encoder_t *encoder)
{
    ws2812_encoder_t *ws = __containerof(encoder, ws2812_encoder_t, base);
    rmt_del_encoder(ws->bytes_encoder);
    rmt_del_encoder(ws->copy_encoder);
    free(ws);
    return ESP_OK;
}

static esp_err_t ws2812_reset(rmt_encoder_t *encoder)
{
    ws2812_encoder_t *ws = __containerof(encoder, ws2812_encoder_t, base);
    rmt_encoder_reset(ws->bytes_encoder);
    rmt_encoder_reset(ws->copy_encoder);
    ws->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

static esp_err_t ws2812_init(void)
{
    uint32_t resolution = 10000000; // 10 MHz

    rmt_tx_channel_config_t tx_cfg = {
        .gpio_num = RGB_LED_PIN,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = resolution,
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
    };
    
    esp_err_t err = rmt_new_tx_channel(&tx_cfg, &led_chan);
    if (err != ESP_OK) {
        printf("Failed to create RMT channel: %s\n", esp_err_to_name(err));
        return err;
    }

    ws2812_encoder_t *ws = calloc(1, sizeof(ws2812_encoder_t));
    ws->base.encode = ws2812_encode;
    ws->base.del = ws2812_del;
    ws->base.reset = ws2812_reset;

    rmt_bytes_encoder_config_t bytes_cfg = {
        .bit0 = { .level0 = 1, .duration0 = 4, .level1 = 0, .duration1 = 8 },
        .bit1 = { .level0 = 1, .duration0 = 7, .level1 = 0, .duration1 = 6 },
        .flags.msb_first = 1,
    };
    rmt_new_bytes_encoder(&bytes_cfg, &ws->bytes_encoder);

    rmt_copy_encoder_config_t copy_cfg = {};
    rmt_new_copy_encoder(&copy_cfg, &ws->copy_encoder);

    ws->reset_code = (rmt_symbol_word_t){ .level0 = 0, .duration0 = 500, .level1 = 0, .duration1 = 500 };

    led_encoder = &ws->base;
    
    err = rmt_enable(led_chan);
    if (err != ESP_OK) {
        printf("Failed to enable RMT: %s\n", esp_err_to_name(err));
        return err;
    }
    
    return ESP_OK;
}

static void set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t grb[3] = {g, r, b};
    rmt_transmit_config_t cfg = { .loop_count = 0 };
    rmt_transmit(led_chan, led_encoder, grb, 3, &cfg);
    rmt_tx_wait_all_done(led_chan, portMAX_DELAY);
}

void app_mainx(void)
{
    // Set up yellow LED
    gpio_reset_pin(YELLOW_LED_PIN);
    gpio_set_direction(YELLOW_LED_PIN, GPIO_MODE_OUTPUT);

    // Set up RGB LED
    esp_err_t rgb_ok = ws2812_init();
    
    printf("LEDs started (RGB init: %s)\n", rgb_ok == ESP_OK ? "OK" : "FAILED");

    int count = 0;
    while (1) {
        // Yellow LED on, RGB red
        gpio_set_level(YELLOW_LED_PIN, 1);
        if (rgb_ok == ESP_OK) set_rgb(32, 0, 0);
        printf("%d: Yellow ON, RGB Red\n", count);
        vTaskDelay(pdMS_TO_TICKS(500));

        // Yellow LED off, RGB green
        gpio_set_level(YELLOW_LED_PIN, 0);
        if (rgb_ok == ESP_OK) set_rgb(0, 32, 0);
        printf("%d: Yellow OFF, RGB Green\n", count);
        vTaskDelay(pdMS_TO_TICKS(1500));

        // Yellow LED on, RGB blue
        gpio_set_level(YELLOW_LED_PIN, 1);
        if (rgb_ok == ESP_OK) set_rgb(0, 0, 32);
        printf("%d: Yellow ON, RGB Blue\n", count);
        vTaskDelay(pdMS_TO_TICKS(500));

        // Yellow LED off, RGB off
        gpio_set_level(YELLOW_LED_PIN, 0);
        if (rgb_ok == ESP_OK) set_rgb(0, 0, 0);
        printf("%d: Yellow OFF, RGB Off\n", count);
        vTaskDelay(pdMS_TO_TICKS(1500));

        count++;
    }
}