#include "bn_core.h"
#include "bn_keypad.h"

#include "bn_bg_palettes.h"
#include "bn_regular_bg_ptr.h"

#include "bn_regular_bg_items_bg_city.h"

#include "bn_sprite_ptr.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_item.h"

#include "bn_sprite_items_title_logo.h"

#include "bn_sound_items.h"
#include "bn_sound_actions.h"
#include "bn_music_items.h"
#include "bn_music_actions.h"

#include "bn_vector.h"

#include "fb_scene_title.h"
#include "fb_scene.h"

#include "bn_sprite_text_generator.h"
#include "common_variable_8x16_sprite_font.h"


namespace fb
{
	Title::Title(){}
	
	Scene Title::execute()
	{
		//Setup the background
		bn::regular_bg_ptr bg_city = bn::regular_bg_items::bg_city.create_bg(0, 0);
		bn::bg_palettes::set_transparent_color(bn::color(15, 25, 16));
		
		//Setup the title logo's sprites
		bn::sprite_ptr s1 = bn::sprite_items::title_logo.create_sprite(-32, -32);
		s1.set_tiles(bn::sprite_items::title_logo.tiles_item().create_tiles(0));
		
		bn::sprite_ptr s2 = bn::sprite_items::title_logo.create_sprite(0, -32);
		s2.set_tiles(bn::sprite_items::title_logo.tiles_item().create_tiles(1));
		
		bn::sprite_ptr s3 = bn::sprite_items::title_logo.create_sprite(32, -32);
		s3.set_tiles(bn::sprite_items::title_logo.tiles_item().create_tiles(2));
		
		//Setup the text
		bn::sprite_text_generator text_generator(common::variable_8x16_sprite_font);
		text_generator.set_center_alignment();
		bn::vector<bn::sprite_ptr, 32> text_sprites;
		text_generator.generate(0, 0, "PRESS START TO BEGIN", text_sprites);
		
		while(true)
		{
			bn::core::update();
			
			if (bn::keypad::pressed(bn::keypad::key_type::START))
			{
				bn::sound_items::swooshing.play(1);
				return Scene::Ingame;
			}
		}			
		
	}
}