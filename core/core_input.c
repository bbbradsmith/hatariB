// translate input events into SDL events for hatari

#include "../libretro/libretro.h"
#include "core.h"
#include "core_internal.h"
#include <SDL.h>

//
// Internal input state
//

// mouse speed calibration
// fixed point mouse precision pre-scale (so that we can move less than 1 pixel per frame)
#define MOUSE_PRECISION   256
const float MOUSE_SPEED_FACTOR = 0.00005; // sets the overall speed meaning of the settings in core_config.c
const float DPAD_MOUSE_SPEED = 0.3; // scale of d-pad relative to analog max
// the speed factor of Mouse Speed Slow and Mouse Speed Fast button assignments
#define MOUSE_MODIFIER_FACTOR   3

// auxiliary input bitfield
#define AUX_MOUSE_L      0x00000001
#define AUX_MOUSE_R      0x00000002
#define AUX_PAUSE        0x00000004
#define AUX_OSK_ON       0x00000008
#define AUX_OSK_SHOT     0x00000010
#define AUX_DRIVE_SWAP   0x00000020
#define AUX_DISK_SWAP    0x00000040
#define AUX_WARM_BOOT    0x00000080
#define AUX_COLD_BOOT    0x00000100
#define AUX_CPU_SPEED    0x00000200
#define AUX_STATUSBAR    0x00000400
#define AUX_OSK_CLOSED   0x00000800
#define AUX_JM_TOGGLE    0x00001000

// in core_internal.h
//#define AUX_OSK_U        0x00010000
//#define AUX_OSK_D        0x00020000
//#define AUX_OSK_L        0x00040000
//#define AUX_OSK_R        0x00080000
//#define AUX_OSK_CONFIRM  0x00100000
//#define AUX_OSK_CANCEL   0x00200000
//#define AUX_OSK_POS      0x00400000
//#define AUX_OSK_ALL      0x00FF0000

