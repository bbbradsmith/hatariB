#include "../libretro/libretro.h"
#include "core.h"
#include "core_internal.h"
#include <string.h>
#include <stdlib.h>

// large enough for TT high resolution 1280x960 at 32bpp
#define VIDEO_MAX_W   2048
#define VIDEO_MAX_H   1024
#define VIDEO_MAX_PITCH   (VIDEO_MAX_W*4)
// 4 frames of buffer at slowest framerate
#define AUDIO_BUFFER_LEN   (4*2*96000/50)

// Header size must accomodate core data before the hatari memory snapshot
// the base savesate for a 1MB ST is about 3.5MB
// inserting floppies adds to it, they might be as large as 2MB each
// using an 8MB minimum (+ ST memory size over 1MB) accomodates this.
// The overhead is added to the initial estimate just in case it's not quite enough,
// and the rounding just makes the file size into a round number,
// because I thought it was aesthetically pleasing to do so.
#define SNAPSHOT_HEADER_SIZE   1024
#define SNAPSHOT_MINIMUM       (8 * 1024 * 1024)
#define SNAPSHOT_OVERHEAD      (64 * 1024)
#define SNAPSHOT_ROUND         (64 * 1024)
#define SNAPSHOT_VERSION       1

// Transmit Hatari log message to the Libretro debug log
// 0 = none, 1 = errors, 2 = all
#define DEBUG_HATARI_LOG   0

// Logs seem valid for either first retro_set_environment or everything else,
// set this to 1 when you want to log the first call to retro_set_environment.
#define DEBUG_RETRO_SET_ENVIRONMENT   0

// make sure this matches ../info/hatarib.info
static const char* const CORE_FILE_EXTENSIONS = "st|msa|dim|stx|m3u|m3u8|zip|gz";
// IPF/RAW/CRT support requires CAPSLIB which has licensing issues for Libretro.
// See: https://github.com/libretro/hatari/issues/4
// See also: https://github.com/mamedev/mame/blob/master/src/lib/formats/ipf_dsk.cpp
//static const char* const CORE_FILE_EXTENSIONS = "st|msa|dim|stx|ipf|raw|ctr|m3u|m3u8|zip|gz";

// serialization quirks
const uint64_t QUIRKS = RETRO_SERIALIZATION_QUIRK_ENDIAN_DEPENDENT;

//
// Libretro
//

static void null_log(enum retro_log_level level, const char* msg, ...) { (void)level; (void)msg; }

retro_environment_t environ_cb;
retro_video_refresh_t video_cb;
retro_audio_sample_t audio_sample_cb;
retro_audio_sample_batch_t audio_batch_cb;
retro_input_poll_t input_poll_cb;
retro_input_state_t input_state_cb;
retro_log_printf_t retro_log = null_log;

static const bool BOOL_TRUE = true;
static const enum retro_pixel_format RPF_0RGB1555 = RETRO_PIXEL_FORMAT_0RGB1555;
static const enum retro_pixel_format RPF_XRGB8888 = RETRO_PIXEL_FORMAT_XRGB8888;
static const enum retro_pixel_format RPF_RGB565   = RETRO_PIXEL_FORMAT_RGB565;

//
// From Hatari
//

extern uint8_t* STRam;
extern uint32_t STRamEnd;
extern int TOS_DefaultLanguage(void);
extern int Reset_Warm(void);
extern int Reset_Cold(void);
extern void UAE_Set_Quit_Reset ( bool hard );
extern void core_flush_audio(void);
extern int core_save_state(void);
extern int core_restore_state(void);

//
// Available to Hatari
//

// external

int core_pixel_format = 0;
bool core_init_return = false;
uint8_t core_runflags = 0;
int core_trace_countdown = 0;
uint8_t* core_rom_mem_pointer = NULL;
int core_crashtime = 10;
int core_crash_frames = 0; // reset to 0 whenever CORE_RUNFLAG_HALT
bool core_option_soft_reset = false;
bool core_show_welcome = true;

// internal

bool content_override_set = false;
bool core_video_restore = false;

uint32_t blank_screen[320*200] = { 0 }; // safety buffer in case frame was never been provided

void* core_video_buffer = blank_screen;
int16_t core_audio_buffer[AUDIO_BUFFER_LEN];
int16_t core_audio_last[2] = { 0, 0 };
double core_audio_hold_remain = 0;
int core_video_w = 320;
int core_video_h = 200;
int core_video_pitch = 320 * sizeof(uint32_t);
int core_video_resolution = 0;
float core_video_aspect = 1.0;
int core_video_aspect_mode = 0;
int core_video_fps = 50;
int core_video_fps_new = 50;
int core_audio_samplerate = 48000;
int core_audio_samplerate_new = 48000;
int core_audio_samples_pending = 0;
bool core_video_changed = false;
bool core_rate_changed = false;
// fps and samplerate update a "new" variable,
// which is later transferred to the actual variable.
// This is because they can sometimes be updated multiple times
// in a single frame, and onl the last one matters.
// (Savestate tends to set them once spuriously during its reset phase.)

