// translate input events into SDL events for hatari

#include "../libretro/libretro.h"
#include "core.h"
#include <SDL2/SDL.h>

extern retro_environment_t environ_cb;
extern retro_input_poll_t input_poll_cb;
extern retro_input_state_t input_state_cb;
extern retro_log_printf_t retro_log;

//
// translated SDL event queue
//

#define EVENT_QUEUE_SIZE 64
#define EVENT_QUEUE_MAX_OVERFLOW_ERRORS 50
static int event_queue_pos = 0;
static int event_queue_len = 0;
static SDL_Event event_queue[EVENT_QUEUE_SIZE];
static int event_queue_overflows = 0;

static void event_queue_init()
{
	event_queue_pos = 0;
	event_queue_len = 0;
	event_queue_overflows = 0;
}

static void event_queue_add(const SDL_Event* event)
{
	int next = (event_queue_pos + event_queue_len) % EVENT_QUEUE_SIZE;
	if (event_queue_len >= EVENT_QUEUE_SIZE)
	{
		if (event_queue_overflows < EVENT_QUEUE_MAX_OVERFLOW_ERRORS)
		{
			++event_queue_overflows;
			retro_log(RETRO_LOG_ERROR,"core_input event queue overflow, %d input events lost.\n",event_queue_overflows);
			if (event_queue_overflows == EVENT_QUEUE_MAX_OVERFLOW_ERRORS)
				retro_log(RETRO_LOG_ERROR,"Further overflows will now be ignored.\n");
		}
		return;
	}
	event_queue[next] = *event;
	++event_queue_len;
	//retro_log(RETRO_LOG_DEBUG,"event_queue_add: %d (%d)\n",next,event->type);
}

//
// Key translation
//

#define HAVE_SDL2
#include "../libretro/libretro_sdl_keymap.h"

static unsigned retrok_to_sdl[RETROK_LAST] = {0};

void retrok_to_sdl_init()
{
	for (int i=0; i<RETROK_LAST; ++i)
		retrok_to_sdl[i] = SDLK_UNKNOWN;
	for (int i=0; true; ++i)
	{
		struct rarch_key_map m = rarch_key_map_sdl[i];
		if (m.rk == RETROK_UNKNOWN) break;
		retrok_to_sdl[m.rk] = m.sym;
	}
}

//
// libretro input
//

static bool retrok_down[RETROK_LAST] = {0}; // for repeat tracking
static int vmouse_x, vmouse_y; // virtual mouse state
static bool vmouse_l, vmouse_r;

void core_input_keyboard_event(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers)
{
	// assuming we don't need to mutex the event queue because the retro_keyboard_callback will only be called
	// out of its own internal queue in between retro_run frames.
	SDL_Event event; memset(&event,0,sizeof(event));
	event.key.type = down ? SDL_KEYDOWN : SDL_KEYUP;
	event.key.state = down ? SDL_PRESSED : SDL_RELEASED;
	event.key.repeat = ((down && retrok_down[keycode]) || (!down && !retrok_down[keycode]));
	event.key.keysym.sym = retrok_to_sdl[keycode];
	event.key.keysym.scancode = SDL_GetScancodeFromKey(event.key.keysym.sym);
	event.key.keysym.mod = 0;
	if (key_modifiers & RETROKMOD_NUMLOCK  ) event.key.keysym.mod |= KMOD_NUM;
	if (key_modifiers & RETROKMOD_CAPSLOCK ) event.key.keysym.mod |= KMOD_CAPS;
	if (key_modifiers & RETROKMOD_SCROLLOCK) event.key.keysym.mod |= KMOD_SCROLL;
	if (retrok_down[RETROK_LSHIFT]) event.key.keysym.mod |= KMOD_LSHIFT;
	if (retrok_down[RETROK_RSHIFT]) event.key.keysym.mod |= KMOD_RSHIFT;
	if (retrok_down[RETROK_LCTRL ]) event.key.keysym.mod |= KMOD_LCTRL;
	if (retrok_down[RETROK_RCTRL ]) event.key.keysym.mod |= KMOD_RCTRL;
	if (retrok_down[RETROK_LALT  ]) event.key.keysym.mod |= KMOD_LALT;
	if (retrok_down[RETROK_RALT  ]) event.key.keysym.mod |= KMOD_RALT;
	if (retrok_down[RETROK_LSUPER]) event.key.keysym.mod |= KMOD_LGUI;
	if (retrok_down[RETROK_RSUPER]) event.key.keysym.mod |= KMOD_RGUI;
	if (retrok_down[RETROK_MODE  ]) event.key.keysym.mod |= KMOD_MODE;
	retrok_down[keycode] = down;
	event_queue_add(&event);
}

