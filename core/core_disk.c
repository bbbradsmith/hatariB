// manage disks

#include "../libretro/libretro.h"
#include "core.h"
#include "core_internal.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_DISKS 32
#define MAX_FILENAME 256

struct disk_image
{
	uint8_t* data;
	unsigned int size;
	char filename[MAX_FILENAME];
	bool modified;
};

bool core_disk_enable_b = true;

static bool first_init = true;
static struct disk_image disks[MAX_DISKS];
static int drive;
static unsigned int image_index[2];
static bool image_insert[2];
static unsigned int image_count;
static int initial_image;

//
// Hatari interface
//

// floppy.c
bool hatari_libretro_floppy_insert(int drive, const char* filename, void* data, unsigned int size);
void hatari_libretro_floppy_eject(int drive);

//
// Utilities
//

// safe truncating strcpy/strcat
static void strcpy_trunc(char* dest, const char* src, unsigned int len)
{
	strncpy(dest,src,len);
	dest[len-1] = 0;
}
static void strcat_trunc(char* dest, const char* src, unsigned int len)
{
	strncat(dest,src,len);
	dest[len-1] = 0;
}

void disks_clear()
{
	drive = 0;
	for (int i=0; i < MAX_DISKS; ++i)
		free(disks[i].data);
	memset(disks,0,sizeof(disks));
	for (int i=0; i< 2; ++i)
	{
		image_index[i] = 0;
		image_insert[i] = 0;
	}
	image_count = 0;
	// Hatari ejects both disks
	hatari_libretro_floppy_eject(0);
	hatari_libretro_floppy_eject(1);
}

//
// Retro Disk Control Interface
//

static bool set_eject_state_drive(bool ejected, int d)
{
	int o = d ^ 1; // other drive
	if (ejected)
	{
		// eject if not already ejected
		if (image_insert[d])
		{
			image_insert[d] = false;
			hatari_libretro_floppy_eject(d); // Hatari eject
			// TODO if ejected, save the disk if modified
		}
		return true;
	}

	// already inserted
	if (image_insert[d]) return true;

	// not inserted, but already in other drive, eject it first
	if (image_insert[o] && (image_index[o] == image_index[d]))
	{
		image_insert[o] = false;
		hatari_libretro_floppy_eject(o); // Hatari eject
	}

	// fail if no disk
	if (image_index[d] >= MAX_DISKS) return false;

	// fail if disk is not loaded
	if (disks[image_index[d]].data == NULL) return false;

	// now ready to insert
	if (!hatari_libretro_floppy_insert(d, disks[image_index[d]].filename, disks[image_index[d]].data, disks[image_index[d]].size))
	{
		static char floppy_error_msg[MAX_FILENAME+256];
		struct retro_message_ext msg;
		snprintf(floppy_error_msg, sizeof(floppy_error_msg), "%c: disk failure: %s",d?'B':'A',disks[image_index[d]].filename);
		msg.msg = floppy_error_msg;
		msg.duration = 5 * 1000;
		msg.priority = 3;
		msg.level = RETRO_LOG_ERROR;
		msg.target = RETRO_MESSAGE_TARGET_ALL;
		msg.type = RETRO_MESSAGE_TYPE_NOTIFICATION;
		msg.progress = -1;
		environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, &msg);
		return false;
	}
	image_insert[d] = true;
	return true;
}

static bool set_eject_state(bool ejected)
{
	retro_log(RETRO_LOG_DEBUG,"set_eject_state(%d)\n",ejected);
	return set_eject_state_drive(ejected, drive);
}

static bool get_eject_state(void)
{
	//retro_log(RETRO_LOG_DEBUG,"get_eject_state()\n");
	return !image_insert[drive];
}

static unsigned get_image_index(void)
{
	//retro_log(RETRO_LOG_DEBUG,"get_image_index()\n");
	return image_index[drive];
}

static bool set_image_index(unsigned index)
{
	retro_log(RETRO_LOG_DEBUG,"set_image_index(%d)\n",index);

	// no change
	if (index == image_index[drive]) return true;

	// eject first if neeed
	if (image_insert[drive])
	{
		set_eject_state(true);
	}

	image_index[drive] = index;
	return true;
}

static unsigned get_num_images(void)
{
	//retro_log(RETRO_LOG_DEBUG,"get_num_images()\n");
	return image_count;
}