static void retro_log_init()
{
	struct retro_log_callback log_cb;
	if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log_cb))
		retro_log = log_cb.log;
	else
		retro_log = null_log;
	retro_log(RETRO_LOG_INFO,"retro_set_environment()\n");
}

static void retro_memory_maps()
{
	// updates memory map information
	static struct retro_memory_descriptor memory_map[] = {
		{
			RETRO_MEMDESC_BIGENDIAN | RETRO_MEMDESC_SYSTEM_RAM | RETRO_MEMDESC_VIDEO_RAM | RETRO_MEMDESC_ALIGN_2 | RETRO_MEMDESC_MINSIZE_2,
			NULL, 0, 0, 0, 0, 0, "Main RAM"
		},
		{
			RETRO_MEMDESC_BIGENDIAN | RETRO_MEMDESC_CONST | RETRO_MEMDESC_ALIGN_2 | RETRO_MEMDESC_MINSIZE_2,
			NULL, 0, 0xE00000, 0, 0, 0, "High ROM"
		},
	};
	static struct retro_memory_map memory_maps = { memory_map, CORE_ARRAY_SIZE(memory_map) };
	memory_map[0].ptr = STRam;
	memory_map[0].len = STRamEnd;
	memory_map[1].ptr = core_rom_mem_pointer + 0xE00000;
	memory_map[1].len =                        0x200000;
	environ_cb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, (void*)&memory_maps);
}

void core_debug_msg(const char* msg)
{
	retro_log(RETRO_LOG_DEBUG,"%s\n",msg);
}

void core_debug_int(const char* msg, int num)
{
	retro_log(RETRO_LOG_DEBUG,"%s%d\n",msg,num);
}

void core_debug_hex(const char* msg, unsigned int num)
{
	retro_log(RETRO_LOG_DEBUG,"%s%08X\n",msg,num);
}

void core_error_msg(const char* msg)
{
	retro_log(RETRO_LOG_ERROR,"%s\n",msg);
}

void core_info_msg(const char* msg)
{
	retro_log(RETRO_LOG_INFO,"%s\n",msg);
}

void core_debug_hatari(bool error, const char* msg)
{
#if (DEBUG_HATARI_LOG < 1)
	(void)msg;
#else
	int len;
	#if (DEBUG_HATARI_LOG < 2)
		if (!error) return;
	#endif
	len = strlen(msg);
	retro_log(
		error ? RETRO_LOG_ERROR : RETRO_LOG_DEBUG,
		(len == 0 || (msg[len-1] != '\n')) ? "Hatari: %s\n" : "Hatari: %s",
		msg);
#endif
}

void core_trace_next(int count)
{
	Log_SetTraceOptions("cpu_all");
	core_trace_countdown = count;
}

void core_debug_bin(const char* data, int len, int offset)
{
	#define DEBUG_BIN_COLUMNS 32
	static char line[3+(DEBUG_BIN_COLUMNS*4)];
	static const char HEX[] = "0123456789ABCDEF";
	static const int cols = DEBUG_BIN_COLUMNS;
	retro_log(RETRO_LOG_DEBUG,"core_debug_bin(%p,%d,%x)\n",data,len,offset);
	for (; len > 0; data += cols, offset += cols, len -= cols)
	{
		// fill line with spaces, terminal zero
		memset(line,' ',sizeof(line));
		line[sizeof(line)-1] = 0;
		// fill data
		for (int i=0; i<cols && i<len ; ++i)
		{
			char b = data[i];
			int shift = (i >= (cols/2)) ? 1 : 0; // extra space mid-row
			int pos = shift + (i*3);
			// hex
			line[pos+0] = HEX[(b>>4)&0xF];
			line[pos+1] = HEX[b&0xF];
			// ascii
			pos = shift + i + 2 + (cols*3);
			line[pos] = '.'; // default if not printable
			if (b >= 0x20 && b < 0x7F) line[pos] = b;
		}
		retro_log(RETRO_LOG_DEBUG,"%010X: %s\n",offset,line);
	}
}

// resolution values are in hatari/src/includes/screen.h:
//  0 ST_LOW
//  1 ST_MEDIUM
//  2 ST_HIGH
//  3 ?
//  4 TT_MEDIUM_RES
//  5 ?
//  6 TT_HIGH_RES
//  7 TT_LOW_RES
// Currenlty only servicing the first 3 and the rest get a default.
// TODO threw in some basic numbers I found, but need to double check and calculate for myself.
//   NTSC/PAL should be computable by their pixel clock timings vs. other known systems.
//   Is Atari monitor really 5/6, is monochrome really 1/1? Though, they were also adjustable.
#define RESOLUTION_COUNT 4
static const double PIXEL_ASPECT_RATIO[4][RESOLUTION_COUNT] = {
	//  low,   med,  high, default
	{ 1./1., 1./1., 1./1., 1./1. }, // square pixels
	{ 5./6., 5./6., 1./1., 5./6. }, // Atari monitor
	{ 0.846, 0.846, 0.846, 0.846 }, // NTSC TV (monochrome is fiction)
	{ 1.040, 1.040, 1.040, 1.040 }, // PAL TV (monochrome is fiction)
};

