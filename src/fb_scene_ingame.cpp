#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_fixed.h"
#include "bn_log.h"

#include "bn_bg_palettes.h"
#include "bn_regular_bg_ptr.h"

#include "bn_regular_bg_items_bg_city.h"

#include "bn_sprite_ptr.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_item.h"

#include "bn_sprite_items_bird.h"
#include "bn_sprite_items_pipe_top.h"
#include "bn_sprite_items_pipe_middle.h"
#include "bn_sprite_items_pipe_bottom.h"
#include "bn_sprite_items_game_over.h"
#include "bn_sprite_items_info_box.h"

#include "bn_vector.h"
#include "bn_rect.h"
#include "bn_seed_random.h"
#include "bn_time.h"

#include "bn_sound_items.h"
#include "bn_sound_actions.h"
#include "bn_music_items.h"
#include "bn_music_actions.h"

#include "fb_constants.h"
#include "fb_scene.h"
#include "fb_scene_ingame.h"
#include "fb_common.h"

#include "bn_sprite_text_generator.h"
#include "common_variable_8x16_sprite_font.h"
#include "bn_string.h"
#include "bn_sram.h"

namespace fb
{
	Ingame::Ingame(){}
	
	void make_pipe(bn::vector<bn::sprite_ptr, 32>& sprites, int direction, int y, int height, bool draw_lip=true)
    {
        for(int i=0; i<height; i++)
        {
            if(i==0)
            {
				if(direction==-1&&draw_lip)
				{
					//Top pipe
					bn::sprite_builder builder(bn::sprite_items::pipe_top);
					builder.set_position(fb::pipe_screen_edge_x, y+(32*i));
					builder.set_z_order(5);
					sprites.push_back(builder.release_build());
				}
				else
				{
					bn::sprite_builder builder(bn::sprite_items::pipe_middle);
					builder.set_position(fb::pipe_screen_edge_x, y+(32*i));
					builder.set_z_order(5);
					sprites.push_back(builder.release_build());
				}
            }
            else if(i==height-1)
            {
				if(direction==1&&draw_lip)
				{
					//Bottom pipe
					bn::sprite_builder builder(bn::sprite_items::pipe_bottom);
					builder.set_position(fb::pipe_screen_edge_x, y+(32*i));
					builder.set_z_order(5);
					sprites.push_back(builder.release_build());
				}
                else
				{
					bn::sprite_builder builder(bn::sprite_items::pipe_middle);
					builder.set_position(fb::pipe_screen_edge_x, y+(32*i));
					builder.set_z_order(5);
					sprites.push_back(builder.release_build());
				}
            }
            else
            {
                //Middle pipe
                bn::sprite_builder builder(bn::sprite_items::pipe_middle);
                builder.set_position(fb::pipe_screen_edge_x, y+(32*i));
                builder.set_z_order(5);
				sprites.push_back(builder.release_build());
            }
        }
    }
	
	void make_pipe_pair(bn::vector<bn::sprite_ptr, 32>& sprites, int y, int height, int height2, int gap)
	{
		make_pipe(sprites, 1, y, height);
		int y2 = y+(32*height)+gap;
		make_pipe(sprites, -1, y2, height2);
	}
	
	bn::vector<bn::sprite_ptr, 4> make_metasprite(bn::sprite_item item, int num, int x, int y, bool visible=false, int priority=0)
	{
		bn::vector<bn::sprite_ptr, 4> sprites;
		for(int i=0; i<num; i++)
		{
			bn::sprite_ptr s = item.create_sprite(x+(i*32), y);
			s.set_tiles(item.tiles_item().create_tiles(i));
			s.set_visible(visible);
			s.set_z_order(priority);
			sprites.push_back(s);
		}
		return sprites;
	}
	
	bn::fixed lerp(bn::fixed a, bn::fixed b, bn::fixed percent) {return a + percent * (b - a);}
	
