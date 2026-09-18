#include "spoken_app.h"

#include "adpcm_ima.h"
#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_display.h"
#include "spoken_core.h"
#include "spoken_data.h"
#include "ui_pixel.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define AUDIO_CHUNK 128

typedef struct {
    size_t index;
    uint32_t generation;
    spoken_mode_t mode;
} audio_request_t;

typedef struct {
    uint32_t generation;
    uint32_t played_ms;
    bool done;
    bool error;
} audio_status_t;

typedef struct {
    bsp_btn_t button;
    bsp_btn_ev_t event;
} button_event_t;

static const char *TAG = "spoken_app";
static spoken_state_t s_state;
static QueueHandle_t s_requests;
static QueueHandle_t s_audio_status;
static QueueHandle_t s_buttons;
static bool s_audio_ready;
static uint32_t s_started;
static uint32_t s_audio_ms;
static uint32_t s_animation_started;
static size_t s_frame;
static size_t s_stage;
static lv_obj_t *s_title;
static lv_obj_t *s_image;
static lv_obj_t *s_command;
static lv_obj_t *s_meaning;
static lv_obj_t *s_counter;
static lv_obj_t *s_status;
static lv_obj_t *s_battery;
static lv_obj_t *s_progress;
static lv_timer_t *s_idle;

static void publish_audio(const audio_request_t *request, uint32_t samples, bool done, bool error)
{
    audio_status_t status = {.generation=request->generation,
                            .played_ms=samples * 1000ULL / SPOKEN_SAMPLE_RATE,
                            .done=done, .error=error};
    xQueueOverwrite(s_audio_status, &status);
}

static bool play_audio(const audio_request_t *request, audio_request_t *next)
{
    if (request->mode == SPOKEN_STOPPED) return false;
    const spoken_card_t *card = &SPOKEN_CARDS[request->index];
    const spoken_audio_t *clip = request->mode == SPOKEN_LESSON
                                    ? &card->lesson_audio : &card->short_audio;
    uint8_t encoded[AUDIO_CHUNK];
    int16_t mono[AUDIO_CHUNK * 2];
    int16_t stereo[AUDIO_CHUNK * 4];
    adpcm_ima_t decoder;
    adpcm_ima_init(&decoder);
    size_t offset = 0;
    uint32_t played = 0;
    if (bsp_audio_set_format(SPOKEN_SAMPLE_RATE, 16, 2) != ESP_OK) {
        publish_audio(request, 0, true, true);
        return false;
    }
    bsp_audio_set_volume(82);
    ESP_LOGI(TAG, "audio start %u %s mode=%u generation=%lu",
             (unsigned)request->index + 1, card->id, request->mode,
             (unsigned long)request->generation);
    while (offset < clip->bytes && played < clip->samples) {
        if (xQueueReceive(s_requests, next, 0) == pdTRUE) {
            ESP_LOGI(TAG, "audio interrupted generation=%lu", (unsigned long)request->generation);
            return true;
        }
        size_t bytes = clip->bytes - offset;
        if (bytes > AUDIO_CHUNK) bytes = AUDIO_CHUNK;
        if (!spoken_audio_read(clip->offset + offset, encoded, bytes)) {
            publish_audio(request, played, true, true);
            return false;
        }
        size_t samples = adpcm_ima_decode(&decoder, encoded, bytes, mono);
        if (samples > clip->samples - played) samples = clip->samples - played;
        for (size_t i = 0; i < samples; ++i) {
            stereo[i * 2] = mono[i];
            stereo[i * 2 + 1] = mono[i];
        }
        if (bsp_audio_write(stereo, samples * 2 * sizeof(int16_t)) != ESP_OK) {
            publish_audio(request, played, true, true);
            return false;
        }
        offset += bytes;
        played += samples;
        publish_audio(request, played, false, false);
    }
    ESP_LOGI(TAG, "audio complete %u %s mode=%u", (unsigned)request->index + 1, card->id, request->mode);
    publish_audio(request, played, true, false);
    return false;
}

static void audio_worker(void *arg)
{
    (void)arg;
    audio_request_t request;
    while (true) {
        if (xQueueReceive(s_requests, &request, portMAX_DELAY) != pdTRUE) continue;
        audio_request_t next;
        while (play_audio(&request, &next)) request = next;
    }
}

static void update_battery(lv_timer_t *timer)
{
    (void)timer;
    int level = bsp_battery_soc();
    if (level >= 0) lv_label_set_text_fmt(s_battery, "%d%%", level);
    else lv_label_set_text(s_battery, "--%");
}

static void dim(lv_timer_t *timer)
{
    (void)timer;
    if (s_state.mode == SPOKEN_STOPPED) bsp_display_backlight(18);
}

static void show_frame(size_t frame)
{
    if (frame == s_frame) return;
    s_frame = frame;
    const spoken_card_t *card = &SPOKEN_CARDS[s_state.index];
    lv_image_set_src(s_image, &card->frames[frame]);
}