void core_video_update(void* data, int w, int h, int pitch, int resolution)
{
	if (data == NULL) // provide safety buffer until restored
	{
		core_video_buffer = blank_screen;
		w = 320;
		h = 200;
		pitch = 320 * 4;
		resolution = core_video_resolution; // unchanged
	}

	if (w > VIDEO_MAX_W) w = VIDEO_MAX_W;
	if (h > VIDEO_MAX_H) w = VIDEO_MAX_H;
	if (pitch > VIDEO_MAX_PITCH) w = VIDEO_MAX_PITCH;
	core_video_buffer = data;
	if (w != core_video_w) { core_video_w = w; core_video_changed = true; }
	if (h != core_video_h) { core_video_h = h; core_video_changed = true; }
	core_video_pitch = pitch;
	if (resolution >= RESOLUTION_COUNT) resolution = RESOLUTION_COUNT-1;
	if (core_video_resolution != resolution)
	{
		core_video_changed = true;
		core_video_resolution = resolution;
	}
}

void core_audio_update(const int16_t data[][2], int index, int length)
{
	int pos = core_audio_samples_pending;
	int len = length * 2;
	int max = AUDIO_BUFFER_LEN - pos;
	if (len > max) len = max;
	core_audio_samples_pending += len;
	int l2 = len / 2;
	for (int i=0; i<l2; ++i)
	{
		core_audio_buffer[pos+0] = data[index+i][0];
		core_audio_buffer[pos+1] = data[index+i][1];
		pos += 2;
	}
	// save last sample in case hold is needed
	if (l2 > 0)
	{
		core_audio_last[0] = data[index+l2-1][0];
		core_audio_last[1] = data[index+l2-1][1];
	}
}

static void core_audio_hold(int length)
{
	// generate hold samples to fill a pause
	int pos = core_audio_samples_pending;
	int len = length * 2;
	int max = AUDIO_BUFFER_LEN - pos;
	if (len > max) len = max;
	core_audio_samples_pending += len;
	int l2 = len / 2;
	for (int i=0; i<l2; ++i)
	{
		core_audio_buffer[pos+0] = core_audio_last[0];
		core_audio_buffer[pos+1] = core_audio_last[1];
		pos += 2;
	}
}

void core_set_fps(int rate)
{
	//retro_log(RETRO_LOG_DEBUG,"core_set_fps(%d)\n",rate);
	if (rate != core_video_fps_new) core_rate_changed = true;
	core_video_fps_new = rate;
}

void core_set_samplerate(int rate)
{
	//retro_log(RETRO_LOG_DEBUG,"core_set_samplerate(%d)\n",rate);
	if (rate != core_audio_samplerate_new) core_rate_changed = true;
	core_audio_samplerate_new = rate;
}

// halting / reset

void core_signal_halt(void)
{
	retro_log(RETRO_LOG_ERROR,"CPU has halted.\n");
	if (!(core_runflags & CORE_RUNFLAG_HALT))
	{
		struct retro_message_ext msg;
		msg.msg = "CPU has crashed!";
		msg.duration = 8 * 1000;
		msg.priority = 3;
		msg.level = RETRO_LOG_ERROR;
		msg.target = RETRO_MESSAGE_TARGET_ALL;
		msg.type = RETRO_MESSAGE_TYPE_NOTIFICATION;
		msg.progress = -1;
		environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, &msg);
	}
	core_runflags |= CORE_RUNFLAG_HALT;
	core_crash_frames = 0;
}

void core_signal_tos_fail(void)
{
	retro_log(RETRO_LOG_ERROR,"Could not load TOS.\n");
	struct retro_message_ext msg;
	msg.msg = "TOS not found. See System > TOS ROM in core settings.";
	msg.duration = 10 * 1000;
	msg.priority = 3;
	msg.level = RETRO_LOG_ERROR;
	msg.target = RETRO_MESSAGE_TARGET_ALL;
	msg.type = RETRO_MESSAGE_TYPE_NOTIFICATION;
	msg.progress = -1;
	environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, &msg);
	core_runflags |= CORE_RUNFLAG_HALT;
	core_crash_frames = 0;
}

void core_signal_alert(const char* alertmsg)
{
	struct retro_message_ext msg;
	msg.msg = alertmsg;
	msg.duration = 5 * 1000;
	msg.priority = 1;
	msg.level = RETRO_LOG_INFO;
	msg.target = RETRO_MESSAGE_TARGET_ALL;
	msg.type = RETRO_MESSAGE_TYPE_NOTIFICATION;
	msg.progress = -1;
	environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, &msg);
}

