#pragma once

#include "bsp_button.h"
#include <stdbool.h>

void spoken_app_start(bool audio_ready);
void spoken_app_key_cb(bsp_btn_t button, bsp_btn_ev_t event, void *user);