#define AUX(mask_)   (aux_buttons & (AUX_##mask_))
#define AUX_SET(v_,mask_)   { \
	if(v_) aux_buttons |=  (AUX_##mask_); \
	else   aux_buttons &= ~(AUX_##mask_); }

// AUX(MOUSE_L) gets the state of MOUSE_L
// AUX_SET(b,MOUSE_L) sets MOUSE_L to the state of b

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

#define OSK_CONFIRM   0
#define OSK_CANCEL    1
#define OSK_POS       2
#define OSK_U         3
#define OSK_D         4
#define OSK_L         5
#define OSK_R         6
#define OSK_INPUT_COUNT   7

// input state that gets serialized
static int32_t vmouse_x, vmouse_y; // virtual mouse state
static int32_t mod_state;
static int32_t aux_buttons; // last state of auxiliary buttons
static uint8_t joy_autofire[4];
uint8_t retrok_down[RETROK_LAST] = {0}; // for repeat tracking
uint8_t osk_press_mod;
int32_t osk_press_key;
int32_t osk_press_time;
int32_t jm_toggle_index = -1;

// input state that is temporary
static int32_t joy_fire[JOY_PORTS];
static int32_t joy_stick[JOY_PORTS];
static uint8_t retrok_joy[RETROK_LAST]; // overlay to retrok_down

int core_joy_port_map[4] = {1,0,2,3};
int core_stick_map[4][3] = {STICK_DEF,STICK_DEF,STICK_DEF,STICK_DEF};
int core_button_map[4][12] = {BUTTON_DEF,BUTTON_DEF,BUTTON_DEF,BUTTON_DEF};
int core_oskey_map[4][CORE_INPUT_OSKEY_TOTAL] = {OSKEY_DEF,OSKEY_DEF,OSKEY_DEF};

// other input configuration
bool core_input_debug = false;
bool core_mouse_port = true;
bool core_host_keyboard = true;
bool core_host_mouse = true;
int core_autofire = 6;
int core_stick_threshold = 30; // percentage of stick to digital joystick direction threshold
int core_mouse_speed = 6; // 1-20 speed factor
int core_mouse_dead = 5; // percentage of stick deadzone

//
// From Hatari
//

extern void Main_EventHandler(void);
extern int Reset_Warm(void);
extern int Reset_Cold(void);

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
		if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP)
		{
			// update mod-state only when key events are fetched
			mod_state = event->key.keysym.mod;
		}
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

void core_input_osk_close(void)
{
	if (core_osk_mode != CORE_OSK_OFF)
	{
		// OSK_CLOSED used to reset OSK triggers so they can't re-open themselves
		AUX_SET(true,OSK_CLOSED);
		core_osk_mode = CORE_OSK_OFF;
		core_osk_button_last = 0;
	}
}

uint16_t core_input_mod_state(void)
{
	return (uint16_t)mod_state;
}

void core_input_keyboard_event(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers)
{
	//retro_log(RETRO_LOG_DEBUG,"core_input_keyboard_event(%d,%d,%d,%04X)\n",down,keycode,character,key_modifiers);
	SDL_Event event; memset(&event,0,sizeof(event));
	event.key.type = down ? SDL_KEYDOWN : SDL_KEYUP;
	event.key.state = down ? SDL_PRESSED : SDL_RELEASED;
	event.key.repeat = ((down && retrok_down[keycode]) || (!down && !retrok_down[keycode]));
	event.key.keysym.sym = retrok_to_sdl[keycode];
	// keysym.scancode is not used (and SDL_keyboard is not initialized)
	//event.key.keysym.scancode = SDL_GetScancodeFromKey(event.key.keysym.sym);
	event.key.keysym.mod = 0;
	// NUMLOCK is the only mod state that Hatari uses (for: Keymap_GetKeyPadScanCode)
	// but I have disabled it because Atari ST has no numlock
	//if (key_modifiers & RETROKMOD_NUMLOCK  ) event.key.keysym.mod |= KMOD_NUM;
	//if (key_modifiers & RETROKMOD_CAPSLOCK ) event.key.keysym.mod |= KMOD_CAPS;
	//if (key_modifiers & RETROKMOD_SCROLLOCK) event.key.keysym.mod |= KMOD_SCROLL;
	//if (retrok_down[RETROK_LSHIFT]) event.key.keysym.mod |= KMOD_LSHIFT;
	//if (retrok_down[RETROK_RSHIFT]) event.key.keysym.mod |= KMOD_RSHIFT;
	//if (retrok_down[RETROK_LCTRL ]) event.key.keysym.mod |= KMOD_LCTRL;
	//if (retrok_down[RETROK_RCTRL ]) event.key.keysym.mod |= KMOD_RCTRL;
	//if (retrok_down[RETROK_LALT  ]) event.key.keysym.mod |= KMOD_LALT;
	//if (retrok_down[RETROK_RALT  ]) event.key.keysym.mod |= KMOD_RALT;
	//if (retrok_down[RETROK_LSUPER]) event.key.keysym.mod |= KMOD_LGUI;
	//if (retrok_down[RETROK_RSUPER]) event.key.keysym.mod |= KMOD_RGUI;
	//if (retrok_down[RETROK_MODE  ]) event.key.keysym.mod |= KMOD_MODE;
	retrok_down[keycode] = down;
	event_queue_push(&event);
	(void)character;
}

void core_input_keyboard_event_callback(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers)
{
	#if CORE_INPUT_DEBUG
	if (core_input_debug)
		retro_log(RETRO_LOG_INFO,"core_input_keyboard_event_callback(%d,%d,%d,%04X)\n",down,keycode,character,key_modifiers);
	#endif
	//retro_log(RETRO_LOG_DEBUG,"core_input_keyboard_event_callback(%d,%d,%d,%04X)\n",down,keycode,character,key_modifiers);
	// assuming we don't need to mutex the event queue,
	// because it doesn't make sense for retro_keyboard_callback to operate during retro_run,
	// (run-ahead, netplay, etc. would need to depend on this?)
	if (!core_host_keyboard) return;
	core_input_keyboard_event(down, keycode, character, key_modifiers);
}

void core_input_keyboard_unstick() // release any keys that don't currently match state
{
	// NUMLOCK is the only mod state that Hatari uses (for: Keymap_GetKeyPadScanCode)
	// but I have disabled it because Atari ST has no numlock
	uint16_t mod = 0;
	//if (mod_state & KMOD_NUM   ) mod |= RETROKMOD_NUMLOCK;
	//if (mod_state & KMOD_CAPS  ) mod |= RETROKMOD_CAPSLOCK;
	//if (mod_state & KMOD_SCROLL) mod |= RETROKMOD_SCROLLOCK;

	for (int i=0; i<RETROK_LAST; ++i)
	{
		if (!retrok_down[i]) continue;
		if (retrok_joy[i]) continue;
		if (!core_host_keyboard || !input_state_cb(0,RETRO_DEVICE_KEYBOARD,0,i))
		{
			core_input_keyboard_event(false,i,0,mod); // release key
			//retro_log(RETRO_LOG_DEBUG,"core_input_keyboard_unstick() released: %d\n",i);
			#if CORE_INPUT_DEBUG
			if (core_input_debug)
				retro_log(RETRO_LOG_INFO,"core_input_keyboard_unstick() released: %d\n",i);
			#endif
		}
	}
}

void core_input_keyboard_joy(int keycode)
{
	retrok_joy[keycode] = 1;
	// numlock disabled (Atari ST has no numlock)
	//if (!retrok_down[keycode]) core_input_keyboard_event(true, keycode, 0, (mod_state & KMOD_NUM) ? RETROKMOD_NUMLOCK : 0);
	if (!retrok_down[keycode]) core_input_keyboard_event(true, keycode, 0, 0);
}

void core_input_serialize(void)
{
	// note: doesn't include event queue, and shouldn't need to (should be empty, but even if not the pending events haven't effected emulation yet)
	for (int i=0; i<RETROK_LAST; ++i) core_serialize_uint8(&retrok_down[i]);
	for (int i=0; i<4; ++i) core_serialize_uint8(&joy_autofire[i]);
	core_serialize_int32(&vmouse_x);
	core_serialize_int32(&vmouse_y);
	core_serialize_int32(&mod_state);
	core_serialize_int32(&aux_buttons);
	core_serialize_uint8(&osk_press_mod);
	core_serialize_int32(&osk_press_key);
	core_serialize_int32(&osk_press_time);
	core_serialize_int32(&jm_toggle_index);
}

void core_input_set_environment(retro_environment_t cb)
{
	static const struct retro_keyboard_callback keyboard_callback = {
		core_input_keyboard_event_callback
	};
	cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, (void*)&keyboard_callback);
}