void core_signal_reset(bool cold) // called by Reset_ST, allows the retro_run loop to know a reset happened.
{
	//retro_log(RETRO_LOG_DEBUG,"core_signal_reset(%d)\n",cold);
	core_runflags |= cold ? CORE_RUNFLAG_RESET_COLD : CORE_RUNFLAG_RESET_WARM;
}

//
// MIDI interface
//

struct retro_midi_interface* retro_midi = NULL;
bool midi_needs_flush = false;
uint32_t midi_delta_time = 0;

static void core_midi_set_environment(retro_environment_t cb)
{
	// if we call this a second time after succeeding it seems to fail, so just keep the first one we get
	static struct retro_midi_interface retro_midi_interface;
	if (!retro_midi && cb(RETRO_ENVIRONMENT_GET_MIDI_INTERFACE,&retro_midi_interface))
	{
		retro_midi = &retro_midi_interface;
	}
	if (retro_midi)
	{
		retro_log(RETRO_LOG_INFO,"MIDI interface available.\n");
		retro_log(RETRO_LOG_INFO,"MIDI IN: %s\n",retro_midi->input_enabled() ? "Enabled" : "Disabled");
		retro_log(RETRO_LOG_INFO,"MIDI OUT: %s\n",retro_midi->output_enabled() ? "Enabled" : "Disabled");
	}
	else
	{
		retro_midi = NULL;
		retro_log(RETRO_LOG_INFO,"MIDI interface not available.\n");
	}
}

bool core_midi_read(uint8_t* data)
{
	//retro_log(RETRO_LOG_DEBUG,"core_midi_read(%p)\n",data);
	if (retro_midi && retro_midi->input_enabled() && retro_midi->read(data))
	{
		//retro_log(RETRO_LOG_DEBUG,"MIDI READ: %02X\n",*data);
		return true;
	}
	return false;
}

bool core_midi_write(uint8_t data)
{
	//retro_log(RETRO_LOG_DEBUG,"core_midi_write(%02X)\n",data);
	if (retro_midi && retro_midi->output_enabled() && retro_midi->write(data,midi_delta_time))
	{
		//retro_log(RETRO_LOG_DEBUG,"MIDI WRITE: %02X (%d ms)\n",data,midi_delta_time);
		midi_delta_time = 0;
		midi_needs_flush = true;
		return true;
	}
	midi_delta_time = 0;
	return false;
}

static void core_midi_frame()
{
	if (midi_needs_flush && retro_midi && retro_midi->output_enabled())
		retro_midi->flush();
	midi_needs_flush = false;
	midi_delta_time += (1000 / core_video_fps);
}

//
// memory snapshot simulated file
//

char* snapshot_buffer = NULL;
int snapshot_pos = 0;
int snapshot_max = 0;
int snapshot_size = 0;
bool snapshot_error = false;

static void core_snapshot_open_internal(void)
{
	//retro_log(RETRO_LOG_DEBUG,"core_snapshot_open_internal()\n");
	snapshot_pos = 0;
	snapshot_max = 0;
	snapshot_error = false;
}

void core_snapshot_open(void)
{
	//retro_log(RETRO_LOG_DEBUG,"core_snapshot_open()\n");
	snapshot_pos = SNAPSHOT_HEADER_SIZE;
	snapshot_max = snapshot_pos;
}

void core_snapshot_close(void)
{
	//retro_log(RETRO_LOG_DEBUG,"core_snapshot_close() max: %X = %d\n",snapshot_max,snapshot_max);
}

void core_snapshot_read(char* buf, int len)
{
	int copy_len = len;
	//retro_log(RETRO_LOG_DEBUG,"core_snapshot_read(%p,%d) @ %X\n",buf,len,snapshot_pos);
	if ((snapshot_pos+len) > snapshot_size)
	{
		if (!snapshot_error)
		{
			retro_log(RETRO_LOG_ERROR,"core_snapshot_read out of bounds: %X + %d > %d\n",snapshot_pos,len,snapshot_size);
		}
		copy_len = snapshot_size - snapshot_pos;
		if (copy_len < 0) copy_len = 0;
		snapshot_error = true;
	}
	memcpy(buf,snapshot_buffer+snapshot_pos,copy_len);
	snapshot_pos += len;
	if (snapshot_pos > snapshot_max) snapshot_max = snapshot_pos;
}

void core_snapshot_write(const char* buf, int len)
{
	//retro_log(RETRO_LOG_DEBUG,"core_snapshot_write(%p,%d) @ %X\n",buf,len,snapshot_pos);
	if (snapshot_buffer)
	{
		int write_len = len;
		if ((snapshot_pos + len) > snapshot_size)
		{
			if (!snapshot_error)
			{
				retro_log(RETRO_LOG_ERROR,"core_snapshot_write: snapshot_size estimate too small (%d), data loss.\n",snapshot_size);
			}
			write_len = snapshot_size - snapshot_pos;
			if (write_len < 0) write_len = 0;
			snapshot_error = true;
		}
		memcpy(snapshot_buffer+snapshot_pos,buf,write_len);
	}
	snapshot_pos += len;
	if (snapshot_pos > snapshot_max) snapshot_max = snapshot_pos;
}

