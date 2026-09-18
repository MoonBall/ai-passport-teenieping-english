#pragma once

#include "lvgl.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SPOKEN_CARD_COUNT 17
#define SPOKEN_SAMPLE_RATE 16000

typedef struct {
    uint32_t offset;
    uint32_t bytes;
    uint32_t samples;
    uint32_t duration_ms;
} spoken_audio_t;

typedef struct {
    const char *id;
    const char *title;
    const char *scene;
    const char *command;
    const char *meaning;
    const char *response;
    const char *usage;
    spoken_audio_t short_audio;
    spoken_audio_t lesson_audio;
    uint32_t stages_ms[4];
    const lv_image_dsc_t *frames;
    uint16_t frame_count;
    uint16_t frame_ms;
} spoken_card_t;

extern const spoken_card_t SPOKEN_CARDS[SPOKEN_CARD_COUNT];
extern const uint32_t SPOKEN_AUDIO_BYTES;
extern const uint32_t SPOKEN_AUDIO_CRC;
LV_FONT_DECLARE(ui_font_spoken_16);
LV_FONT_DECLARE(ui_font_spoken_14);

bool spoken_audio_init(void);
bool spoken_audio_read(size_t offset, void *buffer, size_t length);
