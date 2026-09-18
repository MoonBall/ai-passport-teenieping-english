#include "spoken_core.h"

void spoken_core_init(spoken_state_t *state)
{
    *state = (spoken_state_t){0};
}

uint32_t spoken_core_start(spoken_state_t *state, spoken_mode_t mode)
{
    state->mode = mode;
    state->audio_done = mode == SPOKEN_STOPPED;
    return ++state->generation;
}

void spoken_core_move(spoken_state_t *state, int direction, size_t count)
{
    spoken_core_start(state, SPOKEN_STOPPED);
    if (!count) {
        state->index = 0;
    } else if (direction < 0) {
        state->index = state->index ? state->index - 1 : count - 1;
    } else if (direction > 0) {
        state->index = (state->index + 1) % count;
    }
}

bool spoken_core_audio_done(spoken_state_t *state, uint32_t generation)
{
    if (generation != state->generation || state->mode == SPOKEN_STOPPED) return false;
    state->audio_done = true;
    return true;
}

bool spoken_core_finish(spoken_state_t *state, uint32_t elapsed_ms, uint32_t animation_ms)
{
    if (state->mode == SPOKEN_STOPPED || !state->audio_done) return false;
    if (state->mode == SPOKEN_SHORT && elapsed_ms < animation_ms) return false;
    state->mode = SPOKEN_STOPPED;
    return true;
}

size_t spoken_core_stage(uint32_t audio_ms, const uint32_t stages_ms[4])
{
    size_t stage = 0;
    while (stage < 3 && audio_ms >= stages_ms[stage + 1]) ++stage;
    return stage;
}

size_t spoken_core_frame(uint32_t elapsed_ms, size_t count, uint32_t frame_ms)
{
    if (count <= 1 || !frame_ms) return 0;
    size_t frame = elapsed_ms / frame_ms;
    return frame < count ? frame : count - 1;
}
