// translate input events into SDL events for hatari

#include "../libretro/libretro.h"
#include "core.h"
#include "core_internal.h"
#include <SDL2/SDL.h>

//
// Internal input state
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

// input state that gets serialized
static uint8_t retrok_down[RETROK_LAST] = {0}; // for repeat tracking
static int32_t vmouse_x, vmouse_y; // virtual mouse state
static uint8_t vmouse_l, vmouse_r;
static int32_t mod_state;
static uint8_t drive_toggle; // TODO make a bitfield of osk, drive toggle, resets? mouse l/r?
static uint8_t joy_autofire[4];

// input state that is temporary
static int32_t joy_fire[JOY_PORTS];
static int32_t joy_stick[JOY_PORTS];
static uint8_t retrok_joy[RETROK_LAST]; // overlay to retrok_down

// input mappings
int core_joy_port_map[4] = {1,0,2,3};
int core_stick_map[4][3];
int core_button_map[4][12];
int core_oskey_map[4][5];

// other input configuration
bool core_mouse_port = true;
bool core_host_keyboard = true;
bool core_host_mouse = true;
int core_autofire = 6;
int core_stick_threshold = 30; // percentage of stick to digital joystick direction threshold
int core_mouse_speed = 4; // 1-10 speed factor
int core_mouse_dead = 5; // percentage of stick deadzone

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
	event.key.keysym.scancode = SDL_GetScancodeFromKey(event.key.keysym.sym);
	event.key.keysym.mod = 0;
	// NUMLOCK is the only mod state that Hatari uses (for: Keymap_GetKeyPadScanCode)
	if (key_modifiers & RETROKMOD_NUMLOCK  ) event.key.keysym.mod |= KMOD_NUM;
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
	uint16_t mod = 0;
	if (mod_state & KMOD_NUM   ) mod |= RETROKMOD_NUMLOCK;
	//if (mod_state & KMOD_CAPS  ) mod |= RETROKMOD_CAPSLOCK;
	//if (mod_state & KMOD_SCROLL) mod |= RETROKMOD_SCROLLOCK;

	for (int i=0; i<RETROK_LAST; ++i)
	{
		if (!retrok_down[i]) continue;
		if (retrok_joy[i]) continue;
		if (!core_host_keyboard || !input_state_cb(0,RETRO_DEVICE_KEYBOARD,0,i))
		{
			core_input_keyboard_event(false,i,0,mod); // release key
			retro_log(RETRO_LOG_DEBUG,"core_input_keyboard_unstick() released: %d\n",i);
		}
	}
}

void core_input_keyboard_joy(int keycode)
{
	retrok_joy[keycode] = 1;
	if (!retrok_down[keycode]) core_input_keyboard_event(true, keycode, 0, (mod_state & KMOD_NUM) ? RETROKMOD_NUMLOCK : 0);
}

void core_input_serialize(void)
{
	// note: doesn't include event queue, and shouldn't need to (should be empty, but even if not the pending events haven't effected emulation yet)
	for (int i=0; i<RETROK_LAST; ++i) core_serialize_uint8(&retrok_down[i]);
	for (int i=0; i<4; ++i) core_serialize_uint8(&joy_autofire[i]);
	core_serialize_int32(&vmouse_x);
	core_serialize_int32(&vmouse_y);
	core_serialize_uint8(&vmouse_l);
	core_serialize_uint8(&vmouse_r);
	core_serialize_int32(&mod_state);
	core_serialize_uint8(&drive_toggle);
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
	vmouse_l = vmouse_r = false;
	drive_toggle = 0;
	for (int i=0; i<JOY_PORTS; ++i)
	{
		joy_fire[i] = 0;
		joy_stick[i] = 0;
	}
	for (int i=0; i<4; ++i) joy_autofire[i] = 0;
}

