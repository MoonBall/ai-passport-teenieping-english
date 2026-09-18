#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"       // 错误日志里要打印 BSP_LCD_* 引脚号
#include "spoken_app.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "lvgl.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "main";

#define FAP_SCREENSHOT_COMMAND "FAP_SCREENSHOT_V1\n"
#define FAP_SCREENSHOT_HEADER_BYTES 64u
#define FAP_SCREENSHOT_TX_CHUNK_BYTES 128u
#define FAP_SCREENSHOT_PAYLOAD_BYTES \
    ((size_t)BSP_LCD_W * BSP_LCD_H * sizeof(uint16_t))

typedef struct {
    bool active;
    bool failed;
    uint16_t next_y;
    size_t bytes_sent;
} screenshot_capture_t;

static lv_display_t *s_screenshot_display;
static screenshot_capture_t s_screenshot;

static bool screenshot_write(const void *data, size_t bytes)
{
    const uint8_t *cursor = data;
    while (bytes > 0) {
        size_t chunk = bytes > FAP_SCREENSHOT_TX_CHUNK_BYTES
                           ? FAP_SCREENSHOT_TX_CHUNK_BYTES
                           : bytes;
        int sent = usb_serial_jtag_write_bytes(cursor, chunk,
                                               pdMS_TO_TICKS(10000));
        if (sent != (int)chunk) return false;
        cursor += chunk;
        bytes -= chunk;
    }
    return true;
}

// Stream each full-width LVGL render stripe before the panel driver swaps its
// RGB565 bytes. This reuses the existing 20-row draw buffer instead of
// allocating a second full-screen image.
static void screenshot_flush_event(lv_event_t *event)
{
    if (!s_screenshot.active || s_screenshot.failed) return;

    lv_area_t *area = lv_event_get_param(event);
    lv_display_t *display = lv_event_get_target(event);
    lv_draw_buf_t *draw_buf = lv_display_get_buf_active(display);
    if (!area || !draw_buf || !draw_buf->data) {
        s_screenshot.failed = true;
        return;
    }
    int32_t width = lv_area_get_width(area);
    int32_t height = lv_area_get_height(area);
    size_t bytes = (size_t)width * height * sizeof(uint16_t);
    if (area->x1 != 0 || area->x2 != BSP_LCD_W - 1 ||
        area->y1 != s_screenshot.next_y ||
        area->y2 >= BSP_LCD_H || width != BSP_LCD_W || height <= 0 ||
        draw_buf->header.cf != LV_COLOR_FORMAT_RGB565 ||
        draw_buf->header.stride != BSP_LCD_W * sizeof(uint16_t)) {
        s_screenshot.failed = true;
        return;
    }

    if (!screenshot_write(draw_buf->data, bytes)) {
        s_screenshot.failed = true;
        return;
    }
    s_screenshot.next_y = (uint16_t)(area->y2 + 1);
    s_screenshot.bytes_sent += bytes;
}

// Force one full refresh and stream its RGB565 stripes as one protocol reply.
static bool send_screenshot(void)
{
    char header[FAP_SCREENSHOT_HEADER_BYTES];
    int header_len = snprintf(header, sizeof(header),
                              "FAP_SCREENSHOT_V1 %u %u RGB565LE %u\n",
                              (unsigned)BSP_LCD_W, (unsigned)BSP_LCD_H,
                              (unsigned)FAP_SCREENSHOT_PAYLOAD_BYTES);
    if (!s_screenshot_display || header_len <= 0 ||
        header_len >= (int)sizeof(header) || !bsp_lvgl_lock(5000)) return false;

    // Logs share this USB stream; silence them only while the binary frame is
    // in flight so no unrelated text can corrupt the declared payload.
    esp_log_level_t previous_log_level = esp_log_get_level_master();
    esp_log_set_level_master(ESP_LOG_NONE);
    bool header_sent = screenshot_write(header, (size_t)header_len);
    memset(&s_screenshot, 0, sizeof(s_screenshot));
    if (header_sent) {
        s_screenshot.active = true;
        lv_obj_invalidate(lv_screen_active());
        lv_refr_now(s_screenshot_display);
        s_screenshot.active = false;
    } else {
        s_screenshot.failed = true;
    }
    esp_err_t flushed = usb_serial_jtag_wait_tx_done(pdMS_TO_TICKS(10000));
    esp_log_set_level_master(previous_log_level);
    bsp_lvgl_unlock();

    if (s_screenshot.failed || s_screenshot.next_y != BSP_LCD_H ||
        s_screenshot.bytes_sent != FAP_SCREENSHOT_PAYLOAD_BYTES ||
        flushed != ESP_OK) {
        ESP_LOGE(TAG, "screen response failed: rows=%u bytes=%u flush=%s",
                 (unsigned)s_screenshot.next_y,
                 (unsigned)s_screenshot.bytes_sent, esp_err_to_name(flushed));
        return false;
    }
    return true;
}

