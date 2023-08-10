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
	// primary file
	uint8_t* data;
	unsigned int size;
	char filename[MAX_FILENAME];
	// secondary file (used for STX save)
	uint8_t* extra_data;
	unsigned int extra_size;
	char extra_filename[MAX_FILENAME];
	// whether the file has a save
	bool saved;
};

bool core_disk_enable_b = true;
bool core_disk_enable_save = true;

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
extern bool hatari_libretro_floppy_insert(int drive, const char* filename, void* data, unsigned int size, void* extra_data, unsigned int extra_size);
extern void hatari_libretro_floppy_eject(int drive);
extern const char* hatari_libretro_floppy_inserted(int drive);
extern void hatari_libretro_floppy_changed(int drive);
//
// Utilities
//

void disks_clear()
{
	drive = 0;
	for (int i=0; i < MAX_DISKS; ++i)
	{
		free(disks[i].data);
		free(disks[i].extra_data);
	}
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
	retro_log(RETRO_LOG_DEBUG,"set_eject_state_drive(%d,%d)\n",ejected,drive);

	if (ejected)
	{
		// eject if not already ejected
		if (image_insert[d])
		{
			image_insert[d] = false;
			hatari_libretro_floppy_eject(d); // Hatari eject
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
	if (!hatari_libretro_floppy_insert(d, disks[image_index[d]].filename,
		disks[image_index[d]].data, disks[image_index[d]].size,
		disks[image_index[d]].extra_data, disks[image_index[d]].extra_size))
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
	retro_log(RETRO_LOG_DEBUG,"set_eject_state(%d) drive %d\n",ejected,drive);
	return set_eject_state_drive(ejected, drive);
}

static bool get_eject_state(void)
{
	//retro_log(RETRO_LOG_DEBUG,"get_eject_state() drive %d\n",drive);
	return !image_insert[drive];
}

static unsigned get_image_index(void)
{
	//retro_log(RETRO_LOG_DEBUG,"get_image_index() drive %d\n",drive);
	return image_index[drive];
}

static bool set_image_index(unsigned index)
{
	retro_log(RETRO_LOG_DEBUG,"set_image_index(%d) drive %d\n",index,drive);

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
	const char* ext = NULL;
	//struct retro_game_info_ext* game_ext = NULL;

	retro_log(RETRO_LOG_DEBUG,"replace_image_index(%d,%p)\n",index,game);
	if (index >= image_count) return false;
	if (game == NULL) return false;
	
	retro_log(RETRO_LOG_INFO,"retro_game_info:\n");
	retro_log(RETRO_LOG_INFO," path: %s\n",game->path ? game->path : "(NULL)");
	retro_log(RETRO_LOG_INFO," data: %p\n",game->data);
	retro_log(RETRO_LOG_INFO," size: %d\n",(int)game->size);
	retro_log(RETRO_LOG_INFO," meta: %s\n",game->meta ? game->meta : "(NULL)");
	//
	// The ext info is actually not very useful.
	// It only works during retro_load_game and not in response to Load New Disk.
	// There isn't any new information that I need from it.
	//
	//if (environ_cb(RETRO_ENVIRONMENT_GET_GAME_INFO_EXT, &game_ext) && game_ext)
	//{
	//	retro_log(RETRO_LOG_INFO,"retro_game_info_ext:\n");
	//	retro_log(RETRO_LOG_INFO," full_path: %s\n",game_ext->full_path ? game_ext->full_path : "(NULL)");
	//	retro_log(RETRO_LOG_INFO," archive_path: %s\n",game_ext->archive_path ? game_ext->archive_path : "(NULL)");
	//	retro_log(RETRO_LOG_INFO," archive_file: %s\n",game_ext->archive_file ? game_ext->archive_file : "(NULL)");
	//	retro_log(RETRO_LOG_INFO," dir: %s\n",game_ext->dir ? game_ext->dir : "(NULL)");
	//	retro_log(RETRO_LOG_INFO," name: %s\n",game_ext->name ? game_ext->name : "(NULL)");
	//	retro_log(RETRO_LOG_INFO," ext: %s\n",game_ext->ext ? game_ext->ext : "(NULL)");
	//	retro_log(RETRO_LOG_INFO," meta: %s\n",game_ext->ext ? game_ext->meta : "(NULL)");
	//	retro_log(RETRO_LOG_INFO," data: %p\n",game_ext->data);
	//	retro_log(RETRO_LOG_INFO," size: %d\n",(int)game_ext->size);
	//	retro_log(RETRO_LOG_INFO," file_in_archive: %d\n",(int)game_ext->file_in_archive);
	//	retro_log(RETRO_LOG_INFO," persistent_data: %d\n",(int)game_ext->persistent_data);
	//}
	//else game_ext = NULL;
	
	path = game->path;
	ext = NULL;
	// find base filename and extension
	if (path)
	{
		int e = strlen(path);
		for(;e>0;--e)
		{
			if(path[e-1] == '.' && ext == NULL) ext = path + e;
			if(path[e-1] == '/' || path[e-1] == '\\') break;
		}
		path += e;
	}

	// eject if currently inserted
	if (image_insert[0] && (image_index[0] == index)) set_eject_state_drive(false,0);
	if (image_insert[1] && (image_index[1] == index)) set_eject_state_drive(false,0);

	free(disks[index].data);
	free(disks[index].extra_data);
	disks[index].data = NULL;
	disks[index].extra_data = NULL;
	disks[index].size = 0;
	disks[index].extra_size = 0;
	disks[index].saved = false;

	if (core_disk_enable_save && path)
	{
		unsigned int save_size = 0;
		uint8_t* save_data = core_read_file_save(path,&save_size);
		if (save_data != NULL)
		{
			retro_log(RETRO_LOG_INFO,"Disk image replaced with save: %s (%d bytes)\n",path,save_size);
			disks[index].data = save_data;
			disks[index].size = save_size;
			disks[index].saved = true;
		}
		if (ext && !strcasecmp(ext,"stx")) // STX has a secondary save file used as an overlay
		{
			strcpy_trunc(disks[index].extra_filename,path,MAX_FILENAME);
			int l = strlen(disks[index].extra_filename);
			if (l >= 3) disks[index].extra_filename[l-3] = 0;
			strcat_trunc(disks[index].extra_filename,"wd1772",MAX_FILENAME);
			unsigned int extra_size = 0;
			uint8_t* extra_data = core_read_file_save(disks[index].extra_filename,&extra_size);
			if (extra_data != NULL)
			{
				retro_log(RETRO_LOG_INFO,"STX WD1772 overlay save found: %s (%d bytes)\n",disks[index].extra_filename,extra_size);
				disks[index].extra_data = extra_data;
				disks[index].extra_size = extra_size;
				disks[index].saved = true;
			}
		}
	}

	if (disks[index].data == NULL)
	{
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
			// Load New Disk does not obey need_fullpath, so we may need to manually load the file.
			// This also takes care of fetching file links from an M3U list.
			disks[index].data = core_read_file(game->path,&disks[index].size);
		}
	}

	// TODO m3u/m3u8 or zip should parse the file and recursively call this function,
	// first replacing this one, but then adding new ones. Zip should use alphabetical sort?
	// If invalid keep with empty data and call it <Invalid M3U> etc.?
	// see example: https://github.com/libretro/beetle-psx-libretro/blob/379793f1005b1d8810b99f81fe7b5f9126831c89/libretro.cpp#L4024C13-L4024C20

	if (path == NULL)
	{
		// Without a path we have to guess an extension, and Hatari will likely fail to load it.
		sprintf(disks[index].filename,"<Image %d>.st",index);
	}
	else
	{
		strcpy_trunc(disks[index].filename,path,MAX_FILENAME);
	}
	return true;
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

void core_disk_reindex(void)
{
	// after loading a savesate, remap the loaded disks to the ones we have in core_disk
	for (int d=0; d<2; ++d)
	{
		const char* infile = hatari_libretro_floppy_inserted(d);
		if (!infile)
		{
			image_insert[d] = false;
		}
		else // match to an existing disk image
		{
			int i;
			image_insert[d] = true; // update insertion state
			i=0;
			for (; i<MAX_DISKS; ++i)
			{
				if (!strcmp(disks[i].filename,infile)) break;
			}
			if (i < MAX_DISKS)
			{
				image_index[d] = i;
				if (disks[i].saved)
				{
					// Reloading a savesate may have reverted changes to the disk.
					// If it was never saved, then there would be no changes,
					// but if we've seen this disk load a save or receive one,
					// there's a possibility a savesate will be reverting data,
					// which will need to be saved again. In the case that the data
					// is the same, it seems okay to request a redundant save,
					// since we've already established a valid save file exists.
					// We mark this drive's contents as changed so it will be saved
					// at the next eject/close.
					hatari_libretro_floppy_changed(d);
					retro_log(RETRO_LOG_DEBUG,"Savestate marks floppy contents changed: %s\n",disks[i].filename);
				}
			}
			else
			{
				image_index[d] = MAX_DISKS; // set to invalid index
				retro_log(RETRO_LOG_ERROR,"core_disk_serialize disk in drive %d not cached: '%s'\n",d,infile);
				// do a save test for possible modifications
				if (core_disk_save_exists(infile)) // note this doesn't work for STX because it wants an overlay instead
				{
					hatari_libretro_floppy_changed(d);
					retro_log(RETRO_LOG_DEBUG,"Savestate marks uncached floppy contents changed: %s\n",infile);
				}
			}
		}
	}
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

void core_disk_drive_reinsert(void)
{
	int restore_drive = drive;
	bool restore_eject;
	retro_log(RETRO_LOG_DEBUG,"core_disk_drive_reinsert()\n");
	drive = 0;
	restore_eject = get_eject_state();
	set_eject_state(true);
	set_eject_state(restore_eject);
	if (core_disk_enable_b)
	{
		drive = 1;
		restore_eject = get_eject_state();
		set_eject_state(true);
		set_eject_state(restore_eject);
	}
	drive = restore_drive;
}

//
// saving
//

// standard save:
//   saves file to saves/ if enabled
//   replaces the disk cache with the new data for the remainder of the session
//   for efficiency, core_disk_save can be given owndership of the data pointer passed.
bool core_disk_save(const char* filename, uint8_t* data, unsigned int size, bool core_owns_data)
{
	int i;
	retro_log(RETRO_LOG_DEBUG,"core_disk_save('%s',%p,%d,%d)\n",filename,data,size,(int)core_owns_data);
	if (data == NULL)
	{
		retro_log(RETRO_LOG_ERROR,"disk save data null?\n");
		return false;
	}

	// write to disk if allowed
	bool result = true;
	if (core_disk_enable_save)
	{
		result = core_write_file_save(filename, size, data);
	}

	i = 0;
	for (; i<MAX_DISKS; ++i)
	{
		if (!strcmp(disks[i].filename,filename)) break;
	}
	if (i < MAX_DISKS)
	{
		disks[i].saved = true;
		if (!core_owns_data)
		{
			if (disks[i].data && size <= disks[i].size) // same size or smaller, just copy it over
			{
				retro_log(RETRO_LOG_DEBUG,"disk cache overwritten: %s\n",filename);
				memcpy(disks[i].data, data, size);
				disks[i].size = size;
			}
			else // need to reallocate and copy
			{
				free(disks[i].data);
				disks[i].data = malloc(size);
				if (disks[i].data == NULL)
				{
					retro_log(RETRO_LOG_ERROR,"disk cache out of memory: %s\n",filename);
					strcpy(disks[i].filename,"<Out Of Memory>");
					disks[i].size = 0;
					result = false;
				}
				else
				{
					retro_log(RETRO_LOG_DEBUG,"disk cache reallocated: %s\n",filename);
					memcpy(disks[i].data, data, size);
					disks[i].size = size;
				}
			}
		}
		else
		{
			retro_log(RETRO_LOG_DEBUG,"disk cache transfered: %s\n",filename);
			free(disks[i].data);
			disks[i].data = data;
			disks[i].size = size;
			data = NULL; // take ownership
		}
	}
	else // not found
	{
		retro_log(RETRO_LOG_INFO,"core_disk_save for uncached disk: %s\n",filename);		
	}

	// free data if ownership was given but transferred
	if (core_owns_data)
	{
		free(data);
	}
	return result;
}

//
// more advanced save file creation
// opens one file and allows serial writes
// on close, will delete itself if failure is indicated
// otherwise can update the extra_data cache
//

void* disk_save_advanced_file = NULL;
char disk_save_advanced_path[2048] = "";
char disk_save_advanced_filename[MAX_FILENAME] = "";

// buffer to 
uint8_t* disk_save_advanced_buffer = NULL;
unsigned int disk_save_advanced_buffer_size = 0;
unsigned int disk_save_advanced_buffer_pos = 0;
// use 2MB
#define ADVBUFFER_SIZE   (2*1024*1024)

void* core_disk_save_open(const char* filename)
{
	free(disk_save_advanced_buffer);
	disk_save_advanced_buffer = malloc(ADVBUFFER_SIZE);
	disk_save_advanced_buffer_size = ADVBUFFER_SIZE;

	// open file and remember it for closing time
	void* handle = core_file_open_save(filename,CORE_FILE_WRITE);
	disk_save_advanced_file = handle;
	strcpy_trunc(disk_save_advanced_path,get_temp_fn(),sizeof(disk_save_advanced_path));
	strcpy_trunc(disk_save_advanced_filename,filename,sizeof(disk_save_advanced_filename));
	retro_log(RETRO_LOG_DEBUG,"core_disk_save_open('%s') = %p\n",filename,handle);
	return handle;
}

void core_disk_save_close_extra(void* file, bool success)
{
	core_file_close(file);
	if (file != disk_save_advanced_file)
	{
		// this shouldn't happen
		retro_log(RETRO_LOG_ERROR,"core_disk_save_close_extra with changed file? (%p != %p)\n",file,disk_save_advanced_file);
	}
	else if (!success)
	{
	// file closed but it was a failure, delete it
		retro_log(RETRO_LOG_ERROR,"core_disk_save_close_extra failure, deleting: '%s'\n",disk_save_advanced_path);
		if (0 != core_file_remove(disk_save_advanced_path))
			retro_log(RETRO_LOG_ERROR,"failed to delete: '%s'\n",disk_save_advanced_path);
	}
	else // success
	{
		// if it's a cached disk, update it
		int i=0;
		for (; i<MAX_DISKS; ++i)
		{
			if (!strcmp(disks[i].extra_filename,disk_save_advanced_filename)) break;
		}
		if (i < MAX_DISKS)
		{
			if (disk_save_advanced_buffer)
			{
				free(disks[i].extra_data);
				disks[i].extra_data = disk_save_advanced_buffer;
				disks[i].extra_size = disk_save_advanced_buffer_pos;
				disks[i].saved = true;
				disk_save_advanced_buffer = NULL; // disks owns the pointer now
				retro_log(RETRO_LOG_DEBUG,"disk cache transfered: %s\n",disk_save_advanced_filename);
			}
			else // if disk_save_advanced_buffer failed we can still try to read it back from the file
			{
				unsigned int size;
				uint8_t* data;
				retro_log(RETRO_LOG_WARN,"core_disk_save_close_extra failed to cache, attempting reload: %s\n",disk_save_advanced_filename);
				data = core_read_file_save(disk_save_advanced_filename,&size);
				if (data)
				{
					free(disks[i].extra_data);
					disks[i].extra_data = data;
					disks[i].extra_size = size;
					disks[i].saved = true;
					retro_log(RETRO_LOG_DEBUG,"disk cache read back: %s\n",disk_save_advanced_filename);
				}
				else
				{
					retro_log(RETRO_LOG_ERROR,"core_disk_save_close_extra failed to reload from file: %s\n",disk_save_advanced_filename);
				}
			}
		}
	}
	free(disk_save_advanced_buffer);
	disk_save_advanced_buffer = NULL;
}

bool core_disk_save_write(const uint8_t* data, unsigned int size, void* file)
{
	// store the data to give to the cache later
	if (disk_save_advanced_buffer)
	{
		// grow the buffer if needed
		if ((disk_save_advanced_buffer_pos + size) > disk_save_advanced_buffer_size)
		{
			uint8_t* r = realloc(disk_save_advanced_buffer, disk_save_advanced_buffer_size * 2);
			if (r == NULL) // if realloc fails, it does not free the original pointer
			{
				free(disk_save_advanced_buffer); // free it and abandon the attempt to cache it
				disk_save_advanced_buffer = NULL;
			}
			else
			{
				disk_save_advanced_buffer = r;
				disk_save_advanced_buffer_size *= 2;
			}
		}
		// if still valid, add the new data
		if (disk_save_advanced_buffer)
		{
			memcpy(disk_save_advanced_buffer+disk_save_advanced_buffer_pos,data,size);
			disk_save_advanced_buffer_pos += size;
		}
	}

	if (file != disk_save_advanced_file)
		retro_log(RETRO_LOG_ERROR,"core_disk_save_write with changed file? (%p != %p)\n",file,disk_save_advanced_file);
	return (size == core_file_write(data, size, file));
}

bool core_disk_save_exists(const char* filename)
{
	return core_file_save_exists(filename);
}
