/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

#include "iot_button.h"
#include "button_gpio.h"

#include "led_strip.h"

#define BOOT_BUTTON_NUM       39
#define BUTTON_ACTIVE_LEVEL   0

#define LED_STRIP_BLINK_GPIO  27
#define LED_STRIP_LED_NUMBERS 1
#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)

static const char *TAG = "LED_STRIP";

uint8_t button_count = 0;
led_strip_handle_t led_strip;

void button_event_cb(void *arg, void *data)
{
    iot_button_print_event((button_handle_t)arg);
    switch (button_count) {
        case 0:
            led_strip_set_pixel(led_strip, 0, 255, 0, 0);
            break;
        case 1:
            led_strip_set_pixel(led_strip, 0, 0, 255, 0);
            break;
        case 2:
            led_strip_set_pixel(led_strip, 0, 0, 0, 255);
            break;
    }
    led_strip_refresh(led_strip);
    button_count = button_count >= 2 ? 0 : button_count + 1;
}
void app_main(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_BLINK_GPIO,   // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_NUMBERS,        // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_SK6812,            // LED strip model
        .flags.invert_out = false,                // whether to invert the output signal
    };

    led_strip_rmt_config_t rmt_config = {
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .rmt_channel = 0,
#else
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = LED_STRIP_RMT_RES_HZ,
        .flags.with_dma = false,
#endif
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");
    led_strip_set_pixel(led_strip, 0, 0, 255, 0);
    button_count = 2;
    led_strip_refresh(led_strip);

    button_config_t btn_cfg = {0};
    button_gpio_config_t gpio_cfg = {
        .gpio_num = BOOT_BUTTON_NUM,
        .active_level = BUTTON_ACTIVE_LEVEL,
        .enable_power_save = true,
    };

    button_handle_t btn_handle = NULL;
    ESP_ERROR_CHECK(iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &btn_handle));
    assert(btn_handle != NULL);

    ESP_ERROR_CHECK(iot_button_register_cb(btn_handle, BUTTON_PRESS_DOWN, NULL, button_event_cb, NULL));
    
    while (1) 
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
