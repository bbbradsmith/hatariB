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

#define M3U_EXTENSIONS "m3u\0" "m3u8\0" "\0"
#define DISK_EXTENSIONS "st\0" "msa\0" "dim\0" "stx\0" "ipf\0" "ctr\0" "raw\0" "\0"
#define HD_EXTENSIONS "acsi\0" "ahd\0" "vhd\0" "scsi\0" "shd\0" "ide\0" "gem\0" "\0"
#define ZIP_EXTENSIONS "zip\0" "zst\0" "\0"
#define GZ_EXTENSIONS "gz\0" "\0"
#define STX_EXTENSIONS "stx\0" "\0"
#define HD_ACSI_EXTENSIONS "acsi\0" "ahd\0" "vhd\0" "\0"
#define HD_SCSI_EXTENSIONS "scsi\0" "shd\0" "\0"
#define HD_IDE_EXTENSIONS "ide\0" "\0"
#define HD_GEM_EXTENSIONS "gem\0" "\0"
#define STX_SAVE_EXT "wd1772"

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
static int boot_index[2];

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
	boot_index[0] = -1;
	boot_index[1] = -1;
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
static uint8_t* load_zip_search_file(unzFile* zip, const char* zip_filename, size_t* filesize, const char* search_filename);

//
// M3U files
//