static void screenshot_task(void *arg)
{
    (void)arg;
    char line[48];
    size_t used = 0;
    bool overflow = false;

    while (true) {
        uint8_t byte;
        if (usb_serial_jtag_read_bytes(&byte, 1, portMAX_DELAY) != 1) continue;
        if (byte == '\n') {
            line[used] = '\0';
            if (!overflow) {
                if (strcmp(line, "FAP_SCREENSHOT_V1") == 0) {
                    send_screenshot();
                } else if (strcmp(line, "FAP_KEY_UP_V1") == 0) {
                    spoken_app_key_cb(BSP_BTN_UP, BSP_BTN_CLICK, NULL);
                } else if (strcmp(line, "FAP_KEY_DOWN_V1") == 0) {
                    spoken_app_key_cb(BSP_BTN_DOWN, BSP_BTN_CLICK, NULL);
                } else if (strcmp(line, "FAP_KEY_OK_LONG_V1") == 0) {
                    spoken_app_key_cb(BSP_BTN_OK, BSP_BTN_LONG, NULL);
                } else if (strcmp(line, "FAP_KEY_OK_V1") == 0) {
                    spoken_app_key_cb(BSP_BTN_OK, BSP_BTN_CLICK, NULL);
                }
            }
            used = 0;
            overflow = false;
        } else if (!overflow && used < sizeof(line) - 1) {
            line[used++] = (char)byte;
        } else {
            overflow = true;
        }
    }
}

static void screenshot_start(lv_display_t *display)
{
    if (!usb_serial_jtag_is_driver_installed()) {
        usb_serial_jtag_driver_config_t config =
            USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
        if (usb_serial_jtag_driver_install(&config) != ESP_OK) {
            ESP_LOGE(TAG, "USB serial driver initialization failed");
            return;
        }
    }
    usb_serial_jtag_vfs_use_driver();
    if (!bsp_lvgl_lock(1000)) {
        ESP_LOGE(TAG, "screenshot display registration failed");
        return;
    }
    s_screenshot_display = display;
    lv_display_add_event_cb(display, screenshot_flush_event,
                            LV_EVENT_FLUSH_START, NULL);
    bsp_lvgl_unlock();
    if (xTaskCreate(screenshot_task, "fap_screenshot", 12288, NULL, 4, NULL) !=
        pdPASS) {
        ESP_LOGE(TAG, "screenshot task creation failed");
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Teenieping spoken English application starting");

    bsp_i2c_init();
    bsp_i2c_scan();

    lv_display_t *display = NULL;
    if (bsp_display_init() != ESP_OK || !(display = bsp_lvgl_init())) {
        ESP_LOGE(TAG, "显示/LVGL 初始化失败。检查 SPI 接线(MOSI=%d SCLK=%d CS=%d DC=%d BL=%d)",
                 BSP_LCD_MOSI, BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL);
        return;
    }
    bsp_display_backlight(100);

    if (bsp_button_init(spoken_app_key_cb, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "按键初始化失败,设备将无法操作");
    }
    if (bsp_battery_init() != ESP_OK) {
        ESP_LOGW(TAG, "电量计初始化失败,电量指示隐藏");
    }
    bool audio_ready = bsp_audio_init() == ESP_OK;
    if (!audio_ready) {
        ESP_LOGW(TAG, "音频初始化失败,卡片语音将禁用");
    }

    if (bsp_lvgl_lock(1000)) {
        spoken_app_start(audio_ready);
        bsp_lvgl_unlock();
    }

    ESP_LOGI(TAG, "ready: battery=%d audio=%d",
             (int)(bsp_battery_soc() >= 0),
             (int)audio_ready);
    screenshot_start(display);
}
