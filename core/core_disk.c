// manage disks

#include "../libretro/libretro.h"
#include "core.h"
#include "core_internal.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define HAVE_ZLIB_H
#include "../hatari/src/includes/unzip.h"

#define MAX_DISKS 32

struct disk_image
{
	// primary file
	uint8_t* data;
	unsigned int size;
	char filename[CORE_MAX_FILENAME];
	// secondary file (used for STX save)
	uint8_t* extra_data;
	unsigned int extra_size;
	char extra_filename[CORE_MAX_FILENAME];
	// whether the file has a save
	bool saved;
};

bool core_disk_enable_b = true;
bool core_disk_enable_save = true;
bool core_savestate_floppy_modify = true;

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
extern bool core_floppy_insert(int drive, const char* filename, void* data, unsigned int size, void* extra_data, unsigned int extra_size);
extern void core_floppy_eject(int drive);
extern const char* core_floppy_inserted(int drive);
extern void core_floppy_changed(int drive);
// options.c
extern void core_auto_start(const char* path);

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
	core_floppy_eject(0);
	core_floppy_eject(1);
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
			core_floppy_eject(d); // Hatari eject
		}
		return true;
	}

	// already inserted
	if (image_insert[d]) return true;

	// not inserted, but already in other drive, eject it first
	if (image_insert[o] && (image_index[o] == image_index[d]))
	{
		image_insert[o] = false;
		core_floppy_eject(o); // Hatari eject
	}

	// fail if no disk
	if (image_index[d] >= MAX_DISKS) return false;

	// fail if disk is not loaded
	if (disks[image_index[d]].data == NULL) return false;

	// now ready to insert
	if (!core_floppy_insert(d, disks[image_index[d]].filename,
		disks[image_index[d]].data, disks[image_index[d]].size,
		disks[image_index[d]].extra_data, disks[image_index[d]].extra_size))
	{
		static char floppy_error_msg[CORE_MAX_FILENAME+256];
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
	//retro_log(RETRO_LOG_DEBUG,"set_eject_state(%d) drive %d\n",ejected,drive);
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

unsigned get_num_images(void)
{
	//retro_log(RETRO_LOG_DEBUG,"get_num_images()\n");
	return image_count;
}

//
// file loaders
//

static bool add_image_index(void);
static bool replace_image_index(unsigned index, const struct retro_game_info* game);

static bool load_m3u(uint8_t* data, unsigned int size, const char* m3u_path, unsigned first_index)
{
	static char path[2048] = "";
	static char link[2048] = "";
	static char line[2048] = "";

	// remove data from disks (take ownership of *data)
	strcpy(disks[first_index].filename,"<m3u>");
	disks[first_index].data = NULL;
	disks[first_index].size = 0;
	disks[first_index].saved = false;

	retro_log(RETRO_LOG_INFO,"load_m3u(%d,'%s',%d)\n",size,m3u_path?m3u_path:"NULL",first_index);

	// find the base path
	if (m3u_path == NULL)
	{
		retro_log(RETRO_LOG_ERROR,"load_m3u with no path?\n");
		m3u_path = "";
	}
	strcpy_trunc(path,m3u_path,sizeof(path));
	{
		int e = strlen(path);
		for(;e>0;--e)
		{
			if(path[e-1] == '/' || path[e-1] == '\\') break;
		}
		path[e] = 0;
	}

	if (data == NULL)
	{
		retro_log(RETRO_LOG_ERROR,"load_m3u with no data? '%s'\n",m3u_path);
		return false;
	}

	// scan the file line by line
	unsigned int p = 0; // position in file
	unsigned int lp = 0; // position in line buffer (length)
	bool first = true;
	bool result = false;
	while (p <= size)
	{
		char c = 10;
		if (p < size) c = data[p]; // if p==size then c=10 to generate a final end-of-line
		if (c == 10 || c == 13) // end of line
		{
			//retro_log(RETRO_LOG_DEBUG,"M3U line: '%s'\n",line);
			// trim trailing whitespace
			while (lp > 1 && (line[lp-1] == ' ' || line[lp-1] == '\t'))
			{
				line[lp-1] = 0;
				--lp;
			}
			//retro_log(RETRO_LOG_DEBUG,"M3U trim: '%s'\n",line);
			// use the line if not a comment or empty
			if (lp != 0 && line[0] != '#')
			{
				struct retro_game_info info;

				// assume a line containing a colon (e.g. C:) or starting with a / is an absolute path
				bool absolute = false;
				if (line[0] == '/') absolute = true;
				else
				{
					for (unsigned int i=0; i<lp; ++i)
					{
						if (line[i] == ':') { absolute = true; break; }
					}
				}

				link[0] = 0;
				if (!absolute) // relative path
					strcpy_trunc(link,path,sizeof(link));
				strcat_trunc(link,line,sizeof(link));

				memset(&info,0,sizeof(info));
				info.path = link;
				info.data = NULL;
				info.size = 0;
				info.meta = "<m3u>"; // use this to block recursive m3u
				int index = first_index;
				if (!first) // add new indices after the first one
				{ 
					index = get_num_images();
					if (!add_image_index())
					{
						retro_log(RETRO_LOG_ERROR,"Too many disks loaded, stopping M3U before: %s\n",link);
						result = false;
						break;
					}
				}
				// load the file
				bool file_result = replace_image_index(index, &info);
				//
				if (first)
				{
					result = file_result;
					first = false;
				}
				else if (!file_result) result = false;
			}
			else if (lp != 0 && !strncasecmp(line,"#AUTO:",6)) // EXTM3U AUTO directive passed to Hatari --auto parameter.
			{
				const char* auto_start = line+6;
				while (*auto_start == ' ' || *auto_start == '\t') ++auto_start; // skip leading whitespace
				core_auto_start(auto_start);
			}
			// ready for next line
			lp = 0;
		}
		else if (lp < (sizeof(line)-1))
		{
			line[lp] = c;
			line[lp+1] = 0;
			++lp;
		}
		++p;
	}

	free(data);
	return result;
}

// load first file from zip
static bool load_zip(uint8_t* data, unsigned int size, const char* zip_filename, unsigned first_index)
{
	static char base[256] = "";
	static char link[256] = "";
	unzFile zip = NULL;

	// remove data from disks (take ownership of *data)
	strcpy(disks[first_index].filename,"<zip>");
	disks[first_index].data = NULL;
	disks[first_index].size = 0;
	disks[first_index].saved = false;

	if (zip_filename)
	{
		// retroarch prefixes zip-unpacked files with the zip-name + #, following suit
		strcpy_trunc(base,zip_filename,sizeof(base));
		strcat_trunc(base,"#",sizeof(base));
	}
	else zip_filename = "<NULL>";

	retro_log(RETRO_LOG_INFO,"load_zip(%d,'%s',%d)\n",size,zip_filename,first_index);

	if (data == NULL || size < 1)
	{
		retro_log(RETRO_LOG_ERROR,"load_zip with no data? '%s'\n",zip_filename);
		free(data); // just in case of 0 byte file
		return false;
	}

	zip = unzOpen(data, size);
	if (zip == NULL)
	{
		retro_log(RETRO_LOG_ERROR,"Could not open ZIP file: %s\n",zip_filename);
		free(data);
		return false;
	}

	char zip_file_filename[512];
	unz_file_info zip_file_info;
	while (true)
	{
		if (UNZ_OK != unzGetCurrentFileInfo(zip, &zip_file_info, zip_file_filename, sizeof(zip_file_filename), NULL, 0, NULL, 0))
		{
			retro_log(RETRO_LOG_ERROR,"Could not read file info in ZIP file: %s\n",zip_filename);
			free(data);
			return false;
		}
		retro_log(RETRO_LOG_DEBUG,"Zip contains: '%s'\n",zip_file_filename);
		if (has_extension(zip_file_filename,"st\0" "msa\0" "dim\0" "stx\0" "ipf\0" "ctr\0" "raw\0" "\0"))
			break; // found a file
		if (UNZ_OK != unzGoToNextFile(zip))
		{
			retro_log(RETRO_LOG_ERROR,"Could not find a disk image in ZIP file: %s\n",zip_filename);
			free(data);
			return false;
		}
	};

	if (UNZ_OK != unzOpenCurrentFile(zip))
	{
		retro_log(RETRO_LOG_ERROR,"Could not open disk image in ZIP file: %s (%s)\n",zip_filename,zip_file_filename);
		free(data);
		return false;
	}

	size_t zsize = zip_file_info.uncompressed_size;
	uint8_t* zdata = malloc(zsize);
	if (zdata == NULL)
	{
		retro_log(RETRO_LOG_ERROR,"Out of memory reading disk image from ZIP file: %s (%s)\n",zip_filename,zip_file_filename);
		free(data);
		return false;
	}

	if (zsize != unzReadCurrentFile(zip, zdata, zsize))
	{
		retro_log(RETRO_LOG_ERROR,"Error reading disk image from ZIP file: %s (%s)\n",zip_filename,zip_file_filename);
		free(zdata);
		free(data);
		return false;
	}

	unzCloseCurrentFile(zip);
	unzClose(zip);

	strcpy_trunc(link,base,sizeof(link));
	int e = strlen(zip_file_filename);
	for(;e>0;--e)
	{
		if (zip_file_filename[e-1] == '/' || zip_file_filename[e-1] == '\\') break;
	}
	strcat_trunc(link,zip_file_filename+e,sizeof(link));

	struct retro_game_info info;
	memset(&info,0,sizeof(info));
	info.path = link;
	info.data = zdata;
	info.size = zsize;
	info.meta = NULL;
	bool result = replace_image_index(first_index, &info);
	free(zdata);

	free(data);
	return result;
}

static bool load_gz(uint8_t* data, unsigned int size, const char* gz_filename, unsigned first_index)
{
	static char link[256] = "";

	// remove data from disks (take ownership of *data)
	strcpy(disks[first_index].filename,"<gz>");
	disks[first_index].data = NULL;
	disks[first_index].size = 0;
	disks[first_index].saved = false;

	// strip .gz from filename
	strcpy_trunc(link,gz_filename,sizeof(link));
	{
		int fnl = strlen(link);
		if (fnl >= 3)
			link[fnl-3] = 0;
	}
	if(has_extension(link,"m3u\0" "m3u8\0" "\0"))
	{
		retro_log(RETRO_LOG_ERROR,"Cannot load m3u contained in gz: '%s'\n",gz_filename);
		free(data);
		return false;
	}

	size_t zsize = 2*1024*1024; // assume up to 2MB by default, realloc later if needed
	uint8_t* zdata = malloc(zsize);
	if (zdata == NULL)
	{
		retro_log(RETRO_LOG_ERROR,"Could not load_gz, out of memory (%d bytes): '%s'\n",gz_filename,(int)zsize);
		free(data);
		return false;
	}

	z_stream zs;
	zs.next_in = (Bytef*)data;
	zs.avail_in = size;
	zs.total_out = 0;
	zs.zalloc = Z_NULL;
	zs.zfree = Z_NULL;
	if(inflateInit2(&zs,(16+MAX_WBITS)) != Z_OK)
	{
		retro_log(RETRO_LOG_ERROR,"Could not load_gz, inflateInit2 failure: '%s'\n",gz_filename);
		free(data);
		return false;
	}
	while (true)
	{
		if (zs.total_out >= zsize) // file is bigger, keep growing
		{
			zsize *= 2;
			uint8_t* zdata2 = realloc(zdata, zsize);
			if (zdata2 == NULL)
			{
				retro_log(RETRO_LOG_ERROR,"Could not load_gz, out of memory (%d bytes): '%s'\n",gz_filename,(int)zsize);
				inflateEnd(&zs);
				free(zdata);
				free(data);
				return false;
			}
			zdata = zdata2;
		}
		zs.next_out = zdata + zs.total_out;
		zs.avail_out = zsize - zs.total_out;
		int result = inflate(&zs,Z_SYNC_FLUSH);
		if (result == Z_STREAM_END) break;
		if (result != Z_OK)
		{
			retro_log(RETRO_LOG_ERROR,"Could not load_gz, inflate error: '%s'\n",gz_filename);
			inflateEnd(&zs);
			free(zdata);
			free(data);
			return false;
		}
	}
	zsize = zs.total_out;
	inflateEnd(&zs);
	free(data);

	struct retro_game_info info;
	memset(&info,0,sizeof(info));
	info.path = link;
	info.data = zdata;
	info.size = zsize;
	info.meta = NULL;
	bool result = replace_image_index(first_index, &info);
	free(zdata);

	return result;
}

static bool load_hard(const char* path, const char* filename, unsigned index, const char* ext)
{
	retro_log(RETRO_LOG_INFO,"load_hard('%s','%s',%d,'%s')\n",path?path:"NULL",filename?filename:"NULL",index,ext);

	strcpy_trunc(disks[index].filename,"<HD>",CORE_MAX_FILENAME);
	strcat_trunc(disks[index].filename,filename,sizeof(disks[index].filename));

	// find the base path
	if (path == NULL)
	{
		retro_log(RETRO_LOG_ERROR,"load_hard with no path?\n");
		path = "";
	}

	int ht = -1;
	if (!strcasecmp(ext,"gem")) ht = 0; // GemDOS
	else if (!strcasecmp(ext,"acsi") || !strcasecmp(ext,"ahd") || !strcasecmp(ext,"vhd")) ht = 2; // ACSI
	else if (!strcasecmp(ext,"scsi") || !strcasecmp(ext,"shd")) ht = 3; // SCSI
	else if (!strcasecmp(ext,"ide")) ht = 4; // IDE (Auto)
	if (ht < 0)
	{
		retro_log(RETRO_LOG_ERROR,"Unknown hard disk image type: %s",path);
		return false;
	}
	else if (ht == 0) // GemDOS converts .GEM into directory name
	{
		char gemdos_path[2048];
		strcpy_trunc(gemdos_path, path, sizeof(gemdos_path));
		int ep = strlen(gemdos_path)-4;
		if (ep >= 0)
		{
			gemdos_path[ep] = '/';
			gemdos_path[ep+1] = 0;
		}
		return core_config_hard_content(gemdos_path,ht);
	}
	return core_config_hard_content(path,ht); // Use image name directly
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

	if (ext && (
		!strcasecmp(ext,"acsi") ||
		!strcasecmp(ext,"ahd") ||
		!strcasecmp(ext,"vhd") ||
		!strcasecmp(ext,"scsi") ||
		!strcasecmp(ext,"shd") ||
		!strcasecmp(ext,"ide") ||
		!strcasecmp(ext,"gem") ))
	{
		return load_hard(game->path, path, index, ext);
	}

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
			strcpy_trunc(disks[index].extra_filename,path,CORE_MAX_FILENAME);
			int l = strlen(disks[index].extra_filename);
			if (l >= 3) disks[index].extra_filename[l-3] = 0;
			strcat_trunc(disks[index].extra_filename,"wd1772",CORE_MAX_FILENAME);
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

	if (disks[index].data == NULL) // no save, load the data
	{
		if (game->data) // supplied by libretro, make a copy
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
		else // try to load it ourselves
		{
			// Load New Disk does not obey need_fullpath, so we may need to manually load the file.
			// This also takes care of fetching file links from an M3U list.
			disks[index].data = core_read_file(game->path,&disks[index].size);
		}
		if (disks[index].data == NULL) // we still don't have it
		{
			strcpy(disks[index].filename,"<Not Found>");
			disks[index].size = 0;
			return false;
		}
	}

	if (ext && (!strcasecmp(ext,"m3u") || !strcasecmp(ext,"m3u8")))
	{
		if (game->meta && !strcmp(game->meta,"<m3u>")) // use meta = "<m3u>" to prevent recursive M3Us
		{
			free(disks[index].data);
			disks[index].data = NULL;
			disks[index].size = 0;
			strcpy(disks[index].filename,"<M3U can't contain other M3Us>");
			return false;
		}
		return load_m3u(disks[index].data, disks[index].size, game->path, index);
	}
	else if (ext && !strcasecmp(ext,"zip"))
	{
		return load_zip(disks[index].data, disks[index].size, path, index);
	}
	else if (ext && !strcasecmp(ext,"gz"))
	{
		return load_gz(disks[index].data, disks[index].size, path, index);
	}

	if (path == NULL)
	{
		// Without a path we have to guess an extension, and Hatari will likely fail to load it.
		sprintf(disks[index].filename,"<Image %d>.st",index);
	}
	else
	{
		strcpy_trunc(disks[index].filename,path,CORE_MAX_FILENAME);
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
	if(path) retro_log(RETRO_LOG_DEBUG,"path: %s\n",path); // logging this path but not doing anything with it
	//initial_image = index;
	//return true;
	// Disabling this. RetroArch appears to give you the last disk you had in the drive, last time you played.
	// This is inappropriate for Atari ST, where you almost always need a boot disk to start the game.
	// It also conflicts with the drive B workaround, where RetroArch won't know which drive it is saving the last index of.
	initial_image = 0;
	return false;
}

bool get_image_path(unsigned index, char* path, size_t len)
{
	//retro_log(RETRO_LOG_DEBUG,"get_image_path(%d,%p,%d)\n",index,path,(int)len);
	if (index >= MAX_DISKS) return false;
	if (path && len>0)
	{
		strcpy_trunc(path,disks[index].filename,len);
	}
	return true;
}

bool get_image_label(unsigned index, char* label, size_t len)
{
	//retro_log(RETRO_LOG_DEBUG,"get_image_label(%d,%p,%d)\n",index,label,(int)len);
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
	//retro_log(RETRO_LOG_DEBUG,"core_disk_load_game(%p)\n",game);

	disks_clear();
	if (game == NULL) return;
	add_image_index(); // add one disk
	replace_image_index(0,game); // load it there (may load multiple if M3U/Zip)

	// ensure initial image has data, if possible (avoids selecting hard disk or other invalid image)
	for (int i=0; i<2; ++i) // two passes to be thorough, in case initial_image is not 0
	{
		while (initial_image < image_count && disks[initial_image].data == NULL) ++initial_image;
		if (initial_image >= image_count) initial_image = 0;
	}
	// insert first disk
	set_image_index(initial_image);
	set_eject_state(false); // insert it

	// insert second disk if available
	if (core_disk_enable_b)
	{
		int second_image = initial_image + 1;
		for (int i=0; i<2; ++i)
		{
			while(second_image < image_count && disks[second_image].data == NULL) ++second_image;
			if (second_image >= image_count) second_image = 0;
		}
		if (second_image != initial_image && disks[second_image].data != NULL)
		{
			drive = 1;
			set_image_index(second_image);
			set_eject_state(false);
			drive = 0;
		}
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
		const char* infile = core_floppy_inserted(d);
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
					if (core_savestate_floppy_modify)
					{
						core_floppy_changed(d);
						//retro_log(RETRO_LOG_DEBUG,"Savestate marks floppy contents changed: %s\n",disks[i].filename);
					}
				}
			}
			else
			{
				image_index[d] = MAX_DISKS; // set to invalid index
				retro_log(RETRO_LOG_ERROR,"core_disk_serialize disk in drive %d not cached: '%s'\n",d,infile);
				// do a save test for possible modifications
				if (core_disk_enable_save && core_savestate_floppy_modify && core_disk_save_exists(infile))
				{
					// TODO this doesn't work for STX because it wants an overlay file instead (change extension if STX)
					core_floppy_changed(d);
					//retro_log(RETRO_LOG_DEBUG,"Savestate marks uncached floppy contents changed: %s\n",infile);
				}
			}
		}
	}
}

