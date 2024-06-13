#include "bn_core.h"

#include "bn_seed_random.h"

#include "fb_scene.h"
#include "fb_scene_title.h"
#include "fb_scene_ingame.h"

int main()
{
    bn::core::init();
	
	fb::Scene scene = fb::Scene::Title;
	
    while(true)
    {
        bn::core::update();
		if(scene == fb::Scene::Title)
		{
			fb::Title title = fb::Title();
			scene = title.execute();
		}
		else if(scene == fb::Scene::Ingame)
		{
			fb::Ingame ingame = fb::Ingame();
			scene = ingame.execute();
		}
    }
}
