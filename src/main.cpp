/***************************************************************************
    Cannonball Main Entry Point.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

// Error reporting
#include <iostream>

// SDL Library
#include <SDL.h>

// SDL Specific Code
#if defined SDL2
#include "sdl2/timer.hpp"
#include "sdl2/input.hpp"
#else
#include "sdl/timer.hpp"
#include "sdl/input.hpp"
#endif

#include "video.hpp"

#include "romloader.hpp"
#include "trackloader.hpp"
#include "stdint.hpp"
#include "main.hpp"
#include "setup.hpp"
#include "engine/outrun.hpp"
#include "frontend/config.hpp"
#include "frontend/menu.hpp"

#include "engine/oinputs.hpp"
#include "engine/ooutputs.hpp"
#include "engine/omusic.hpp"

// Initialize Shared Variables
using namespace cannonball;

char FILENAME_CONFIG[256];
char FILENAME_SCORES[256];
char FILENAME_TTRIAL[256];
char FILENAME_CONT[256];

int    cannonball::state       = STATE_BOOT;
double cannonball::frame_ms    = 0;
int    cannonball::frame       = 0;
bool   cannonball::tick_frame  = true;
int    cannonball::fps_counter = 0;

char configload[128], ttrialload[128], contload[128], scoresload[128];
Menu* menu;


static void quit_func(int code)
{
    input.close();
    delete menu;
    SDL_Quit();
}

static void process_events(void)
{
    SDL_Event event;

    // Grab all events from the queue.
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_KEYDOWN:
                // Handle key presses.
                /*if (event.key.keysym.sym == SDLK_ESCAPE)
                    state = STATE_QUIT;
                else*/
                    input.handle_key_down(&event.key.keysym);
                break;

            case SDL_KEYUP:
                input.handle_key_up(&event.key.keysym);
                break;

            case SDL_QUIT:
                // Handle quit requests (like Ctrl-c).
                state = STATE_QUIT;
                break;
        }
    }
}

// Pause Engine
bool pause_engine;

static void tick()
{
    frame++;

    // Non standard FPS.
    // Determine whether to tick the current frame.
    /*if (config.fps != 30)
    {
        if (config.fps == 60)
            tick_frame = frame & 1;
        else if (config.fps == 120)
            tick_frame = (frame & 3) == 1;
    }*/

    process_events();

	oinputs.tick(NULL); // Do Controls
    oinputs.do_gear();        // Digital Gear

    switch (state)
    {
        case STATE_GAME:
        {
            if (input.has_pressed(Input::TIMER))
                outrun.freeze_timer = !outrun.freeze_timer;

            if (input.has_pressed(Input::PAUSE))
                pause_engine = !pause_engine;

            if (input.has_pressed(Input::MENU))
                state = STATE_QUIT;

            if (!pause_engine || input.has_pressed(Input::STEP))
            {
                outrun.tick(NULL, tick_frame);
                input.frame_done(); // Denote keys read
            }
            else
            {                
                input.frame_done(); // Denote keys read
            }
        }
        break;

        case STATE_INIT_GAME:
            if (config.engine.jap && !roms.load_japanese_roms())
            {
                state = STATE_QUIT;
            }
            else
            {
                pause_engine = false;
                outrun.init();
                state = STATE_GAME;
            }
            break;

        case STATE_INIT_MENU:
            oinputs.init();
            outrun.outputs->init();
            menu->init();
            state = STATE_MENU;
            break;
    }

    // Draw SDL Video
    video.draw_frame();  
}

static void main_loop()
{
    while (state != STATE_QUIT)
    {
        tick();
    }
	quit_func(0);
}

int main(int argc, char* argv[])
{
	#ifdef _TINSPIRE
	enable_relative_paths(argv);
	#endif
	
    // Initialize timer and video systems
    if( SDL_Init(SDL_INIT_VIDEO) == -1 ) 
    { 
        std::cerr << "SDL Initialization Failed: " << SDL_GetError() << std::endl;
        return 1; 
    }
    
	char homedir[128];
    
    snprintf(homedir, sizeof(homedir), "./");
    mkdir("./roms", 0755);
	snprintf(FILENAME_CONFIG, sizeof(FILENAME_CONFIG), "./%s", "config.xml.tns");
	snprintf(FILENAME_SCORES, sizeof(FILENAME_SCORES), "%./%s","hiscores.tns");
	snprintf(FILENAME_TTRIAL, sizeof(FILENAME_TTRIAL), "./%s","hiscores_timetrial.tns");
	snprintf(FILENAME_CONT, sizeof(FILENAME_CONT), "./%s", "hiscores_continuous.tns");
	
    menu = new Menu(NULL);

    bool loaded = false;

    // Load LayOut File
	loaded = roms.load_revb_roms();

    if (loaded)
    {
        // Load XML Config
        config.load(FILENAME_CONFIG);

        // Initialize SDL Video
        if (!video.init(&roms, &config.video))
            quit_func(1);

        state = config.menu.enabled ? STATE_INIT_MENU : STATE_INIT_GAME;

        // Initalize controls
        input.init(config.controls.pad_id,
                   config.controls.keyconfig, config.controls.padconfig, 
                   config.controls.analog,    config.controls.axis, config.controls.asettings);


        // Populate menus
        menu->populate();
        main_loop();  // Loop until we quit the app
    }
    else
    {
        quit_func(1);
    }

    // Never Reached
    return 0;
}