static bool load_m3u(uint8_t* data, unsigned int size, const char* m3u_path, unsigned first_index, unzFile* zip)
{
	static char path[2048] = "";
	static char link[2048] = "";
	static char line[2048] = "";

	// remove data from disks (take ownership of *data)
	strcpy(disks[first_index].filename,"<m3u>");
	disks[first_index].data = NULL;
	disks[first_index].size = 0;
	disks[first_index].saved = false;

	retro_log(RETRO_LOG_INFO,"load_m3u(%d,'%s',%d,%s)\n",size,m3u_path?m3u_path:"NULL",first_index,zip?"ZIP":"filesystem");

	// find the base path
	if (m3u_path == NULL)
	{
		retro_log(RETRO_LOG_ERROR,"load_m3u with no path?\n");
		m3u_path = "";
	}
	// use the M3U's directory as our base path
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

				// invalidate this image in case it's not found
				strcpy(disks[index].filename,"<Missing>");
				disks[index].data = NULL;
				disks[index].size = 0;
				disks[index].saved = false;

				bool file_result = true;

				uint8_t* zdata = NULL;
				if (zip) // load the ZIP data
				{
					file_result = false;
					if (has_extension(link,HD_EXTENSIONS))
					{
						retro_log(RETRO_LOG_ERROR,"Hard disk image not supported inside ZIP: '%s'\n",link);
					}
					else
					{
						// NOTE: The zip search does not support or a path using ./ or ../ but it is probably
						//       unnessary to resolve this. I can't think of a reason to put images above the M3U,
						//       only at the same level or below. Also, ZIP standardizes paths as UTF-8,
						//       which is good since M3U8 is UTF-8 too.
						
						// ZIP specifies only '/' as the path separator
						for (char* c = link; *c; ++c) { if(*c=='\\') *c='/'; }

						size_t zsize;
						zdata = load_zip_search_file(zip, m3u_path, &zsize, link);
						if (zdata)
						{
							info.data = zdata;
							info.size = zsize;
							file_result = true;
						}
					}
				}

				// load the file
				if (file_result) file_result = replace_image_index(index, &info);
				if (first)
				{
					result = file_result;
					first = false;
				}
				else if (!file_result) result = false;
				free(zdata);
			}
			else if (lp != 0 && !strncasecmp(line,"#AUTO:",6)) // EXTM3U AUTO directive passed to Hatari --auto parameter.
			{
				const char* auto_start = line+6;
				while (*auto_start == ' ' || *auto_start == '\t') ++auto_start; // skip leading whitespace
				core_auto_start(auto_start);
			}
			else if (lp != 0 && (!strncasecmp(line,"#BOOTA:",7) || !strncasecmp(line,"#BOOTB:",7))) // EXTM3U BOOTA/BOOTB directive
			{
				const char* boot = line+7;
				int d = line[5] - 'A';
				while (*boot == ' ' || *boot == '\t') ++boot;
				if (*boot == 0)
				{
					boot_index[d] = 0; // empty drive
				}
				else
				{
					boot_index[d] = strtol(boot,NULL,10);
					if (boot_index[d] < 0) boot_index[d] = 0; // treat negative values as empty
				}
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

//
// ZIP files
//

// load current file from ZIP
static uint8_t* load_zip_current_file(unzFile* zip, const char* zip_filename, size_t* filesize)
{
	char filename[CORE_MAX_FILENAME];
	unz_file_info info;
	if (filesize) *filesize = 0;
	retro_log(RETRO_LOG_INFO,"load_zip_current_file(%p,%s,%p)\n",zip,zip_filename,filesize);

	if (UNZ_OK != unzGetCurrentFileInfo(zip, &info, filename, sizeof(filename), NULL, 0, NULL, 0))
	{
		retro_log(RETRO_LOG_ERROR,"Could not read ZIP file info: %s\n",zip_filename);
		return NULL;
	}
	if (UNZ_OK != unzOpenCurrentFile(zip))
	{
		retro_log(RETRO_LOG_ERROR,"Could not open ZIP file: %s (%s)\n",filename,zip_filename);
		return NULL;
	}

	size_t zsize = info.uncompressed_size;
	uint8_t* data = malloc(zsize);
	if (data == NULL)
	{
		retro_log(RETRO_LOG_ERROR,"Out of memory reading from ZIP file: %s (%s)\n",filename,zip_filename);
		unzCloseCurrentFile(zip);
		return NULL;
	}
	if (zsize != unzReadCurrentFile(zip, data, zsize))
	{
		retro_log(RETRO_LOG_ERROR,"Error reading from ZIP file: %s (%s)\n",filename,zip_filename);
		free(data);
		unzCloseCurrentFile(zip);
		return NULL;
	}
	unzCloseCurrentFile(zip);

	if (filesize) *filesize = zsize;
	return data;
}

// search for and load a file from ZIP
static uint8_t* load_zip_search_file(unzFile* zip, const char* zip_filename, size_t* filesize, const char* search_filename)
{
	retro_log(RETRO_LOG_INFO,"load_zip_search_file(%p,'%s',%p,'%s')\n",zip,zip_filename,filesize,search_filename);
	if (filesize) *filesize = 0;
	if (UNZ_OK != unzLocateFile(zip,search_filename,2)) // case insensitive search
	{
		retro_log(RETRO_LOG_ERROR,"File not found in ZIP: %s (%s)\n",search_filename,zip_filename);
		return NULL;
	}
	return load_zip_current_file(zip, zip_filename, filesize);
}

// load entire ZIP
static bool load_zip(uint8_t* data, unsigned int size, const char* zip_filename, unsigned first_index)
{
	static char link[CORE_MAX_FILENAME] = "";
	unzFile zip = NULL;

	// remove data from disks (take ownership of *data)
	strcpy(disks[first_index].filename,"<ZIP>");
	disks[first_index].data = NULL;
	disks[first_index].size = 0;
	disks[first_index].saved = false;

	if (!zip_filename) zip_filename = "<Unknown ZIP>";

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

	// look for M3U first
	retro_log(RETRO_LOG_DEBUG,"Scanning ZIP for M3U: %s\n",zip_filename);
	while (UNZ_OK ==  unzGetCurrentFileInfo(zip, &zip_file_info, zip_file_filename, sizeof(zip_file_filename), NULL, 0, NULL, 0))
	{
		retro_log(RETRO_LOG_DEBUG,"ZIP M3U scan: '%s'\n",zip_file_filename);
		if (has_extension(zip_file_filename,M3U_EXTENSIONS))
		{
			// load the M3U
			size_t zsize;
			uint8_t* zdata = load_zip_current_file(zip, zip_filename, &zsize);
			if (zdata)
			{
				bool result = load_m3u(zdata,zsize,zip_file_filename,first_index,zip); // load_m3u now owns zdata
				unzClose(zip); free(data);
				return result;
			}
			unzClose(zip); free(data);
			return false;
		}
		if (UNZ_OK != unzGoToNextFile(zip)) break;
	}

	// No M3U just load the files in the order they're found
	retro_log(RETRO_LOG_DEBUG,"No M3U found, loading all files in ZIP: %s\n",zip_filename);
	if (UNZ_OK != unzGoToFirstFile(zip))
	{
		retro_log(RETRO_LOG_ERROR,"Could not find first file in ZIP: %s\n",zip_filename);
		unzClose(zip); free(data); return false;
	}
	bool first = true;
	bool result = false;
	while (true)
	{
		if (UNZ_OK != unzGetCurrentFileInfo(zip, &zip_file_info, zip_file_filename, sizeof(zip_file_filename), NULL, 0, NULL, 0))
		{
			retro_log(RETRO_LOG_ERROR,"Could not read ZIP file info: %s\n",zip_filename);
			unzClose(zip); free(data); return false;
		}
		retro_log(RETRO_LOG_DEBUG,"ZIP contains: '%s'\n",zip_file_filename);
		if (has_extension(zip_file_filename,DISK_EXTENSIONS))
		{
			size_t zsize;
			uint8_t* zdata = load_zip_current_file(zip, zip_filename, &zsize);
			if (!zdata)
			{
				result = false;
			}
			else
			{
				int index = first_index;
				if (!first)
				{
					index = get_num_images();
					if (!add_image_index())
					{
						retro_log(RETRO_LOG_ERROR,"Too many disks loaded, stopping ZIP before: %s\n",link);
						result = false;
						break;
					}
				}

				struct retro_game_info info;
				memset(&info,0,sizeof(info));
				info.path = zip_file_filename;
				info.data = zdata;
				info.size = zsize;
				info.meta = NULL;
				bool file_result = replace_image_index(index, &info);
				free(zdata);
				if (first && file_result)
				{
					first = false;
					result = true;
				}
				else if (!file_result) result = false;
			}
		}
		if (UNZ_OK != unzGoToNextFile(zip)) break;
	};
	unzClose(zip);
	free(data);
	return result;
}

//
// GZ gzip files
//

uint8_t* unzip_gz(const uint8_t* gz_data, size_t gz_size, const char* gz_filename, size_t* filesize, size_t size_estimate)
{
	size_t zsize = size_estimate; // gz does not contain its size: start with estimate, realloc later if needed
	if (filesize) *filesize = 0;

	uint8_t* zdata = malloc(zsize);
	if (zdata == NULL)
	{
		retro_log(RETRO_LOG_ERROR,"Could not unzip gz, out of memory (%d bytes): '%s'\n",gz_filename,(int)zsize);
		return NULL;
	}

	z_stream zs;
	zs.next_in = (Bytef*)gz_data;
	zs.avail_in = gz_size;
	zs.total_out = 0;
	zs.zalloc = Z_NULL;
	zs.zfree = Z_NULL;
	if(inflateInit2(&zs,(16+MAX_WBITS)) != Z_OK)
	{
		retro_log(RETRO_LOG_ERROR,"Could not unzip gz, inflateInit2 failure: '%s'\n",gz_filename);
		return NULL;
	}
	while (true)
	{
		if (zs.total_out >= zsize)
		{
			retro_log(RETRO_LOG_INFO,"Size estimate (%d) too small for gz, expanding: %d\n",(int)(zsize*2));
			zsize *= 2;
			uint8_t* zdata2 = realloc(zdata, zsize);
			if (zdata2 == NULL)
			{
				retro_log(RETRO_LOG_ERROR,"Could not unzip gz, out of memory (%d bytes): '%s'\n",gz_filename,(int)zsize);
				inflateEnd(&zs);
				free(zdata);
				return NULL;
			}
			zdata = zdata2;
		}
		zs.next_out = zdata + zs.total_out;
		zs.avail_out = zsize - zs.total_out;
		int result = inflate(&zs,Z_SYNC_FLUSH);
		if (result == Z_STREAM_END) break;
		if (result != Z_OK)
		{
			retro_log(RETRO_LOG_ERROR,"Could not unzip gz, inflate error: '%s'\n",gz_filename);
			inflateEnd(&zs);
			free(zdata);
			return NULL;
		}
	}
	zsize = zs.total_out;
	inflateEnd(&zs);
	return zdata;
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
	if (has_extension(link,M3U_EXTENSIONS))
	{
		retro_log(RETRO_LOG_ERROR,"Cannot load m3u contained in gz: '%s'\n",gz_filename);
		free(data);
		return false;
	}
	if (has_extension(link,HD_EXTENSIONS))
	{
		retro_log(RETRO_LOG_ERROR,"Hard disk image not supported inside gz: '%s'\n",gz_filename);
		free(data);
		return false;
	}

	const int DEFAULT_SIZE = 2*1024*1024; // 2MB should be larger than most images, it will realloc if larger.
	size_t zsize;
	uint8_t* zdata = unzip_gz(data, size, gz_filename, &zsize, DEFAULT_SIZE);
	if (zdata == NULL)
	{
		free(data);
		return false;
	}

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

//
// Hard drives
//

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
	if      (has_extension(ext,HD_GEM_EXTENSIONS )) ht = 0; // GemDOS
	else if (has_extension(ext,HD_ACSI_EXTENSIONS)) ht = 2; // ACSI
	else if (has_extension(ext,HD_SCSI_EXTENSIONS)) ht = 3; // SCSI
	else if (has_extension(ext,HD_IDE_EXTENSIONS )) ht = 4; // IDE (Auto)
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

//
// Disk image handling
//

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
			if(path[e-1] == '.' && ext == NULL) ext = path + (e-1);
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

	if (ext && has_extension(ext,HD_EXTENSIONS))
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
		if (ext && has_extension(ext,STX_EXTENSIONS)) // STX may have a secondary save file used as an overlay
		{
			strcpy_trunc(disks[index].extra_filename,path,CORE_MAX_FILENAME);
			int l = strlen(disks[index].extra_filename);
			if (l >= 3) disks[index].extra_filename[l-3] = 0;
			strcat_trunc(disks[index].extra_filename,STX_SAVE_EXT,CORE_MAX_FILENAME);
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
			strcpy(disks[index].filename,"<Missing>");
			disks[index].size = 0;
			return false;
		}
	}

	if (ext && has_extension(ext,M3U_EXTENSIONS))
	{
		if (game->meta && !strcmp(game->meta,"<m3u>")) // use meta = "<m3u>" to prevent recursive M3Us
		{
			free(disks[index].data);
			disks[index].data = NULL;
			disks[index].size = 0;
			strcpy(disks[index].filename,"<M3U can't contain other M3Us>");
			return false;
		}
		return load_m3u(disks[index].data, disks[index].size, game->path, index, NULL);
	}
	else if (ext && has_extension(ext,ZIP_EXTENSIONS))
	{
		return load_zip(disks[index].data, disks[index].size, path, index);
	}
	else if (ext && has_extension(ext,GZ_EXTENSIONS))
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
		NULL, //set_initial_image, (disable this feature because it is almost never appropriate to boot from a different disk)
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
}