void core_input_update(void)
{
	// virtual mouse state
	bool vm_l = false;
	bool vm_r = false;
	int vm_x = vmouse_x;
	int vm_y = vmouse_y;
	bool dt = false;
	// onscreen keyboard state
	int osk_stick = 0;
	int osk_button[4] = {0,0,0,0};

	input_poll_cb();

	// clear all temporary state
	for (int i=0; i<JOY_PORTS; ++i)
	{
		joy_fire[i] = 0;
		joy_stick[i] = 0;
	}
	memset(retrok_joy,0,sizeof(retrok_joy));

	// have each joystick update its mapped controls
	for (int i=0; i<4; ++i)
	{
		int stick_threshold;
		int j = core_joy_port_map[i];
		// i = retropad
		// j = ST port (6 for none)

		// autofire countdown
		if (joy_autofire[i] > 0) --joy_autofire[i];

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

			if (core_oskey_map[i][CORE_INPUT_OSKEY_MOVE] == (k+1))
			{
				if (ax <= -stick_threshold) osk_stick |= JOY_STICK_L;
				if (ax >=  stick_threshold) osk_stick |= JOY_STICK_R;
				if (ay <= -stick_threshold) osk_stick |= JOY_STICK_U;
				if (ay >=  stick_threshold) osk_stick |= JOY_STICK_D;
			}

			switch (core_stick_map[i][k]) // must match options in core_config.c
			{
			default:
			case 0: // None
				break;
			case 1: // Joystick
				if (j < JOY_PORTS)
				{
					if (ax <= -stick_threshold) joy_stick[j] |= JOY_STICK_L;
					if (ax >=  stick_threshold) joy_stick[j] |= JOY_STICK_R;
					if (ay <= -stick_threshold) joy_stick[j] |= JOY_STICK_U;
					if (ay >=  stick_threshold) joy_stick[j] |= JOY_STICK_D;
				}
				break;
			case 2: // Mouse
				{
					int deadzone = (0x8000 * core_mouse_dead) / 100;
					const float SPEED_FACTOR = 0.0001;
					float speed = (float)core_mouse_speed * SPEED_FACTOR * ((float)0x8000 / (float)(0x8000 - deadzone));
					if (k == CORE_INPUT_STICK_DPAD) speed *= 0.4; // D-Pad needs a slower speed

					if (ax < deadzone && ax > -deadzone) ax = 0;
					else if (ax < 0) ax += deadzone;
					else if (ax > 0) ax -= deadzone;
					if (ay < deadzone && ay > -deadzone) ay = 0;
					else if (ay < 0) ay += deadzone;
					else if (ay > 0) ay -= deadzone;

					vm_x += (int)(ax * speed);
					vm_y += (int)(ay * speed);
				} break;
			case 3: // Cursor Keys
				if (ax <= -stick_threshold) core_input_keyboard_joy(RETROK_LEFT);
				if (ax >=  stick_threshold) core_input_keyboard_joy(RETROK_RIGHT);
				if (ay <= -stick_threshold) core_input_keyboard_joy(RETROK_UP);
				if (ay >=  stick_threshold) core_input_keyboard_joy(RETROK_DOWN);
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
				static const int BUTTON_KEY[] = // must match options in core_config.c
				{
					RETROK_SPACE, // 16
					RETROK_RETURN,
					RETROK_UP,
					RETROK_DOWN,
					RETROK_LEFT,
					RETROK_RIGHT,
					RETROK_F1, // 22
					RETROK_F2,
					RETROK_F3,
					RETROK_F4,
					RETROK_F5,
					RETROK_F6,
					RETROK_F7,
					RETROK_F8,
					RETROK_F9,
					RETROK_F10,
					RETROK_ESCAPE, // 32
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
					RETROK_TAB, // 47
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
					RETROK_LCTRL, // 61
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
					RETROK_LSHIFT, // 74
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
					RETROK_LALT, // 86
					RETROK_CAPSLOCK,
					RETROK_F11, // 88 (Help)
					RETROK_F12, // (Undo)
					RETROK_INSERT,
					RETROK_HOME,
					RETROK_PAGEUP, // (Left Paren)
					RETROK_PAGEDOWN, // (Right Paren)
					RETROK_KP_DIVIDE, // 92
					RETROK_KP_MULTIPLY,
					RETROK_KP_MINUS,
					RETROK_KP_PLUS,
					RETROK_KP_ENTER,
					RETROK_KP_PERIOD,
					RETROK_KP0, // 100
					RETROK_KP1,
					RETROK_KP2,
					RETROK_KP3,
					RETROK_KP4,
					RETROK_KP5,
					RETROK_KP6,
					RETROK_KP7,
					RETROK_KP8,
					RETROK_KP9, // 109
				};
				#define BUTTON_KEY_COUNT   (sizeof(BUTTON_KEY)/sizeof(BUTTON_KEY[0]))
				#define BUTTON_KEY_START   16

				const int m = core_button_map[i][k];

				// on-screen keyboard button mappings
				for (int l=0; l<4; ++l)
				{
					if (m == core_oskey_map[i][l]) osk_button[l] = 1;
				}

				// regular mappings
				if (m >= (BUTTON_KEY_START + BUTTON_KEY_COUNT)) continue;
				if (m >= BUTTON_KEY_START)
				{
					core_input_keyboard_joy(BUTTON_KEY[m-BUTTON_KEY_START]);
				}
				else
				{
					switch(m) // must match options in core_config.c
					{
					default:
					case 0: // None
						break;
					case 1: // Fire
						if (j < JOY_PORTS)
						{
							joy_stick[j] |= JOY_STICK_F;
							joy_fire[j] |= JOY_FIRE_A;
						}
						break;
					case 2: // Auto-Fire
						if (j < JOY_PORTS)
						{
							if (joy_autofire[i] == 0)
							{
								joy_autofire[i] = core_autofire;
								joy_stick[j] |= JOY_STICK_F;
								joy_fire[j] |= JOY_FIRE_A;
							}
							else
							{
								if (joy_autofire[i] > (core_autofire/2))
								{
									joy_stick[j] |= JOY_STICK_F;
									joy_fire[j] |= JOY_FIRE_A;
								}
							}
						}
						break;
					case 3: // Mouse Left
						vm_l = true;
						break;
					case 4: // Mouse Right
						vm_r = true;
						break;
					case 5: // On-Screen Keyboard
						// TODO
						break;
					case 6: // On-Screen Keyboard One-Shot
						// TODO
						break;
					case 7: // Select Floppy Drive
						dt = true;
						break;
					case 8: // Help Screen
						// TODO
						break;
					case 9: // STE Button A
						if (j < JOY_PORTS)
						{
							joy_stick[j] |= JOY_STICK_F;
							joy_fire[j] |= JOY_FIRE_A;
						}
						break;
					case 10: // STE Button B
						if (j < JOY_PORTS) joy_fire[j] |= JOY_FIRE_B;
						break;
					case 11: // STE Button C
						if (j < JOY_PORTS) joy_fire[j] |= JOY_FIRE_C;
						break;
					case 12: // STE Button Option
						if (j < JOY_PORTS) joy_fire[j] |= JOY_FIRE_OPT;
						break;
					case 13: // STE Button Pause
						if (j < JOY_PORTS) joy_fire[j] |= JOY_FIRE_PAUSE;
						break;
					case 14: // Soft Reset
						// TODO
						break;
					case 15: // Hard Reset
						// TODO
						break;
					}
				}
			}
		}
	}
	(void)osk_button; // TODO suppress unused variable error?

	// unstick any hanging keys
	core_input_keyboard_unstick();

	// select drive
	if (dt && !drive_toggle) core_disk_drive_toggle();
	drive_toggle = dt ? 1 : 0;

	if (core_mouse_port) // mouse is connected to joy 0
	{
		if (core_host_mouse) // libretro mouse gives relative x/y
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