	Scene Ingame::execute()
	{
		int frames = 0;
		
		bool lose = false;
		bool lose_sprites_enabled = false;
		int lose_delay = -1;
		
		int pipes_passed = 0;
		int between_pipes = 0;
		int score_delay = -1;
		
		bn::sram::read_offset(fb::best, 0);
		
		bn::seed_random random;
		
		//Setup the background
		bn::regular_bg_ptr bg_city = bn::regular_bg_items::bg_city.create_bg(0, 0);
		bn::bg_palettes::set_transparent_color(bn::color(15, 25, 16));

		//Setup the bird sprite
		bn::sprite_ptr spr_bird = bn::sprite_items::bird.create_sprite(0, 0);
		bn::sprite_animate_action<4> action = bn::create_sprite_animate_action_forever(spr_bird, 4, bn::sprite_items::bird.tiles_item(), 0, 1, 2, 3); //Bird's animation
		bn::rect bird_rect = bn::rect((int)spr_bird.x()+6, (int)spr_bird.y()+6, 8, 8); //Bird's collision rectangle
		spr_bird.set_z_order(-5);
		
		bn::fixed bird_vY = 0; //Bird's Y velocity
		
		//Setup the pipes
		bn::vector<bn::sprite_ptr, 32> pipe_sprites;
		
		//Setup the text
		bn::sprite_text_generator text_generator(common::variable_8x16_sprite_font);
		text_generator.set_center_alignment();
		bn::vector<bn::sprite_ptr, 32> text_sprites;
		bn::vector<bn::sprite_ptr, 32> best_text_sprites;
		
		//Setup the game over sprites
		bn::vector<bn::sprite_ptr, 4> game_over_sprites = make_metasprite(bn::sprite_items::game_over, 3, -32, -90, false, -10);
		
		//Setup the info box
		bn::vector<bn::sprite_ptr, 4> info_box_sprites = make_metasprite(bn::sprite_items::info_box, 4, -48, 128, false, -10);
		
		//Play music
		bn::music_items::gnon.play(0.25);
		
		while(true)
		{
			bn::core::update();
			
			//Apply the y velocity to the bird
			spr_bird.set_y(spr_bird.y() + bird_vY);
			
			//Decrement timers
			if(score_delay>-1) score_delay--;
			if(lose_delay>-1) lose_delay--;	
			
			if(!lose)
			{
				//Scroll the background
				bg_city.set_x(bg_city.x() - fb::scroll_speed/2);

				//Bird stuff
				bird_vY += fb::bird_gravity;

				if (bn::keypad::pressed(bn::keypad::key_type::A))
				{
					bird_vY = fb::bird_jump_speed;
					bn::sound_items::wing.play(1);
				}
				
				//Player loses if bird hits the ground
				if(spr_bird.y()>108)
				{
					lose = true;
					lose_delay = fb::game_lose_delay;
					bn::music::stop();
					bird_vY = 0;
				}
				
				//Update the bird's collision rectangle's position
				bird_rect.set_x((int)spr_bird.x());
				bird_rect.set_y((int)spr_bird.y());
				
				//Update the bird's animation
				action.update();

				//Pipe stuff: iterate through the pipe vector
				for(auto it = pipe_sprites.begin(), end = pipe_sprites.end(); it != end; )
				{
					bn::sprite_ptr current = *it;
					
					current.set_x(current.x() - fb::scroll_speed);
					
					//Collision w/ player
					bn::rect pipe_rect = bn::rect((int)current.x(), (int)current.y(), 32, 32);
					if(pipe_rect.intersects(bird_rect) && pipe_rect.y()>-fb::pipe_screen_edge_y)
					{
						//Player loses
						lose = true;
						bn::music::stop();
						lose_delay = fb::game_lose_delay;
						bn::sound_items::hit.play(1);
						bird_vY = 0;
					}
					
					//Is the player in between this pipe?
					if(spr_bird.x()+8>current.x() && spr_bird.x()+8<current.x()+32)
					{
						between_pipes++;
					}
					
					//Delete pipes after going offscreen (left side)
					if(current.x()<-fb::pipe_screen_edge_x)
					{
						it = pipe_sprites.erase(it);
						end = pipe_sprites.end();
					}
					else it++;
				}
				
				//Update the score
				if(between_pipes>0 && score_delay==-1)
				{	
					between_pipes = 0;
					pipes_passed++;
					score_delay = 120;
					bn::sound_items::point.play(1);
				}
				
				//Make pipes continously
				if(frames % 120 == 0)
				{
					int type =  random.get_unbiased_int(5);
					int nudge = 0;
					switch(type)
					{
						case 0: default:
							nudge = (random.get_int(64)-32);
							make_pipe_pair(pipe_sprites, -fb::pipe_screen_edge_y+nudge, 2, 2+(nudge/32), 42);
							if(nudge>0) make_pipe(pipe_sprites, 1, (-fb::pipe_screen_edge_y+nudge)-32, 1);
							if(nudge<0) make_pipe(pipe_sprites, 1, (fb::pipe_screen_edge_y-nudge)-28, 2, false);
							break;
						case 1:
							make_pipe_pair(pipe_sprites, -fb::pipe_screen_edge_y-32, 1, 4, 42);
							break;
						case 2:
							make_pipe_pair(pipe_sprites, -fb::pipe_screen_edge_y, 3, 1, 42);
							break;
					}
					random.update();
				}
				
				//Draw the number of pipes passed
				text_sprites.clear();
				bn::string<4> text;
				text = bn::to_string<4>(pipes_passed); 
				text_generator.generate(0, -70, text, text_sprites);
				for(bn::sprite_ptr& sprite : text_sprites)
				{
					sprite.set_z_order(-20);
				}
			}
			else
			{
				if(pipes_passed > best) best = pipes_passed;
				
				if(!lose_sprites_enabled)
				{
					for(bn::sprite_ptr& sprite : game_over_sprites)
					{
						sprite.set_visible(true);
					}
					for(bn::sprite_ptr& sprite : info_box_sprites)
					{
						sprite.set_visible(true);
					}
					text_sprites.clear();
					bn::string<4> text;
					bn::string<4> text2;
					text = bn::to_string<4>(pipes_passed); 
					text_generator.generate(38, 100, text, text_sprites);
					text2 = bn::to_string<4>(fb::best); 
					text_generator.generate(38, 100, text2, best_text_sprites);
					
					for(bn::sprite_ptr& sprite : text_sprites)
					{
						sprite.set_z_order(-20);
					}
					for(bn::sprite_ptr& sprite : best_text_sprites)
					{
						sprite.set_z_order(-20);
					}
					lose_sprites_enabled = true;
					
					bn::sram::write(fb::best);
				}
				
				//Slide the game over and info box sprites onto the screen
				for(bn::sprite_ptr& sprite : game_over_sprites)
				{
					if(sprite.y()>-44) break;
					sprite.set_y(lerp(sprite.y(), -44, 0.125));
				}
				if(lose_delay==-1)
				{
					for(bn::sprite_ptr& sprite : info_box_sprites)
					{
						if(sprite.y()<0) break;
						sprite.set_y(lerp(sprite.y(), 0, 0.125));
					}
					
					if (bn::keypad::pressed(bn::keypad::key_type::START))
					{
						return Scene::Title;
					}
					
					for(bn::sprite_ptr& sprite : text_sprites)
					{
						sprite.set_y(lerp(sprite.y(), -9, 0.125));
					}
					
					for(bn::sprite_ptr& sprite : best_text_sprites)
					{
						sprite.set_y(lerp(sprite.y(), 12, 0.125));
					}
					
					if(bird_vY==0)
					{
						bn::sound_items::die.play(1);
					}
					bird_vY += bird_gravity;
				}
				
			}
			frames++;	
		}
	}
}