void core_disk_load_game(const struct retro_game_info *game)
{
	//retro_log(RETRO_LOG_DEBUG,"core_disk_load_game(%p)\n",game);
	int initial_image = 0;

	disks_clear();
	if (game == NULL) return;
	add_image_index(); // add one disk
	replace_image_index(0,game); // load it there (may load multiple if M3U/Zip)

	// out of bounds = empty drive
	for (int i=0; i<2; ++i)
	{
		if (boot_index[i] > (int)image_count) boot_index[i] = 0;
	}

	// fill drives selected by #BOOTA/#BOOTB
	// reverse order gives BOOTA precedence if they were both the same
	if (boot_index[1] > 0 && core_disk_enable_b)
	{
		drive = 1;
		set_image_index(boot_index[1]-1);
		set_eject_state(false);
	}
	if (boot_index[0] > 0)
	{
		drive = 0;
		set_image_index(boot_index[0]-1);
		set_eject_state(false);
		initial_image = image_index[0];
	}

	// automatic boot disk selection
	if (boot_index[0] < 0)
	{
		// ensure initial image has data, if possible (avoids selecting hard disk or other invalid image)
		for (int i=0; i<2; ++i) // two passes, in case initial_image is not 0
		{
			while (initial_image < image_count &&
				(disks[initial_image].data == NULL || (image_insert[1] == true && initial_image == image_index[1])))
				++initial_image;
			if (initial_image >= image_count) initial_image = 0;
		}
		// insert first disk, if it's not already the second disk
		if (image_insert[1] == false || (initial_image != image_index[1]))
		{
			drive = 0;
			set_image_index(initial_image);
			set_eject_state(false);
		}
	}

	// automatic second disk selection
	if (boot_index[1] < 0 && core_disk_enable_b)
	{
		int second_image = 0;
		while(second_image < image_count &&
			(disks[second_image].data == NULL || (image_insert[0] == true && second_image == image_index[0])))
			++second_image;
		if (second_image >= image_count) second_image = 0;
		// insert second disk, if it exists, and it's not already inserted in drive A
		if (disks[second_image].data != NULL &&
			(image_insert[0] == false || (second_image != image_index[0])))
		{
			drive = 1;
			set_image_index(second_image);
			set_eject_state(false);
		}
	}

	// set initial drive
	drive = 0;
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
	if (has_extension(filename,STX_EXTENSIONS))
	{
		char stx_savename[CORE_MAX_FILENAME];
		strcpy_trunc(stx_savename,filename,CORE_MAX_FILENAME);
		int l = strlen(stx_savename);
		if (l >= 3) stx_savename[l-3] = 0;
		strcat_trunc(stx_savename,STX_SAVE_EXT,CORE_MAX_FILENAME);
		return core_file_exists_save(stx_savename);
	}
	return core_file_exists_save(filename);
}