static void show_stage(size_t stage)
{
    const spoken_card_t *card = &SPOKEN_CARDS[s_state.index];
    s_stage = stage;
    lv_obj_set_style_text_font(s_command, &ui_font_spoken_16, 0);
    lv_obj_set_height(s_command, 66);
    lv_obj_set_pos(s_command, 14, 210);
    lv_obj_remove_flag(s_meaning, LV_OBJ_FLAG_HIDDEN);
    if (s_state.mode != SPOKEN_LESSON || stage == 1) {
        lv_label_set_text(s_command, card->command);
        lv_obj_set_style_text_font(s_command, &lv_font_montserrat_20, 0);
        lv_obj_set_height(s_command, 50);
        lv_label_set_text(s_meaning, card->meaning);
        if (s_state.mode == SPOKEN_LESSON) lv_label_set_text(s_title, "2/4 英语指令");
        else lv_label_set_text(s_title, card->title);
    } else {
        const char *text = stage == 0 ? card->scene : stage == 2 ? card->response : card->usage;
        lv_label_set_text(s_command, text);
        lv_obj_add_flag(s_meaning, LV_OBJ_FLAG_HIDDEN);
        static const char *titles[] = {"1/4 场景", "2/4 英语指令", "3/4 怎样回应", "4/4 适用场景"};
        lv_label_set_text(s_title, titles[stage]);
    }
    ESP_LOGI(TAG, "stage %u card=%u", (unsigned)stage, (unsigned)s_state.index + 1);
}

static void show_card(void)
{
    s_frame = SIZE_MAX;
    show_frame(0);
    show_stage(0);
    lv_label_set_text_fmt(s_counter, "%02u / 17", (unsigned)s_state.index + 1);
    lv_obj_set_width(s_progress, 0);
    ESP_LOGI(TAG, "card %u/17: %s", (unsigned)s_state.index + 1, SPOKEN_CARDS[s_state.index].id);
}

static void send_request(void)
{
    if (!s_audio_ready) return;
    audio_request_t request = {.index=s_state.index, .generation=s_state.generation, .mode=s_state.mode};
    xQueueOverwrite(s_requests, &request);
}

static void change_playback(spoken_mode_t mode)
{
    if (!s_audio_ready && mode != SPOKEN_STOPPED) {
        lv_label_set_text(s_status, "语音不可用");
        return;
    }
    spoken_core_start(&s_state, mode);
    s_started = lv_tick_get();
    s_audio_ms = 0;
    s_animation_started = s_started;
    send_request();
    s_frame = SIZE_MAX;
    show_frame(0);
    show_stage(0);
    lv_label_set_text(s_status, mode == SPOKEN_STOPPED ? "短按播放  长按讲解" : "短按停止  长按讲解");
    if (mode == SPOKEN_STOPPED) lv_obj_set_width(s_progress, 0);
    ESP_LOGI(TAG, "mode=%u card=%u generation=%lu", mode, (unsigned)s_state.index + 1,
             (unsigned long)s_state.generation);
}

static void tick(lv_timer_t *timer)
{
    (void)timer;
    button_event_t event;
    while (xQueueReceive(s_buttons, &event, 0) == pdTRUE) {
        bsp_display_backlight(100);
        lv_timer_reset(s_idle);
        if (event.button == BSP_BTN_OK) {
            spoken_mode_t mode = event.event == BSP_BTN_LONG ? SPOKEN_LESSON
                                 : s_state.mode == SPOKEN_STOPPED ? SPOKEN_SHORT : SPOKEN_STOPPED;
            change_playback(mode);
        } else {
            spoken_core_move(&s_state, event.button == BSP_BTN_UP ? -1 : 1, SPOKEN_CARD_COUNT);
            send_request();
            show_card();
            lv_label_set_text(s_status, "短按播放  长按讲解");
        }
    }
    audio_status_t status;
    if (xQueueReceive(s_audio_status, &status, 0) == pdTRUE && status.generation == s_state.generation) {
        s_audio_ms = status.played_ms;
        if (status.done) spoken_core_audio_done(&s_state, status.generation);
        if (status.error) {
            change_playback(SPOKEN_STOPPED);
            lv_label_set_text(s_status, "播放失败，请重试");
            ESP_LOGE(TAG, "audio failure");
        }
    }
    if (s_state.mode == SPOKEN_STOPPED) return;
    const spoken_card_t *card = &SPOKEN_CARDS[s_state.index];
    uint32_t elapsed = lv_tick_elaps(s_started);
    uint32_t animation_ms = card->frame_count > 1 ? card->frame_count * card->frame_ms : 4000;
    if (s_state.mode == SPOKEN_LESSON) {
        size_t stage = spoken_core_stage(s_audio_ms, card->stages_ms);
        if (stage != s_stage) {
            show_stage(stage);
            if (stage == 1) s_animation_started = lv_tick_get();
        }
        if (s_stage == 1) {
            uint32_t motion_ms = lv_tick_elaps(s_animation_started);
            show_frame(spoken_core_frame(motion_ms, card->frame_count, card->frame_ms));
        }
    } else {
        show_frame(spoken_core_frame(elapsed, card->frame_count, card->frame_ms));
    }
    uint32_t duration = s_state.mode == SPOKEN_LESSON ? card->lesson_audio.duration_ms : card->short_audio.duration_ms;
    if (s_state.mode == SPOKEN_SHORT && duration < animation_ms) duration = animation_ms;
    uint32_t progress = s_state.mode == SPOKEN_LESSON ? s_audio_ms : elapsed;
    lv_obj_set_width(s_progress, progress >= duration ? 220 : 220ULL * progress / duration);
    if (spoken_core_finish(&s_state, elapsed, animation_ms)) {
        lv_label_set_text(s_status, "短按播放  长按讲解");
        show_stage(0);
        lv_obj_set_width(s_progress, 220);
        ESP_LOGI(TAG, "playback complete card=%u", (unsigned)s_state.index + 1);
        lv_timer_reset(s_idle);
    }
}