static bool replace_image_index(unsigned index, const struct retro_game_info* game)
{
	const char* path = NULL;
	struct retro_game_info_ext* game_ext = NULL;

	retro_log(RETRO_LOG_DEBUG,"replace_image_index(%d,%p)\n",index,game);
	if (index >= image_count) return false;
	if (game == NULL) return false;
	
	retro_log(RETRO_LOG_INFO,"retro_game_info:\n");
	retro_log(RETRO_LOG_INFO," path: %s\n",game->path ? game->path : "(NULL)");
	retro_log(RETRO_LOG_INFO," data: %p\n",game->data);
	retro_log(RETRO_LOG_INFO," size: %d\n",(int)game->size);
	retro_log(RETRO_LOG_INFO," meta: %s\n",game->meta ? game->meta : "(NULL)");
	if (environ_cb(RETRO_ENVIRONMENT_GET_GAME_INFO_EXT, &game_ext) && game_ext)
	{
		retro_log(RETRO_LOG_INFO,"retro_game_info_ext:\n");
		retro_log(RETRO_LOG_INFO," full_path: %s\n",game_ext->full_path ? game_ext->full_path : "(NULL)");
		retro_log(RETRO_LOG_INFO," archive_path: %s\n",game_ext->archive_path ? game_ext->archive_path : "(NULL)");
		retro_log(RETRO_LOG_INFO," archive_file: %s\n",game_ext->archive_file ? game_ext->archive_file : "(NULL)");
		retro_log(RETRO_LOG_INFO," dir: %s\n",game_ext->dir ? game_ext->dir : "(NULL)");
		retro_log(RETRO_LOG_INFO," name: %s\n",game_ext->name ? game_ext->name : "(NULL)");
		retro_log(RETRO_LOG_INFO," ext: %s\n",game_ext->ext ? game_ext->ext : "(NULL)");
		retro_log(RETRO_LOG_INFO," meta: %s\n",game_ext->ext ? game_ext->meta : "(NULL)");
		retro_log(RETRO_LOG_INFO," data: %p\n",game_ext->data);
		retro_log(RETRO_LOG_INFO," size: %d\n",(int)game_ext->size);
		retro_log(RETRO_LOG_INFO," file_in_archive: %d\n",(int)game_ext->file_in_archive);
		retro_log(RETRO_LOG_INFO," persistent_data: %d\n",(int)game_ext->persistent_data);
	}
	else game_ext = NULL;
	
	path = game->path;
	// find base filename
	{
		int e = strlen(path);
		for(;e>0;--e)
		{
			if(path[e-1] == '/' || path[e-1] == '\\') break;
		}
		path += e;
	}

	// eject if currently inserted
	if (image_insert[0] && (image_index[0] == index)) set_eject_state_drive(false,0);
	if (image_insert[1] && (image_index[1] == index)) set_eject_state_drive(false,0);

// TODO load saves instead?

// TODO
// Load disk file actually bypasses the need_fullpath setting
// so if we get data=0 here, we might still have a path we can fall back on and use the vfs to read it
// TODO also
// look for save file to replace if allowed
// (3 settings: save floppies, don't use saves, write protect)

	free(disks[index].data);
	disks[index].modified = false;
	disks[index].size = 0;

	if (game->data)
	{
		disks[index].data = malloc(game->size);
		if (disks[index].data == NULL)
		{
			strcpy(disks[index].filename,"<Out Of Memory>");
			return false;
		}
		disks[index].size = game->size;
		memcpy(disks[index].data,game->data,game->size);
	}
	else
	{
		// Load New Disk does not obey need_fullpath, so we may need to manually load the file
		FILE* f = fopen(game->path,"rb");
		if (f != NULL)
		{
			fseek(f,0,SEEK_END);
			unsigned int size = (unsigned int)ftell(f);
			fseek(f,0,SEEK_SET);
			disks[index].data = malloc(size);
			if (disks[index].data != NULL)
			{
				fread(disks[index].data,1,size,f);
				disks[index].size = size;
			}
			fclose(f);
		}
	}

	if (path == NULL)
	{
		sprintf(disks[index].filename,"<Image %d>",index);
	}
	else
	{
		strcpy_trunc(disks[index].filename,path,MAX_FILENAME);
		// TODO make sure the extension is always present? Hatari uses the ext to figure out what to do
	}
	return true;

	// TODO if m3u, load sequence of files instead
	// TODO if zip, load files in alphabetical order instead
	// if invalid... I guess just keep with empty data and call it [Invalid M3U] etc.
	// see example: https://github.com/libretro/beetle-psx-libretro/blob/379793f1005b1d8810b99f81fe7b5f9126831c89/libretro.cpp#L4024C13-L4024C20
	// we may need to set an override for m3u to give full path? or does this permit what is needed thru vfn? check what beetle is doing
}