void core_input_init(void)
{
	// generate mappings
	retrok_to_sdl_init();

	// clear state
	event_queue_init();
	memset(&retrok_down,0,sizeof(retrok_down));
	vmouse_x = vmouse_y = 0;
	jm_toggle_index = -1;
	aux_buttons = 0;
	for (int i=0; i<JOY_PORTS; ++i)
	{
		joy_fire[i] = 0;
		joy_stick[i] = 0;
	}
	for (int i=0; i<4; ++i) joy_autofire[i] = 0;
	osk_press_mod = 0;
	osk_press_key = 0;
	osk_press_time = 0;
}

void core_input_update(void)
{
	#if CORE_INPUT_DEBUG
	static int framecount = 0;
	if (core_input_debug)
	{
		retro_log(RETRO_LOG_INFO,"core_input_update() frame: %08X M %c%c\n",framecount,AUX(MOUSE_L)?'L':'.',AUX(MOUSE_R)?'R':'.');
		++framecount;
		if ((framecount % 512) == 0) // big state dump every ~10 seconds
		{
			retro_log(RETRO_LOG_INFO,"mappings: \n");
			for (int i=0; i<4; ++i)
				retro_log(RETRO_LOG_INFO,"pad%d>%d (%d%d%d) (%d %d %d %d %d %d %d %d %d %d %d %d)\n",
					i,core_joy_port_map[i],
					core_stick_map[i][0],
					core_stick_map[i][1],
					core_stick_map[i][2],
					core_button_map[i][0],
					core_button_map[i][1],
					core_button_map[i][2],
					core_button_map[i][3],
					core_button_map[i][4],
					core_button_map[i][5],
					core_button_map[i][6],
					core_button_map[i][7],
					core_button_map[i][8],
					core_button_map[i][9],
					core_button_map[i][10],
					core_button_map[i][11]);
			// not dumped:
			//  retrok_down
			//  mod_state
			//  joy_autofire
			//  joy_stick
			//  retrok_joy
			//  core_oskey_map
			//  aux_buttons not mouse
		}
	}
	#endif

	// accumulated virtual mouse state
	bool vm_l = false;
	bool vm_r = false;
	int vm_rx = 0;
	int vm_ry = 0;
	// accumulated temporary joystick state
	int32_t vjoy_fire[JOY_PORTS] = { 0,0,0,0,0,0 };
	int32_t vjoy_stick[JOY_PORTS] = { 0,0,0,0,0,0 };
	// accumulated auxiliary button state
	bool drive_swap = false;
	bool disk_swap = false;
	bool warm_boot = false;
	bool mouse_slow = false;
	bool mouse_fast = false;
	bool cold_boot = false;
	bool cpu_speed = false;
	bool statusbar = false;
	bool jm_toggle = false;
	int jm_toggle_source = -1;
	bool pause = false;
	bool osk_on = false;
	bool osk_shot = false;
	bool osk_button[OSK_INPUT_COUNT] = {false,false,false,false,false,false,false};

	// OSK has to block inputs when emulation is paused
	// Note that we never block physical keyboard inputs, because they don't conflict with OSK inputs,
	// and we need to track them for the unstick. The event queue is still processed when paused,
	// so Hatari should also have the correct state for them when we resume.
	const bool input_paused = (core_osk_mode == CORE_OSK_PAUSE || core_osk_mode == CORE_OSK_KEY_SHOT);
	const bool input_osk_shot = (core_osk_mode == CORE_OSK_KEY_SHOT);
	const bool input_osk_key = (core_osk_mode == CORE_OSK_KEY);

	input_poll_cb();

	// clear temporary state
	memset(retrok_joy,0,sizeof(retrok_joy));

	// have each joystick update its mapped controls
	for (int i=0; i<4; ++i)
	{
		#if CORE_INPUT_DEBUG
		static int debug_ax[3];
		static int debug_ay[3];
		static int debug_b[12];
		bool debug_pad = false;
		#endif

		// Is it possible to skip unconnected devices? does libretro have this ability?
		// probably not... ideally it should be able to report something if there is nothing mapped to an entire pad,
		// but this doesn't seem to be the case.

		int stick_threshold;
		int j = core_joy_port_map[i];
		// i = retropad
		// j = ST port (6 for none)

		// autofire countdown
		if (!input_paused && joy_autofire[i] > 0) --joy_autofire[i];

		// process sticks (includes d-pad)
		stick_threshold = (0x8000 * core_stick_threshold) / 100;
		for (int k=0; k<3; ++k)
		{
			static const unsigned DEVICE[3] = {
				0,
				RETRO_DEVICE_INDEX_ANALOG_LEFT,
				RETRO_DEVICE_INDEX_ANALOG_RIGHT,
			};
			int ax, ay;
			if (k == CORE_INPUT_STICK_DPAD)
			{
				ax = ay = 0;
				if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT )) ax -= 0x8000;
				if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT)) ax += 0x8000;
				if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP   )) ay -= 0x8000;
				if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN )) ay += 0x8000;
			}
			else
			{
				ax = input_state_cb(i, RETRO_DEVICE_ANALOG, DEVICE[k], RETRO_DEVICE_ID_ANALOG_X);
				ay = input_state_cb(i, RETRO_DEVICE_ANALOG, DEVICE[k], RETRO_DEVICE_ID_ANALOG_Y);
			}

			#if CORE_INPUT_DEBUG
			debug_ax[k] = ax;
			debug_ay[k] = ay;	
			if (ax > 0x2000 || ax < -0x2000 || ay > 0x2000 || ay <-0x2000)
				debug_pad = true;
			#endif

			if (core_oskey_map[i][CORE_INPUT_OSKEY_MOVE] == (k+1))
			{
				if (ax <= -stick_threshold) osk_button[OSK_L] = true;
				if (ax >=  stick_threshold) osk_button[OSK_R] = true;
				if (ay <= -stick_threshold) osk_button[OSK_U] = true;
				if (ay >=  stick_threshold) osk_button[OSK_D] = true;
				if (input_osk_key) continue; // when using OSK hide this axis
			}

			int csm = core_stick_map[i][k];
			if (i == jm_toggle_index) // Joy/Mouse swap
			{
				switch (csm)
				{
				case 1: csm = 2; break; // swap mouse for joy
				case 2: csm = 1; break; // swap joy for mouse
				default:         break;
				}
			}
			switch (csm) // cases must match OPTION_PAD_STICK in core_internal.h
			{
			default:
			case 0: // None
				break;
			case 1: // Joystick
				if (j < JOY_PORTS)
				{
					if (ax <= -stick_threshold) vjoy_stick[j] |= JOY_STICK_L;
					if (ax >=  stick_threshold) vjoy_stick[j] |= JOY_STICK_R;
					if (ay <= -stick_threshold) vjoy_stick[j] |= JOY_STICK_U;
					if (ay >=  stick_threshold) vjoy_stick[j] |= JOY_STICK_D;
				}
				break;
			case 2: // Mouse
				{
					int deadzone = (0x8000 * core_mouse_dead) / 100;
					float speed = (float)core_mouse_speed * (float)MOUSE_PRECISION * MOUSE_SPEED_FACTOR * ((float)0x8000 / (float)(0x8000 - deadzone));
					if (k == CORE_INPUT_STICK_DPAD) speed *= DPAD_MOUSE_SPEED; // D-Pad needs a slower speed

					if (ax < deadzone && ax > -deadzone) ax = 0;
					else if (ax < 0) ax += deadzone;
					else if (ax > 0) ax -= deadzone;
					if (ay < deadzone && ay > -deadzone) ay = 0;
					else if (ay < 0) ay += deadzone;
					else if (ay > 0) ay -= deadzone;

					vm_rx += (int)(ax * speed);
					vm_ry += (int)(ay * speed);
				} break;
			case 3: // Cursor Keys
				if (!input_paused)
				{
					if (ax <= -stick_threshold) core_input_keyboard_joy(RETROK_LEFT);
					if (ax >=  stick_threshold) core_input_keyboard_joy(RETROK_RIGHT);
					if (ay <= -stick_threshold) core_input_keyboard_joy(RETROK_UP);
					if (ay >=  stick_threshold) core_input_keyboard_joy(RETROK_DOWN);
				}
				break;
			}
		}

		// process buttons
		for (int k=0; k<12; ++k)
		{
			static const unsigned DEVICE[12] = {
				RETRO_DEVICE_ID_JOYPAD_A,
				RETRO_DEVICE_ID_JOYPAD_B,
				RETRO_DEVICE_ID_JOYPAD_X,
				RETRO_DEVICE_ID_JOYPAD_Y,
				RETRO_DEVICE_ID_JOYPAD_SELECT,
				RETRO_DEVICE_ID_JOYPAD_START,
				RETRO_DEVICE_ID_JOYPAD_L,
				RETRO_DEVICE_ID_JOYPAD_R,
				RETRO_DEVICE_ID_JOYPAD_L2,
				RETRO_DEVICE_ID_JOYPAD_R2,
				RETRO_DEVICE_ID_JOYPAD_L3,
				RETRO_DEVICE_ID_JOYPAD_R3,
			};
			if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, DEVICE[k]))
			{
				#if CORE_INPUT_DEBUG
				debug_b[k] = 1;
				debug_pad = true;
				#endif
				static const int BUTTON_KEY[] = // must match OPTION_PAD_BUTTON in core_internal.h
				{
					RETROK_SPACE,
					RETROK_RETURN,
					RETROK_UP,
					RETROK_DOWN,
					RETROK_LEFT,
					RETROK_RIGHT,
					RETROK_F1,
					RETROK_F2,
					RETROK_F3,
					RETROK_F4,
					RETROK_F5,
					RETROK_F6,
					RETROK_F7,
					RETROK_F8,
					RETROK_F9,
					RETROK_F10,
					RETROK_ESCAPE,
					RETROK_1,
					RETROK_2,
					RETROK_3,
					RETROK_4,
					RETROK_5,
					RETROK_6,
					RETROK_7,
					RETROK_8,
					RETROK_9,
					RETROK_0,
					RETROK_MINUS,
					RETROK_EQUALS,
					RETROK_BACKQUOTE,
					RETROK_BACKSPACE,
					RETROK_TAB,
					RETROK_q,
					RETROK_w,
					RETROK_e,
					RETROK_r,
					RETROK_t,
					RETROK_y,
					RETROK_u,
					RETROK_i,
					RETROK_o,
					RETROK_p,
					RETROK_LEFTBRACKET,
					RETROK_RIGHTBRACKET,
					RETROK_DELETE,
					RETROK_LCTRL,
					RETROK_a,
					RETROK_s,
					RETROK_d,
					RETROK_f,
					RETROK_g,
					RETROK_h,
					RETROK_j,
					RETROK_k,
					RETROK_l,
					RETROK_SEMICOLON,
					RETROK_QUOTE,
					RETROK_BACKSLASH,
					RETROK_LSHIFT,
					RETROK_z,
					RETROK_x,
					RETROK_c,
					RETROK_v,
					RETROK_b,
					RETROK_n,
					RETROK_m,
					RETROK_COMMA,
					RETROK_PERIOD,
					RETROK_SLASH,
					RETROK_RSHIFT,
					RETROK_LALT,
					RETROK_CAPSLOCK,
					RETROK_PRINT, // (Help)
					RETROK_END, // (Undo)
					RETROK_INSERT,
					RETROK_HOME,
					RETROK_PAGEUP, // (Left Paren)
					RETROK_PAGEDOWN, // (Right Paren)
					RETROK_KP_DIVIDE,
					RETROK_KP_MULTIPLY,
					RETROK_KP_MINUS,
					RETROK_KP_PLUS,
					RETROK_KP_ENTER,
					RETROK_KP_PERIOD,
					RETROK_KP0,
					RETROK_KP1,
					RETROK_KP2,
					RETROK_KP3,
					RETROK_KP4,
					RETROK_KP5,
					RETROK_KP6,
					RETROK_KP7,
					RETROK_KP8,
					RETROK_KP9,
				};
				#define BUTTON_KEY_COUNT   (sizeof(BUTTON_KEY)/sizeof(BUTTON_KEY[0]))

				const int m = core_button_map[i][k];

				// on-screen keyboard button mappings
				bool is_osk = false;
				for (int l=0; l<CORE_INPUT_OSKEY_BUTTONS; ++l)
				{
					if ((k+1) == core_oskey_map[i][l])
					{
						osk_button[l] = 1;
						is_osk = true;
					}
				}
				if (input_osk_key && is_osk) continue; // when using OSK hide these buttons

				// regular mappings
				if (m >= (BUTTON_KEY_START + BUTTON_KEY_COUNT)) continue;
				if (m >= BUTTON_KEY_START)
				{
					if (!input_paused)
						core_input_keyboard_joy(BUTTON_KEY[m-BUTTON_KEY_START]);
				}
				else
				{
					switch(m) // must match OPTION_PAD_BUTTION in core_internal.h
					{
					default:
					case 0: // None
						break;
					case 1: // Fire
						if (i != jm_toggle_index)
						{
							if (j < JOY_PORTS)
							{
								vjoy_stick[j] |= JOY_STICK_F;
								vjoy_fire[j] |= JOY_FIRE_A;
							}
						}
						else // swapped for mouse
							vm_l = true;
						break;
					case 2: // Auto-Fire
						if (j < JOY_PORTS && !input_paused)
						{
							if (joy_autofire[i] == 0)
							{
								joy_autofire[i] = core_autofire;
								vjoy_stick[j] |= JOY_STICK_F;
								vjoy_fire[j] |= JOY_FIRE_A;
							}
							else
							{
								if (joy_autofire[i] > (core_autofire/2))
								{
									vjoy_stick[j] |= JOY_STICK_F;
									vjoy_fire[j] |= JOY_FIRE_A;
								}
							}
						}
						break;
					case 3: // Mouse Left
						if (i == jm_toggle_index) // swapped for joy
						{
							if (j < JOY_PORTS)
							{
								vjoy_stick[j] |= JOY_STICK_F;
								vjoy_fire[j] |= JOY_FIRE_A;
							}
						}
						else
							vm_l = true;
						break;
					case 4: // Mouse Right
						vm_r = true;
						break;
					case 5: // On-Screen Keyboard
						osk_on = true;
						break;
					case 6: // On-Screen Keyboard One-Shot
						osk_shot = true;
						break;
					case 7: // Select Floppy Drive
						drive_swap = true;
						break;
					case 8: // Swap to Next Disk
						disk_swap = true;
						break;
					case 9: // Help Screen
						pause = true;
						break;
					case 10: // Joystick Up
						if (j < JOY_PORTS) vjoy_stick[j] |= JOY_STICK_U;
						break;
					case 11: // Joystick Down
						if (j < JOY_PORTS) vjoy_stick[j] |= JOY_STICK_D;
						break;
					case 12: // Joystick Left
						if (j < JOY_PORTS) vjoy_stick[j] |= JOY_STICK_L;
						break;
					case 13: // Joystick Right
						if (j < JOY_PORTS) vjoy_stick[j] |= JOY_STICK_R;
						break;
					case 14: // STE Button A
						if (j < JOY_PORTS)
						{
							vjoy_stick[j] |= JOY_STICK_F;
							vjoy_fire[j] |= JOY_FIRE_A;
						}
						break;
					case 15: // STE Button B
						if (j < JOY_PORTS) vjoy_fire[j] |= JOY_FIRE_B;
						break;
					case 16: // STE Button C
						if (j < JOY_PORTS) vjoy_fire[j] |= JOY_FIRE_C;
						break;
					case 17: // STE Button Option
						if (j < JOY_PORTS) vjoy_fire[j] |= JOY_FIRE_OPT;
						break;
					case 18: // STE Button Pause
						if (j < JOY_PORTS) vjoy_fire[j] |= JOY_FIRE_PAUSE;
						break;
					case 19: // Mouse Speed Slow
						mouse_slow = true;
						break;
					case 20: //  Mouse Speed Fast
						mouse_fast = true;
						break;
					case 21: // Soft Reset
						warm_boot = true;
						break;
					case 22: // Hard Reset
						cold_boot = true;
						break;
					case 23: // Cpu Speed
						cpu_speed = true;
						break;
					case 24: // Toggle Statusbar
						statusbar = true;
						break;
					case 25: // Joystick / Mouse Toggle
						jm_toggle = true;
						jm_toggle_source = i; // always affects the last person to press it
						break;
					}
				}
			}
			#if CORE_INPUT_DEBUG
			else debug_b[k] = 0;
			#endif
		}
		#if CORE_INPUT_DEBUG
		if (core_input_debug && debug_pad)
			retro_log(RETRO_LOG_INFO,"P%d %c%c%c%c%c%c%c%c%c%c%c%c %2d%2d %2d%2d %2d%2d\n",
				i,
				debug_b[ 0] ? 'A' : '.',
				debug_b[ 1] ? 'B' : '.',
				debug_b[ 2] ? 'X' : '.',
				debug_b[ 3] ? 'Y' : '.',
				debug_b[ 4] ? 's' : '.',
				debug_b[ 5] ? 'S' : '.',
				debug_b[ 6] ? 'L' : '.',
				debug_b[ 7] ? 'R' : '.',
				debug_b[ 8] ? 'l' : '.',
				debug_b[ 9] ? 'r' : '.',
				debug_b[10] ? '[' : '.',
				debug_b[11] ? ']' : '.',
				debug_ax[0] / 0x1000,
				debug_ay[0] / 0x1000,
				debug_ax[1] / 0x1000,
				debug_ay[1] / 0x1000,
				debug_ax[2] / 0x1000,
				debug_ay[2] / 0x1000);
		#endif
	}

	// apply osk presses
	if (osk_press_mod && ((osk_press_time > 0) || (core_osk_mode >= CORE_OSK_KEY)))
	{
		if (osk_press_mod & OSK_PRESS_CTRL) core_input_keyboard_joy(RETROK_LCTRL);
		if (osk_press_mod & OSK_PRESS_ALT ) core_input_keyboard_joy(RETROK_LALT);
		if (osk_press_mod & OSK_PRESS_SHL ) core_input_keyboard_joy(RETROK_LSHIFT);
		if (osk_press_mod & OSK_PRESS_SHR ) core_input_keyboard_joy(RETROK_RSHIFT);
	}
	if (osk_press_time > 0)
	{
		--osk_press_time;
		if (osk_press_key > 0 && osk_press_key < RETROK_LAST)
			core_input_keyboard_joy(osk_press_key);
	}

	// unstick any hanging keys
	if (!input_paused)
		core_input_keyboard_unstick();

	if (core_mouse_port && !input_paused) // mouse is connected to joy 0
	{
		if (core_host_mouse) // libretro mouse gives relative x/y
		{
			bool pm_l = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
			bool pm_r = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
			int pm_x  = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
			int pm_y  = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
			vm_l |= pm_l;
			vm_r |= pm_r;
			vm_rx += pm_x * MOUSE_PRECISION;
			vm_ry += pm_y * MOUSE_PRECISION;
			#if CORE_INPUT_DEBUG
			if (core_input_debug && (pm_l || pm_r || pm_x || pm_y))
				retro_log(RETRO_LOG_INFO,"M %c%c %3d %3d\n",
					pm_l ? 'L' : '.',
					pm_r ? 'R' : '.',
					pm_x, pm_y);
			#endif
		}

		if (mouse_slow)
		{
			vm_rx /= MOUSE_MODIFIER_FACTOR;
			vm_ry /= MOUSE_MODIFIER_FACTOR;
		}
		if (mouse_fast)
		{
			vm_rx *= MOUSE_MODIFIER_FACTOR;
			vm_ry *= MOUSE_MODIFIER_FACTOR;
		}
		int vm_x = vmouse_x + vm_rx;
		int vm_y = vmouse_y + vm_ry;

		if ((vm_l && !AUX(MOUSE_L)) || (!vm_l && AUX(MOUSE_L)))
		{
			SDL_Event event; memset(&event,0,sizeof(event));
			event.button.type = vm_l ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
			event.button.button = SDL_BUTTON_LEFT;
			event.button.state = vm_l ? SDL_PRESSED : SDL_RELEASED;
			event.button.clicks = 1;
			event.button.x = vm_x / MOUSE_PRECISION;
			event.button.y = vm_y / MOUSE_PRECISION;
			event_queue_push(&event);
			AUX_SET(vm_l,MOUSE_L);
			#if CORE_INPUT_DEBUG
			if (core_input_debug)
				retro_log(RETRO_LOG_INFO,"Mouse L %s\n",vm_l ? "DOWN" : "UP");
			#endif
		}

		if ((vm_r && !AUX(MOUSE_R)) || (!vm_r && AUX(MOUSE_R)))
		{
			SDL_Event event; memset(&event,0,sizeof(event));
			event.button.type = vm_r ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
			event.button.button = SDL_BUTTON_RIGHT;
			event.button.state = vm_r ? SDL_PRESSED : SDL_RELEASED;
			event.button.clicks = 1;
			event.button.x = vm_x / MOUSE_PRECISION;
			event.button.y = vm_y / MOUSE_PRECISION;
			event_queue_push(&event);
			AUX_SET(vm_r,MOUSE_R);
			#if CORE_INPUT_DEBUG
			if (core_input_debug)
				retro_log(RETRO_LOG_INFO,"Mouse R %s\n",vm_r ? "DOWN" : "UP");
			#endif
		}

		if (vm_x != vmouse_x || vm_y != vmouse_y)
		{
			SDL_Event event; memset(&event,0,sizeof(event));
			event.motion.type = SDL_MOUSEMOTION;
			event.motion.state = 0;
			if (AUX(MOUSE_L)) event.motion.state |= SDL_BUTTON_LMASK;
			if (AUX(MOUSE_R)) event.motion.state |= SDL_BUTTON_RMASK;
			event.motion.x = vm_x / MOUSE_PRECISION;
			event.motion.y = vm_y / MOUSE_PRECISION;
			event.motion.xrel = (vm_x / MOUSE_PRECISION) - (vmouse_x / MOUSE_PRECISION);
			event.motion.yrel = (vm_y / MOUSE_PRECISION) - (vmouse_y / MOUSE_PRECISION);
			event_queue_push(&event);
			vmouse_x = vm_x;
			vmouse_y = vm_y;
			#if CORE_INPUT_DEBUG
			if (core_input_debug)
				retro_log(RETRO_LOG_INFO,"Mouse Move %3d %3d\n",vm_x/MOUSE_PRECISION,vm_y/MOUSE_PRECISION);
			#endif
		}
	}

	if (input_paused)
	{
		// cancel auxiliary buttons
		drive_swap = false;
		disk_swap = false;
		warm_boot = false;
		cold_boot = false;
		cpu_speed = false;
		statusbar = false;
		jm_toggle = false;
		if (input_osk_shot) pause = false; // cancel only if in OSK one-shot mode (otherwise we need it to unpause!)
	}
	else
	{
		// apply joystick
		for (int i=0; i<JOY_PORTS; ++i)
		{
			joy_stick[i] = vjoy_stick[i];
			joy_fire[i] = vjoy_fire[i];
		}
	}

	// auxiliary buttons

	// select drive
	if (drive_swap && !AUX(DRIVE_SWAP)) core_disk_drive_toggle();
	AUX_SET(drive_swap,DRIVE_SWAP);

	// swap disk
	if (disk_swap && !AUX(DISK_SWAP)) core_disk_swap();
	AUX_SET(disk_swap,DISK_SWAP);

	// perform reset
	if (warm_boot && !AUX(WARM_BOOT)) Reset_Warm();
	if (cold_boot && !AUX(COLD_BOOT)) Reset_Cold();
	AUX_SET(warm_boot,WARM_BOOT);
	AUX_SET(cold_boot,COLD_BOOT);

	// CPU speed cycle
	if (cpu_speed && !AUX(CPU_SPEED)) config_cycle_cpu_speed();
	AUX_SET(cpu_speed,CPU_SPEED);

	// status bar toggle
	if (statusbar && !AUX(STATUSBAR)) config_toggle_statusbar();
	AUX_SET(statusbar,STATUSBAR);

	// Jostick / Mouse toggle
	if (jm_toggle && !AUX(JM_TOGGLE))
	{
		if   (jm_toggle_index >= 0) jm_toggle_index = -1; // toggle off
		else                        jm_toggle_index = jm_toggle_source; // toggle on for the person who pressed the button
	}
	AUX_SET(jm_toggle,JM_TOGGLE);

	// pause/help toggle
	// onscreen keyboard toggle
	if (pause && !AUX(PAUSE))
	{
		if (core_osk_mode == CORE_OSK_OFF  )
		{
			core_osk_mode = CORE_OSK_PAUSE;
			core_osk_begin = 1;
		}
		else if (core_osk_mode == CORE_OSK_PAUSE)
		{
			core_input_osk_close();
		}
		//retro_log(RETRO_LOG_DEBUG,"pause toggle: %d\n",core_osk_mode);
	}
	AUX_SET(pause,PAUSE);

	// if osk was just closed, prevent retrigger one time
	if (AUX(OSK_CLOSED))
	{
		AUX_SET(osk_on,OSK_ON);
		AUX_SET(osk_shot,OSK_SHOT);
		AUX_SET(false,OSK_CLOSED);
	}

	if (osk_on && !AUX(OSK_ON) && (core_osk_mode == CORE_OSK_OFF))
	{
		core_osk_mode = CORE_OSK_KEY;
		core_osk_begin = 1;
	}
	AUX_SET(osk_on,OSK_ON);

	if (osk_shot && !AUX(OSK_SHOT) && (core_osk_mode == CORE_OSK_OFF))
	{
		core_osk_mode = CORE_OSK_KEY_SHOT;
		core_osk_begin = 1;
	}
	AUX_SET(osk_shot,OSK_SHOT);

	uint32_t osk_new = aux_buttons; // osk_new is temporarily "osk_old"
	AUX_SET(osk_button[OSK_CONFIRM],OSK_CONFIRM);
	AUX_SET(osk_button[OSK_CANCEL ],OSK_CANCEL);
	AUX_SET(osk_button[OSK_POS    ],OSK_POS);
	AUX_SET(osk_button[OSK_U      ],OSK_U);
	AUX_SET(osk_button[OSK_D      ],OSK_D);
	AUX_SET(osk_button[OSK_L      ],OSK_L);
	AUX_SET(osk_button[OSK_R      ],OSK_R);
	osk_new = (osk_new ^ aux_buttons) & aux_buttons & AUX_OSK_ALL; // OSK buttons pressed this frame
	if (core_osk_mode >= CORE_OSK_KEY)
	{
		core_osk_input(osk_new, aux_buttons & AUX_OSK_ALL);
	}

	#if CORE_INPUT_DEBUG
	if (core_input_debug)
	{
		retro_log(RETRO_LOG_INFO,"--- J01: %02X %02X  M: %c%c %5d %5d\n",
			joy_fire[0],
			joy_fire[1],
			AUX(MOUSE_L) ? 'L' : '.',
			AUX(MOUSE_R) ? 'R' : '.',
			vmouse_x/MOUSE_PRECISION,
			vmouse_y/MOUSE_PRECISION);
	}
	#endif
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
