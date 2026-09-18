#include "spoken_core.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    spoken_state_t s;
    spoken_core_init(&s);
    assert(s.index == 0 && s.mode == SPOKEN_STOPPED);
    spoken_core_move(&s, -1, 17);
    assert(s.index == 16);
    spoken_core_move(&s, 1, 17);
    assert(s.index == 0);
    uint32_t old = spoken_core_start(&s, SPOKEN_SHORT);
    assert(!spoken_core_finish(&s, 5000, 4000));
    assert(spoken_core_audio_done(&s, old));
    assert(!spoken_core_finish(&s, 1200, 4000));
    assert(spoken_core_finish(&s, 4000, 4000));
    assert(!spoken_core_finish(&s, 4001, 4000));
    spoken_core_start(&s, SPOKEN_SHORT);
    uint32_t lesson = spoken_core_start(&s, SPOKEN_LESSON);
    assert(!spoken_core_audio_done(&s, old));
    assert(s.mode == SPOKEN_LESSON && !s.audio_done);
    assert(spoken_core_audio_done(&s, lesson));
    assert(spoken_core_finish(&s, 1000, 4000));
    old = spoken_core_start(&s, SPOKEN_LESSON);
    spoken_core_move(&s, 1, 17);
    assert(s.index == 1 && s.mode == SPOKEN_STOPPED);
    assert(!spoken_core_audio_done(&s, old));
    spoken_core_move(&s, -1, 0);
    assert(s.index == 0);
    uint32_t stages[] = {0, 2500, 7500, 18000};
    assert(spoken_core_stage(0, stages) == 0);
    assert(spoken_core_stage(2499, stages) == 0);
    assert(spoken_core_stage(2500, stages) == 1);
    assert(spoken_core_stage(7500, stages) == 2);
    assert(spoken_core_stage(90000, stages) == 3);
    assert(spoken_core_frame(0, 32, 125) == 0);
    assert(spoken_core_frame(125, 32, 125) == 1);
    assert(spoken_core_frame(90000, 32, 125) == 31);
    assert(spoken_core_frame(999, 0, 125) == 0);
    assert(spoken_core_frame(999, 4, 1000) == 0);
    assert(spoken_core_frame(1000, 4, 1000) == 1);
    assert(spoken_core_frame(3999, 4, 1000) == 3);
    assert(spoken_core_frame(9000, 4, 1000) == 3);
    puts("Spoken interaction and audio cancellation tests: PASS");
    return 0;
}