static bool add_image_index(void)
{
	retro_log(RETRO_LOG_DEBUG,"add_image_index()\n");
	if (image_count < MAX_DISKS)
	{
		++image_count;
		return true;
	}
	return false;
}

static bool set_initial_image(unsigned index, const char* path)
{
	retro_log(RETRO_LOG_DEBUG,"set_initial_image(%d,%p)\n",index,path);
	if(path) retro_log(RETRO_LOG_DEBUG,"path: %s\n",path);
	initial_image = index;
	return true;
}

static bool get_image_path(unsigned index, char* path, size_t len)
{
	retro_log(RETRO_LOG_DEBUG,"get_image_path(%d,%p,%d)\n",index,path,(int)len);
	if (index >= MAX_DISKS) return false;
	if (path && len>0)
	{
		strcpy_trunc(path,disks[index].filename,len);
	}
	return true;
}

static bool get_image_label(unsigned index, char* label, size_t len)
{
	retro_log(RETRO_LOG_DEBUG,"get_image_label(%d,%p,%d)\n",index,label,(int)len);
	if (index >= MAX_DISKS) return false;
	if (label == NULL || len == 0) return true;
	// prefix to indicate insertion
	const char* prefix = "";
	if      (image_index[0] == index && image_insert[0]) prefix = "[A:] ";
	else if (image_index[1] == index && image_insert[1]) prefix = "[B:] ";
	strcpy_trunc(label,prefix,len);
	strcat_trunc(label,disks[index].filename,len);
	return true;
}

//
// core_disk stuff
//

void core_disk_set_environment(retro_environment_t cb)
{
	static const struct retro_disk_control_callback retro_disk_control =
	{
		set_eject_state,
		get_eject_state,
		get_image_index,
		set_image_index,
		get_num_images,
		replace_image_index,
		add_image_index,
	};

	static const struct retro_disk_control_ext_callback retro_disk_control_ext =
	{
		set_eject_state,
		get_eject_state,
		get_image_index,
		set_image_index,
		get_num_images,
		replace_image_index,
		add_image_index,
		set_initial_image,
		get_image_path,
		get_image_label,
	};

	unsigned version = 0;
	if (cb(RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION, &version) && (version >= 1))
		cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE, (void*)&retro_disk_control_ext);
	else
		cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE, (void*)&retro_disk_control);

	//if (cv(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, 
}

void core_disk_init(void)
{
	if (first_init)
	{
		memset(disks,0,sizeof(disks));
		first_init = false;
	}
	disks_clear();
	initial_image = 0;
}

void core_disk_load_game(const struct retro_game_info *game)
{
	retro_log(RETRO_LOG_DEBUG,"core_disk_load_game(%p)\n",game);

	disks_clear();
	if (game == NULL) return;
	add_image_index(); // add one disk
	replace_image_index(0,game); // load it there (may load multiple if M3U/Zip)
	if (initial_image >= image_count) initial_image = 0;
	set_image_index(initial_image);
	set_eject_state(false); // insert it

	// insert second disk if available
	if (core_disk_enable_b && image_count > 1)
	{
		int second_image = initial_image + 1;
		if (second_image >= image_count) second_image = 0;
		drive = 1;
		set_image_index(second_image);
		set_eject_state(false);
		drive = 0;
	}
}

void core_disk_unload_game(void)
{
	// eject both disks to save them
	drive = 0;
	set_eject_state(true);
	drive = 1;
	set_eject_state(true);
	disks_clear();
}

void core_disk_serialize(void)
{
	// TODO is there anything to serialize? do we want to track disk changes?
	// I think after a load, we want to update the inserted state to match hatari's,
	// and match two two indices to our image filenames (set to invalid index if no match).
	// If there was no match and we eject later a modified disk, there will be an error notification that it could not save?
	// ...or maybe we can just save using the hatari filename anyway, despite not having it in the disks list. That'd probably be fine.
}

void core_disk_drive_toggle(void)
{
	drive = drive ^ 1;
	// TODO if no B drive, drive = 0

	struct retro_message_ext msg;
	msg.msg = drive ? "Drive B: selected" : "Drive A: selected";
	msg.duration = 3 * 1000;
	msg.priority = 1;
	msg.level = RETRO_LOG_INFO;
	msg.target = RETRO_MESSAGE_TARGET_ALL;
	msg.type = RETRO_MESSAGE_TYPE_NOTIFICATION;
	msg.progress = -1;
	environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, &msg);
}