void core_snapshot_seek(int pos)
{
	//retro_log(RETRO_LOG_DEBUG,"core_snapshot_seek(%X)\n",pos);
	snapshot_pos = SNAPSHOT_HEADER_SIZE + pos;
	if (snapshot_pos > snapshot_max) snapshot_max = snapshot_pos;
}

static void snapshot_buffer_prepare(size_t size)
{
	if (size > snapshot_size)
	{
		retro_log(RETRO_LOG_ERROR,"serialize size %d > snapshot_size %d: enlarging.\n",size,snapshot_size);
		free(snapshot_buffer); snapshot_buffer = NULL;
		snapshot_size = size;
	}
	if (snapshot_buffer == NULL) snapshot_buffer = malloc(snapshot_size);
	memset(snapshot_buffer,0,snapshot_size);
}

//
// savestates
//

bool core_serialize_write = false; // serialization direction

static void core_serialize_internal(void* x, size_t size)
{
	if (core_serialize_write) core_snapshot_write(x,size);
	else                      core_snapshot_read( x,size);
}

void core_serialize_uint8(uint8_t *x) { core_serialize_internal(x,sizeof(uint8_t)); }
void core_serialize_int32(int32_t *x) { core_serialize_internal(x,sizeof(int32_t)); }
void core_serialize_uint32(uint32_t *x) { core_serialize_internal(x,sizeof(uint32_t)); }

static bool core_serialize(bool write)
{
	uint8_t bval;
	int32_t result = 0;
	core_serialize_write = write;
	core_snapshot_open_internal();

	// header (core data)

	// integrity
	result = SNAPSHOT_VERSION;
	core_serialize_int32(&result);
	if (result != SNAPSHOT_VERSION)
	{
		retro_log(RETRO_LOG_ERROR,"savestate version does not match SNAPSHOT_VERSION (%d != %d)\n",result,SNAPSHOT_VERSION);
		return false;
	}
	result = (int32_t)0x12345678UL;
	core_serialize_int32(&result);
	if (result != (int32_t)0x12345678UL)
	{
		retro_log(RETRO_LOG_ERROR,"savestate endian does not match system (%08X != %08X)\n",(uint32_t)result,0x12345678UL);
		return false;
	}
	bval = sizeof(void*);
	core_serialize_uint8(&bval);
	if (bval != sizeof(void*))
	{
		retro_log(RETRO_LOG_ERROR,"savessate memory address size does not match system (%d != %d)\n",result*8,sizeof(void*)*8);
		return false;
	}

	// core state
	core_serialize_uint8(&core_runflags);
	core_serialize_uint32(&midi_delta_time);
	core_input_serialize();
	core_osk_serialize();
	//retro_log(RETRO_LOG_DEBUG,"core_serialize header: %d <= %d\n",snapshot_pos,SNAPSHOT_HEADER_SIZE);
	if (snapshot_pos > SNAPSHOT_HEADER_SIZE)
		retro_log(RETRO_LOG_ERROR,"core_serialize header too large! %d > %d\n",snapshot_pos,SNAPSHOT_HEADER_SIZE);

	// suffix (hatari data)

	// hatari state
	result = 0;
	if (write) result = core_save_state();
	else       result = core_restore_state();

	// update core_disk to match changes to the inserted disks
	if (!write) core_disk_reindex();

	// finish

	if (write && snapshot_error)
	{
		retro_log(RETRO_LOG_ERROR,"core_serialize error: new size > original size? %d > %d\n",snapshot_max,snapshot_size);
		struct retro_message_ext msg;
		msg.msg = "Savestate size may have increased. Try ejecting floppy disks?";
		msg.duration = 5 * 1000;
		msg.priority = 3;
		msg.level = RETRO_LOG_ERROR;
		msg.target = RETRO_MESSAGE_TARGET_ALL;
		msg.type = RETRO_MESSAGE_TYPE_NOTIFICATION;
		msg.progress = -1;
		environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, &msg);		
	}

	retro_log(RETRO_LOG_DEBUG,"core_serialized: %d of %d used\n",snapshot_max,snapshot_size);
	if (result != 0)
	{
		snapshot_error = true;
		retro_log(RETRO_LOG_ERROR,"hatari state save/restore returned with error.\n");
	}
	return !snapshot_error;
}

//
// config update
//

void core_config_update(bool force)
{
	// skip update if not forced, or unneeded
	if (!force)
	{
		if (!environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE,&force) || !force) return;
	}
	retro_log(RETRO_LOG_INFO,"Applying configuration update.\n");
	core_config_apply();
}

//
// Libretro Exports
//

