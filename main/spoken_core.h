#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    SPOKEN_STOPPED,
    SPOKEN_SHORT,
    SPOKEN_LESSON,
} spoken_mode_t;

typedef struct {
    size_t index;
    spoken_mode_t mode;
    uint32_t generation;
    bool audio_done;
} spoken_state_t;

void spoken_core_init(spoken_state_t *state);
void spoken_core_move(spoken_state_t *state, int direction, size_t count);
uint32_t spoken_core_start(spoken_state_t *state, spoken_mode_t mode);
bool spoken_core_audio_done(spoken_state_t *state, uint32_t generation);
bool spoken_core_finish(spoken_state_t *state, uint32_t elapsed_ms, uint32_t animation_ms);
size_t spoken_core_stage(uint32_t audio_ms, const uint32_t stages_ms[4]);
size_t spoken_core_frame(uint32_t elapsed_ms, size_t count, uint32_t frame_ms);