void core_disk_drive_toggle(void)
{
	drive = drive ^ 1;

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
	//retro_log(RETRO_LOG_DEBUG,"core_disk_drive_reinsert()\n");
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

void core_disk_swap(void) // convenience to eject and swap to next disk
{
	set_eject_state(true);
	unsigned int i = get_image_index();
	i += 1;
	if (i >= get_num_images()) i = 0;
	set_image_index(i);
	set_eject_state(false);
	core_signal_alert2((drive==0) ? "[A:] " : "[B:] ",disks[image_index[drive]].filename);
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
char disk_save_advanced_filename[CORE_MAX_FILENAME] = "";

// buffer to 
uint8_t* disk_save_advanced_buffer = NULL;
unsigned int disk_save_advanced_buffer_size = 0;
unsigned int disk_save_advanced_buffer_pos = 0;
// use 2MB
#define ADVBUFFER_SIZE   (2*1024*1024)

corefile* core_disk_save_open(const char* filename)
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

void core_disk_save_close_extra(corefile* file, bool success)
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

bool core_disk_save_write(const uint8_t* data, unsigned int size, corefile* file)
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
	return (size == core_file_write(data, 1, size, file));
}

bool core_disk_save_exists(const char* filename)
{
	return core_file_exists_save(filename);
}