void spoken_app_start(bool audio_ready)
{
    spoken_core_init(&s_state);
    s_requests = xQueueCreate(1, sizeof(audio_request_t));
    s_audio_status = xQueueCreate(1, sizeof(audio_status_t));
    s_buttons = xQueueCreate(12, sizeof(button_event_t));
    s_audio_ready = audio_ready && s_requests && s_audio_status && spoken_audio_init();
    lv_obj_t *screen = ui_pixel_screen_create("");
    s_battery = ui_pixel_label(screen, "--%", &lv_font_montserrat_14, UI_SUBTLE);
    lv_obj_set_pos(s_battery, 184, 0);
    lv_obj_set_width(s_battery, 46);
    lv_obj_set_style_text_align(s_battery, LV_TEXT_ALIGN_RIGHT, 0);
    s_title = ui_pixel_label(screen, "", &ui_font_spoken_14, UI_INK);
    lv_obj_set_pos(s_title, 10, 29);
    lv_obj_set_width(s_title, 155);
    s_counter = ui_pixel_label(screen, "", &lv_font_montserrat_14, UI_SUBTLE);
    lv_obj_set_pos(s_counter, 164, 29);
    lv_obj_set_width(s_counter, 66);
    lv_obj_set_style_text_align(s_counter, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_t *panel = ui_pixel_panel_create(screen, 10, 59, 220, 148, 0xF5F0E7);
    lv_obj_set_style_pad_all(panel, 0, 0);
    s_image = lv_image_create(panel);
    lv_obj_center(s_image);
    s_command = ui_pixel_label(screen, "", &lv_font_montserrat_20, UI_INK);
    lv_obj_set_width(s_command, 212);
    lv_obj_set_style_text_align(s_command, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(s_command, LV_LABEL_LONG_WRAP);
    s_meaning = ui_pixel_label(screen, "", &ui_font_spoken_14, UI_SUBTLE);
    lv_obj_set_pos(s_meaning, 10, 261);
    lv_obj_set_width(s_meaning, 220);
    lv_obj_set_style_text_align(s_meaning, LV_TEXT_ALIGN_CENTER, 0);
    s_progress = lv_obj_create(screen);
    lv_obj_remove_flag(s_progress, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_progress, 10, 284);
    lv_obj_set_height(s_progress, 3);
    lv_obj_set_style_border_width(s_progress, 0, 0);
    lv_obj_set_style_bg_color(s_progress, lv_color_hex(0xDF76AD), 0);
    lv_obj_set_style_pad_all(s_progress, 0, 0);
    s_status = ui_pixel_label(screen, "短按播放  长按讲解", &ui_font_spoken_14, UI_INK);
    lv_obj_set_pos(s_status, 10, 295);
    lv_obj_set_width(s_status, 220);
    lv_obj_set_style_text_align(s_status, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_t *nav = ui_pixel_label(screen, "上下翻卡", &ui_font_spoken_14, UI_SUBTLE);
    lv_obj_set_pos(nav, 10, 295);
    show_card();
    lv_screen_load(screen);
    s_idle = lv_timer_create(dim, 60000, NULL);
    lv_timer_t *battery_timer = lv_timer_create(update_battery, 5000, NULL);
    lv_timer_ready(battery_timer);
    if (s_buttons && s_audio_status) lv_timer_create(tick, 20, NULL);
    if (s_audio_ready && xTaskCreate(audio_worker, "spoken_audio", 4096, NULL, 5, NULL) != pdPASS) s_audio_ready = false;
    if (!s_audio_ready) lv_label_set_text(s_status, "语音不可用");
}

void spoken_app_key_cb(bsp_btn_t button, bsp_btn_ev_t event, void *user)
{
    (void)user;
    if (!s_buttons || (event != BSP_BTN_CLICK && !(event == BSP_BTN_LONG && button == BSP_BTN_OK))) return;
    button_event_t queued = {.button=button, .event=event};
    xQueueSend(s_buttons, &queued, 0);
}
