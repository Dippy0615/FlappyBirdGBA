#ifndef FB_CONSTANTS
#define FB_CONSTANTS

#include "bn_fixed.h"

namespace fb
{
    const bn::fixed scroll_speed = 0.75;
    const bn::fixed bird_gravity = 0.175;
    const bn::fixed bird_jump_speed = -2.65;
	const int pipe_screen_edge_x = 164;
	const int pipe_screen_edge_y = 64;
	const int game_lose_delay = 30;
}
#endif