void core_input_init()
{
	static struct retro_keyboard_callback keyboard_callback = { core_input_keyboard_event };

	// generate mappings
	retrok_to_sdl_init();

	// clear state
	event_queue_init();
	memset(&retrok_down,0,sizeof(retrok_down));
	vmouse_x = vmouse_y = 0;
	vmouse_l = vmouse_r = false;

	// keyboard events require focus
	environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &keyboard_callback);
}

const bool vmouse_enabled = true; // TODO option
const bool pmouse_enabled = true; // TODO option

void core_input_update()
{
	// virtual mouse state
	bool vm_l = false;
	bool vm_r = false;
	int vm_x = vmouse_x;
	int vm_y = vmouse_y;

	input_poll_cb();

	// TODO process gamepads
	// depending on configuration, may update mouse vm
	// may also update joystick

	if (vmouse_enabled)
	{
		if (pmouse_enabled) // physical mouse gives relative x/y
		{
			bool pm_l = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
			bool pm_r = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
			int pm_x  = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
			int pm_y  = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
			vm_l |= pm_l;
			vm_r |= pm_r;
			vm_x += pm_x;
			vm_y += pm_y;
		}

		if ((vm_l && !vmouse_l) || (!vm_l && vmouse_l))
		{
			SDL_Event event; memset(&event,0,sizeof(event));
			event.button.type = vm_l ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
			event.button.button = SDL_BUTTON_LEFT;
			event.button.state = vm_l ? SDL_PRESSED : SDL_RELEASED;
			event.button.clicks = 1;
			event.button.x = vm_x;
			event.button.y = vm_y;
			event_queue_add(&event);
			vmouse_l = vm_l;
		}

		if ((vm_r && !vmouse_r) || (!vm_r && vmouse_r))
		{
			SDL_Event event; memset(&event,0,sizeof(event));
			event.button.type = vm_r ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
			event.button.button = SDL_BUTTON_RIGHT;
			event.button.state = vm_r ? SDL_PRESSED : SDL_RELEASED;
			event.button.clicks = 1;
			event.button.x = vm_x;
			event.button.y = vm_y;
			event_queue_add(&event);
			vmouse_r = vm_r;
		}

		if (vm_x != vmouse_x || vm_y != vmouse_y)
		{
			SDL_Event event; memset(&event,0,sizeof(event));
			event.motion.type = SDL_MOUSEMOTION;
			event.motion.state = 0;
			if (vmouse_l) event.motion.state |= SDL_BUTTON_LMASK;
			if (vmouse_r) event.motion.state |= SDL_BUTTON_RMASK;
			event.motion.x = vm_x;
			event.motion.y = vm_y;
			event.motion.xrel = vm_x - vmouse_x;
			event.motion.yrel = vm_y - vmouse_y;
			event_queue_add(&event);
			vmouse_x = vm_x;
			vmouse_y = vm_y;
		}
	}
}

int core_poll_event(SDL_Event* event)
{
	if (event_queue_len == 0) return 0;
	if (event)
	{
		*event = event_queue[event_queue_pos];
		//retro_log(RETRO_LOG_DEBUG,"core_poll_event: %d (%d)\n",event_queue_pos,event->type);
		event_queue_pos = (event_queue_pos + 1) % EVENT_QUEUE_SIZE;
		--event_queue_len;
	}
	return 1;
}