RETRO_API void retro_set_environment(retro_environment_t cb)
{
	environ_cb = cb;

	// if we initialize the log now, we can log during retro_set_environment,
	// but the log will become invalid after retro_init, and become unusable?
	#if DEBUG_RETRO_SET_ENVIRONMENT
		retro_log_init();
	#endif

	core_file_set_environment(cb);
	core_input_set_environment(cb);
	core_disk_set_environment(cb);
	core_config_set_environment(cb);
	core_midi_set_environment(cb);

	// M3U/M3U8 need fullpath to find the linked files
	{
		static const struct retro_system_content_info_override CONTENT_OVERRIDE[] = {
			{ "m3u|m3u8", true, false },
			{ NULL, false, false },
		};
		if (content_override_set || cb(RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE, (void*)CONTENT_OVERRIDE))
		{
			retro_log(RETRO_LOG_DEBUG,"SET_CONTENT_INFO_OVERRIDE requested need_fullpath\n");
			content_override_set = true; // seems to fail if called twice?
		}
		else
		{
			retro_log(RETRO_LOG_ERROR,"SET_CONTENT_INFO_OVERRIDE failed, M3U loading may be broken?\n");
		}
	}

	// allow boot with no disks
	cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, (void*)&BOOL_TRUE);

	// indicate serialization quirks
	cb(RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS, (void*)&QUIRKS);

	// future:
	//   RETRO_ENVIRONMENT_GET_MIDI_INTERFACE
}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb)
{
	video_cb = cb;
}

RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb)
{
	audio_sample_cb = cb;
}

RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
	audio_batch_cb = cb;
}

RETRO_API void retro_set_input_poll(retro_input_poll_t cb)
{
	input_poll_cb = cb;
}

RETRO_API void retro_set_input_state(retro_input_state_t cb)
{
	input_state_cb = cb;
}

RETRO_API void retro_init(void)
{
	const char* argv[1] = {""};
	retro_log_init();
	retro_log(RETRO_LOG_INFO,"retro_init()\n");

	// try to get the best pixel format we can
	{
		const char* PIXEL_FORMAT_NAMES[3] = { "0RGB1555", "XRGB8888", "RGB565" };
		core_pixel_format = 0;
		if      (environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, (void*)&RPF_XRGB8888)) core_pixel_format = 1;
		else if (environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, (void*)&RPF_RGB565  )) core_pixel_format = 2;
		else if (environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, (void*)&RPF_0RGB1555)) core_pixel_format = 0;
		retro_log(RETRO_LOG_INFO,"Pixel format: %s\n",PIXEL_FORMAT_NAMES[core_pixel_format]);
	}

	// initialize other modules
	core_input_init();
	core_disk_init();
	core_osk_init();
	midi_delta_time = 0;

	// for trace debugging (requires -DENABLE_TRACING=1)
	//Log_SetTraceOptions("cpu_disasm");
	//Log_SetTraceOptions("video_vbl,video_sync");

	core_runflags = 0;
	main_init(1,(char**)argv);

	// this will be fetched and applied via retro_get_system_av_info before the first frame begins
	core_video_fps = core_video_fps_new;
	core_audio_samplerate = core_audio_samplerate_new;
	core_video_changed = false;
	core_rate_changed = false;
	core_video_restore = false;

	core_audio_hold_remain = 0;
	core_audio_last[0] = 0;
	core_audio_last[1] = 0;
}

RETRO_API void retro_deinit(void)
{
	retro_log(RETRO_LOG_INFO,"retro_deinit()\n");

	m68k_go_quit();
	main_deinit();
}

RETRO_API unsigned retro_api_version(void)
{
	return RETRO_API_VERSION;
}

RETRO_API void retro_get_system_info(struct retro_system_info *info)
{
	retro_log(RETRO_LOG_INFO,"retro_get_system_info()\n");
	memset(info, 0, sizeof(*info));
	info->library_name = "hatariB";
	info->library_version = "v0.0 test " SHORTHASH " " __DATE__ " " __TIME__;
	info->valid_extensions = CORE_FILE_EXTENSIONS;
	info->need_fullpath = false;
	info->block_extract = false;
}

RETRO_API void retro_get_system_av_info(struct retro_system_av_info *info)
{
	retro_log(RETRO_LOG_INFO,"retro_get_system_av_info()\n");

	core_video_fps = core_video_fps_new;
	core_audio_samplerate = core_audio_samplerate_new;

	// update aspect ratio
	{
		double par = PIXEL_ASPECT_RATIO[core_video_aspect_mode][core_video_resolution];
		core_video_aspect = (par * core_video_w) / (double)core_video_h;
	}

	info->geometry.base_width = core_video_w;
	info->geometry.base_height = core_video_h;
	info->geometry.max_width = VIDEO_MAX_W;
	info->geometry.max_height = VIDEO_MAX_H;
	info->geometry.aspect_ratio = core_video_aspect;
	info->timing.fps = core_video_fps;
	info->timing.sample_rate = core_audio_samplerate;

	if (core_video_fps != 50 && core_video_fps != 60 && core_video_fps != 71)
	{
		retro_log(RETRO_LOG_ERROR,"Unexpected fps (%d), assuming 50\n",core_video_fps);
		info->timing.fps = 50;
	}

	retro_log(RETRO_LOG_INFO," geometry.base_width = %d\n",info->geometry.base_width);
	retro_log(RETRO_LOG_INFO," geometry.base_height = %d\n",info->geometry.base_height);
	retro_log(RETRO_LOG_INFO," geometry.aspect_radio = %f\n",info->geometry.aspect_ratio);
	retro_log(RETRO_LOG_INFO," timing.fps = %f\n",info->timing.fps);
	retro_log(RETRO_LOG_INFO," timing.sample_rate = %f\n",info->timing.sample_rate);
}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device)
{
	retro_log(RETRO_LOG_INFO,"retro_set_controller_port_device(%d,%d)\n",port,device);
}

