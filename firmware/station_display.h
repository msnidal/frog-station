// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <stdbool.h>
#include <stdint.h>
// Modifier bits: Command, Shift, Control, Option, in that order.
void fs_render_left(uint8_t out[512], uint8_t layer, uint8_t modifiers, bool lamp_on, bool alert);

void fs_overlay_right(uint8_t out[512], bool typing, uint8_t meter, bool music, bool lamp_off);
