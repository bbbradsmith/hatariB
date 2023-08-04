// translate input events into SDL events for hatari

#include "../libretro/libretro.h"
#include "core.h"
#include <SDL2/SDL.h>

extern retro_environment_t environ_cb;
extern retro_input_poll_t input_poll_cb;
extern retro_input_state_t input_state_cb;
extern retro_log_printf_t retro_log;

//
// From Hatari
//

extern void Main_EventHandler(void);

//
// translated SDL event queue
//

#define EVENT_QUEUE_SIZE 64
#define EVENT_QUEUE_MAX_OVERFLOW_ERRORS 50
static int event_queue_pos = 0;
static int event_queue_len = 0;
static SDL_Event event_queue[EVENT_QUEUE_SIZE];
static int event_queue_overflows = 0;

static void event_queue_init(void)
{
	event_queue_pos = 0;
	event_queue_len = 0;
	event_queue_overflows = 0;
}

static void event_queue_push(const SDL_Event* event)
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
	//retro_log(RETRO_LOG_DEBUG,"event_queue_push: %d (%d)\n",next,event->type);
}

static int event_queue_pop(SDL_Event* event)
{
	if (event_queue_len == 0) return 0;
	if (event)
	{
		*event = event_queue[event_queue_pos];
		//retro_log(RETRO_LOG_DEBUG,"event_queue_pop: %d (%d)\n",event_queue_pos,event->type);
		event_queue_pos = (event_queue_pos + 1) % EVENT_QUEUE_SIZE;
		--event_queue_len;
	}
	return 1;
}

static void event_queue_force_feed(void)
{
	// Hatari polls the keyboard via INTERRUPT_IKBD_AUTOSEND,
	// which it tries to call once per vblank.
	// We want to force it to process immediately at the start of each frame.
	while (event_queue_len) Main_EventHandler();
}

static void event_queue_finish(void)
{
	// this shouldn't happen because of the force feed
	if (event_queue_len != 0)
		retro_log(RETRO_LOG_WARN,"core event_queue not empty at end of retro_run? %d",event_queue_len);
}

//
// Key translation
//

#define HAVE_SDL2
#include "../libretro/libretro_sdl_keymap.h"

static unsigned retrok_to_sdl[RETROK_LAST] = {0};

void retrok_to_sdl_init(void)
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

#define JOY_PORTS   6
// 1-button joysticks
#define JOY_STICK_U    0x01
#define JOY_STICK_D    0x02
#define JOY_STICK_L    0x04
#define JOY_STICK_R    0x08
#define JOY_STICK_F    0x80
// STE Atari Jaguar pads have 5 buttons implemented in Hatari
#define JOY_FIRE_A     0x01
#define JOY_FIRE_B     0x02
#define JOY_FIRE_C     0x04
#define JOY_FIRE_OPT   0x08
#define JOY_FIRE_PAUSE 0x10

static uint8_t retrok_down[RETROK_LAST] = {0}; // for repeat tracking
static int32_t vmouse_x, vmouse_y; // virtual mouse state
static uint8_t vmouse_l, vmouse_r;
static int32_t joy_fire[JOY_PORTS];
static int32_t joy_stick[JOY_PORTS];

void core_input_keyboard_event(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers)
{
	//retro_log(RETRO_LOG_DEBUG,"core_input_keyboard_event(%d,%d,%d,%04X)\n",down,keycode,character,key_modifiers);
	// assuming we don't need to mutex the event queue,
	// because it doesn't make sense for retro_keyboard_callback to operate during retro_run,
	// (run-ahead, netplay, etc. would need to depend on this?)
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
	event_queue_push(&event);
	// TODO configure to block this stuff and only allow virtual keyboard inputs
}

void core_input_serialize(void)
{
	// note: doesn't include event queue, and shouldn't need to.
	for (int i=0; i<RETROK_LAST; ++i) core_serialize_uint8(&retrok_down[i]);
	core_serialize_int32(&vmouse_x);
	core_serialize_int32(&vmouse_y);
	core_serialize_uint8(&vmouse_l);
	core_serialize_uint8(&vmouse_r);
}

void core_input_init(void)
{
	static struct retro_keyboard_callback keyboard_callback = { core_input_keyboard_event };

	// generate mappings
	retrok_to_sdl_init();

	// clear state
	event_queue_init();
	memset(&retrok_down,0,sizeof(retrok_down));
	vmouse_x = vmouse_y = 0;
	vmouse_l = vmouse_r = false;
	for (int i=0; i<JOY_PORTS; ++i)
	{
		joy_fire[i] = 0;
		joy_stick[i] = 0;
	}

	// keyboard events require focus
	environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &keyboard_callback);
}

const bool vmouse_enabled = true; // TODO option
const bool pmouse_enabled = true; // TODO option
const int joy_port_map[4] = {1,0,2,3}; // TODO port mapping options (0,1=ST,2-3=STE,4-5=parallel)

void core_input_update(void)
{
	// virtual mouse state
	bool vm_l = false;
	bool vm_r = false;
	int vm_x = vmouse_x;
	int vm_y = vmouse_y;

	input_poll_cb();

	// clear all joystick ports
	for (int i=0; i<JOY_PORTS; ++i)
	{
		joy_fire[i] = 0;
		joy_stick[i] = 0;
	}
	// have each joystick update its mapped controls
	for (int i=0; i<4; ++i)
	{
		int j = joy_port_map[i];
		// TODO mapped inputs, these are just hardcoded for now (d-pad + B fire)
		if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT )) joy_stick[j] |= JOY_STICK_L;
		if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT)) joy_stick[j] |= JOY_STICK_R;
		if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP   )) joy_stick[j] |= JOY_STICK_U;
		if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN )) joy_stick[j] |= JOY_STICK_D;
		if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B))
		{
			joy_stick[j] |= JOY_STICK_F;
			joy_fire[j] |= JOY_FIRE_A;
		}
		// mouse can be mapped to all gamepads (Y/X click, left stick move)
		if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y)) vm_l = true;
		if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X)) vm_r = true;
		{
			int ax = input_state_cb(i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
			int ay = input_state_cb(i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
			// TODO configurable deadzone, circular, reduction of magnitude in circular way
			// for now just using a dead box
			const int DEADZONE = 0x8000 / 12;
			const float MOUSE_SPEED = 0.001;
			if (ax < DEADZONE && ax > -DEADZONE) ax = 0;
			else if (ax < 0) ax += DEADZONE;
			else if (ax > 0) ax -= DEADZONE;
			if (ay < DEADZONE && ay > -DEADZONE) ay = 0;
			else if (ay < 0) ay += DEADZONE;
			else if (ay > 0) ay -= DEADZONE;
			vm_x += (int)(ax * MOUSE_SPEED);
			vm_y += (int)(ay * MOUSE_SPEED);
		}
	}

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
			event_queue_push(&event);
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
			event_queue_push(&event);
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
			event_queue_push(&event);
			vmouse_x = vm_x;
			vmouse_y = vm_y;
		}
	}
}

void core_input_post(void)
{
	event_queue_force_feed();
}

void core_input_finish(void)
{
	event_queue_finish();
}

int core_poll_event(SDL_Event* event)
{
	return event_queue_pop(event);
}

int core_poll_joy_fire(int port)
{
	if (port >= JOY_PORTS) return 0;
	return joy_fire[port];
}

int core_poll_joy_stick(int port)
{
	if (port >= JOY_PORTS) return 0;
	return joy_stick[port];
}