RETRO_API void retro_reset(void)
{
	// core reset cancels OSK and pause
	core_osk_mode = CORE_OSK_OFF;
	core_runflags &= ~(CORE_RUNFLAG_PAUSE | CORE_RUNFLAG_OSK);
	// also generates a reset, cold if requested or needed because of halt
	if (core_runflags & CORE_RUNFLAG_HALT || !core_option_soft_reset) // halt always needs a cold reset
	{
		core_runflags &= ~CORE_RUNFLAG_HALT;
		Reset_Cold();
	}
	else
	{
		Reset_Warm();
	}
}

RETRO_API void retro_run(void)
{
	// undo overlay
	//   would have done this directly after video_cb,
	//   but RetroArch seems to display what is given at video_cb time only when running,
	//   but when in menus or paused (p) it displays the contents of the buffer at exit of retro_run instead?
	if (core_runflags & CORE_RUNFLAG_OSK)
	{
		core_osk_restore(core_video_buffer,core_video_w,core_video_h,core_video_pitch);
		core_video_restore = false;
	}

	// handle any pending configuration updates
	core_config_update(false);

	//retro_log(RETRO_LOG_DEBUG,"retro_run()\n");
	// poll input, generate event queue for hatari
	core_input_update();

	// input may have changed OSK state
	if (core_osk_mode)
	{
		core_runflags |= CORE_RUNFLAG_OSK;
		if (core_osk_mode == CORE_OSK_PAUSE || core_osk_mode == CORE_OSK_KEY_SHOT)
			core_runflags |= CORE_RUNFLAG_PAUSE;
	}
	else
	{
		core_runflags &= ~(CORE_RUNFLAG_PAUSE | CORE_RUNFLAG_OSK);
	}

	// process CPU reset by restarting the loop and UAE
	// can still happen while paused, as no CPU cycles are executed here, but screen dimensions etc. can be updated,
	// though this will result in a black screen until unpaused and allowed to render
	if (core_runflags & CORE_RUNFLAG_RESET)
	{
		core_config_reset(); // can apply boot parameters (e.g. CPU Freq)
		bool cold = core_runflags & CORE_RUNFLAG_RESET_COLD;
		core_signal_alert(cold ? "Cold Boot" : "Warm Boot");
		m68k_go_quit();
		UAE_Set_Quit_Reset(cold);
		core_runflags &= ~(CORE_RUNFLAG_RESET | CORE_RUNFLAG_HALT);
		m68k_go(true);
		core_init_return = true;
		m68k_go_frame();
		core_init_return = false;
		// STRam/STRam may hae been updated, send the new memory map
		retro_memory_maps();
		// cold reset ejects the disks
		if (cold)
			core_disk_drive_reinsert();
	}

	// force hatari to process the input queue before each frame starts
	core_input_post();

	// run one frame
	if (!(core_runflags & (CORE_RUNFLAG_HALT | CORE_RUNFLAG_PAUSE)))
	{
		m68k_go_frame();
		core_flush_audio();
	}
	else if (core_crashtime && ((core_runflags & (CORE_RUNFLAG_HALT | CORE_RUNFLAG_PAUSE)) == CORE_RUNFLAG_HALT))
	{
		// automatic reset timer after halt
		++core_crash_frames;
		core_debug_int("core_crash_frames: ",core_crash_frames);
		if ((core_crash_frames / core_video_fps) >= core_crashtime)
		{
			Reset_Cold();
			core_crash_frames = 0;
		}
	}

	// update video nature
	if (core_rate_changed)
	{
		retro_log(RETRO_LOG_INFO,"core_rate_changed\n");
		struct retro_system_av_info info;
		retro_get_system_av_info(&info);
		environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
		core_rate_changed = false;
		core_video_changed = false;
	}
	else if (core_video_changed)
	{
		retro_log(RETRO_LOG_INFO,"core_video_changed\n");
		struct retro_system_av_info info;
		retro_get_system_av_info(&info);
		environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &info);
		core_video_changed = false;
	}

	// draw overlay
	if (core_runflags & CORE_RUNFLAG_OSK)
	{
		core_osk_render(core_video_buffer,core_video_w,core_video_h,core_video_pitch);
		core_video_restore = true;
	}

	// send video
	video_cb(core_video_buffer,core_video_w,core_video_h,core_video_pitch);

	// fill audio if pause
	if (core_runflags & (CORE_RUNFLAG_PAUSE | CORE_RUNFLAG_HALT))
	{
		// how many new samples should we need?
		// use floating point to allow fractionals
		double sample_expect = ((double)core_audio_samplerate) / ((double)core_video_fps);
		double pending = (double)(core_audio_samples_pending / 2);
		if (pending < sample_expect)
			core_audio_hold_remain += (sample_expect - pending);
		int hold_samples = (int)core_audio_hold_remain;
		core_audio_hold_remain -= (double)hold_samples;
		core_audio_hold(hold_samples);
		//retro_log(RETRO_LOG_DEBUG,"audio hold: %d\n",hold_samples);
	}

	// send audio
	if (core_audio_samples_pending > 0)
	{
		unsigned int len = core_audio_samples_pending;
		if (len > AUDIO_BUFFER_LEN) len = AUDIO_BUFFER_LEN;
		audio_batch_cb(core_audio_buffer, len/2);
		//retro_log(RETRO_LOG_DEBUG,"audio_batch_cb(%p,%d)\n",core_audio_buffer,len/2);
		core_audio_samples_pending = 0;
		//retro_log(RETRO_LOG_DEBUG,"audio send: %d\n",len/2);
	}

	// event queue end of frame
	core_input_finish();

	// flush midi if needed
	core_midi_frame();
}

RETRO_API size_t retro_serialize_size(void)
{
	return snapshot_size;
}

RETRO_API bool retro_serialize(void *data, size_t size)
{
	//retro_log(RETRO_LOG_DEBUG,"retro_serialize(%p,%d)\n",data,size);
	snapshot_buffer_prepare(size);
	if (core_serialize(true))
	{
		// to test a broken savestate, corrupt its version string
		//++snapshot_buffer[SNAPSHOT_HEADER_SIZE+1];

		memcpy(data,snapshot_buffer,size);
		//core_debug_bin(data,size,0); // dump uncompressed contents to log
		//core_trace_next(20); // use to verify instructions after savestate are the same as after restore
		//core_write_file_save("hatarib_serialize_debug.bin",size,data); // for analyzing the uncompressed contents
		return true;
	}
	return false;
}

RETRO_API bool retro_unserialize(const void *data, size_t size)
{
	//retro_log(RETRO_LOG_DEBUG,"retro_unserialize(%p,%z)\n",data,size);
	//core_debug_bin(data,size,0);
	snapshot_buffer_prepare(size);
	memcpy(snapshot_buffer,data,size);
	if (core_serialize(false))
	{
		core_audio_samples_pending = 0; // clear all pending audio
		//core_trace_next(20); // verify instructions after savestate are the same as after restore
		return true;
	}
	return false;
}

RETRO_API void retro_cheat_reset(void)
{
	retro_log(RETRO_LOG_DEBUG,"retro_cheat_reset()\n");
}

RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
	retro_log(RETRO_LOG_DEBUG,"retro_cheat_set(%d,%d,'%s')\n",index,enabled,code?code:"(NULL)");
	// no cheat mechanism available
}

RETRO_API bool retro_load_game(const struct retro_game_info *game)
{
	retro_log(RETRO_LOG_INFO,"retro_load_game(%s)\n",game?"game":"NULL");

	if (game)
		core_disk_load_game(game);

	// finish initialization of the CPU
	core_init_return = true;
	m68k_go_frame();
	core_init_return = false;

	// create a dummy savestate to measure the needed snapshot size
	free(snapshot_buffer); snapshot_buffer = NULL;
	core_serialize(true);
	snapshot_size = snapshot_max + SNAPSHOT_OVERHEAD;
	// if we've got more than 1MB RAM we should increase our minimum to accomodate it
	int minimum = SNAPSHOT_MINIMUM;
	if (STRamEnd > (1024*1024))
		minimum += (STRamEnd - (1024*1024));
	if (snapshot_size < minimum)
		snapshot_size = minimum;
	// round up
	if (snapshot_size % SNAPSHOT_ROUND)
		snapshot_size += (SNAPSHOT_ROUND - (snapshot_size % SNAPSHOT_ROUND));

	retro_memory_maps();

	return true;
}

RETRO_API bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
	return false;
}

RETRO_API void retro_unload_game(void)
{
	retro_log(RETRO_LOG_DEBUG,"retro_unload_game()\n");
	core_disk_unload_game(); // chance to save
}

RETRO_API unsigned retro_get_region(void)
{
	// Using the TOS language for this (see tos.h).
	// 0 is US, -1 is unknown, 127+ is "all", and I think all the other region enums are in europe?
	int lang = TOS_DefaultLanguage();
	if (lang <= 0 || lang >= 127) return RETRO_REGION_NTSC;
	return RETRO_REGION_PAL;
}

RETRO_API void* retro_get_memory_data(unsigned id)
{
	// This interface seems to be for automatically creating save files,
	// but this core should save to specially named floppy files image instead.
	return NULL;
}

RETRO_API size_t retro_get_memory_size(unsigned id)
{
	return 0;
}
