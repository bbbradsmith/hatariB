#include "../libretro/libretro.h"
#include "core.h"
#include "core_internal.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAX_PATH          2048
#define MAX_FILENAME      256
#define MAX_SYSTEM_FILE   128
#define MAX_SYSTEM_DIR     16

#define USE_RETRO_VFS   1

static int sf_count = 0;
static char sf_filename[MAX_SYSTEM_FILE][MAX_FILENAME];
static int sf_dir_count = 0;
static char sf_dirname[MAX_SYSTEM_DIR][MAX_FILENAME];
static char sf_dirlabel[MAX_SYSTEM_DIR][MAX_FILENAME];

struct retro_vfs_interface* retro_vfs = NULL;
int retro_vfs_version = 0;

static char system_path[MAX_PATH] = "";
static char save_path[MAX_PATH] = "";
static bool save_path_ready = false;

//
// Utilities
//

// safe truncating strcpy/strcat
void strcpy_trunc(char* dest, const char* src, unsigned int len)
{
	while (len > 1 && *src)
	{
		*dest = *src;
		++dest; ++src;
		--len;
	}
	*dest = 0;
}
void strcat_trunc(char* dest, const char* src, unsigned int len)
{
	while (len > 1 && *dest)
	{
		++dest; --len;
	}
	strcpy_trunc(dest, src, len);
}

//
// File system abstraction
//

static char temp_fn[MAX_PATH];

const char* temp_fn2(const char* path1, const char* path2)
{
	if(path1) strcpy_trunc(temp_fn, path1, sizeof(temp_fn));
	else temp_fn[0] = 0;
	if(path2) strcat_trunc(temp_fn, path2, sizeof(temp_fn));
	return temp_fn;
}

const char* temp_fn3(const char* path1, const char* path2, const char* path3)
{
	temp_fn2(path1,path2);
	if(path3) strcat_trunc(temp_fn, path3, sizeof(temp_fn));
	return temp_fn;
}

void save_path_init()
{
	const char* cp;
	if (save_path_ready) return;
	if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY,(void*)&cp) && cp)
	{
		retro_log(RETRO_LOG_DEBUG,"GET_SAVE_DIRECTORY success\n");
		strcpy_trunc(save_path, cp, sizeof(save_path));
		if (strlen(save_path) > 0)
			strcat_trunc(save_path, "/", sizeof(save_path));
	}
	else
	{
		save_path[0] = 0;
	}
	retro_log(RETRO_LOG_INFO,"save_path: %s\n",save_path);
	save_path_ready = true;
}

uint8_t* core_read_file(const char* filename, unsigned int* size_out)
{
	uint8_t* d = NULL;
	unsigned int size = 0;
	retro_log(RETRO_LOG_INFO,"core_read_file('%s')\n",filename);

	if (size_out) *size_out = 0;
	if (retro_vfs_version >= 3)
	{
		int64_t rs;
		struct retro_vfs_file_handle* f = retro_vfs->open(filename,RETRO_VFS_FILE_ACCESS_READ,0);
		if (f == NULL)
		{
			retro_log(RETRO_LOG_ERROR,"core_read_file (VFS) not found: %s\n",filename);
			return NULL;
		}
		rs = retro_vfs->size(f);
		if (rs < 0)
		{
			retro_log(RETRO_LOG_ERROR,"core_read_file (VFS) size error: %s\n",filename);
			return NULL;
		}
		size = (unsigned int)rs;
		d = malloc(size);
		if (d == NULL)
		{
			retro_log(RETRO_LOG_ERROR,"core_read_file (VFS) out of memory: %s\n",filename);
			retro_vfs->close(f);
			return NULL;
		}
		retro_vfs->read(f,d,size);
		retro_vfs->close(f);
	}
	else
	{
		FILE* f = fopen(filename,"rb");
		if (f == NULL)
		{
			retro_log(RETRO_LOG_ERROR,"core_read_file not found: %s\n",filename);
			return NULL;
		}
		fseek(f,0,SEEK_END);
		size = (unsigned int)ftell(f);
		fseek(f,0,SEEK_SET);
		d = malloc(size);
		if (d == NULL)
		{
			retro_log(RETRO_LOG_ERROR,"core_read_file out of memory: %s\n",filename);
			fclose(f);
			return NULL;
		}
		fread(d,1,size,f);
		fclose(f);
	}
	if (size_out) *size_out = size;
	return d;
}

bool core_write_file(const char* filename, unsigned int size, const uint8_t* data)
{
	retro_log(RETRO_LOG_INFO,"core_write_file('%s',%d)\n",filename,size);
	if (retro_vfs_version >= 3)
	{
		struct retro_vfs_file_handle* f = retro_vfs->open(filename,RETRO_VFS_FILE_ACCESS_WRITE,0);
		if (f == NULL)
		{
			retro_log(RETRO_LOG_ERROR,"core_write_file (VFS) could not open: %s\n",filename);
			return false;
		}
		if (retro_vfs->write(f,data,size) < 0)
		{
			retro_log(RETRO_LOG_ERROR,"core_write_file (VFS) could not write: %s\n",filename);
			retro_vfs->close(f);
			return false;
		}
		retro_vfs->close(f);
	}
	else
	{
		FILE* f = fopen(filename,"wb");
		if (f == NULL)
		{
			retro_log(RETRO_LOG_ERROR,"core_write_file could not open: %s\n",filename);
			return false;
		}
		if (size != fwrite(data,1,size,f))
		{
			retro_log(RETRO_LOG_ERROR,"core_write_file could not write: %s\n",filename);
			fclose(f);
			return false;
		}
		fclose(f);
	}
	return true;
}

uint8_t* core_read_file_system(const char* filename, unsigned int* size_out)
{
	retro_log(RETRO_LOG_INFO,"core_read_file_system('%s')\n",filename);
	return core_read_file(temp_fn2(system_path,filename),size_out);
}

uint8_t* core_read_file_save(const char* filename, unsigned int* size_out)
{
	retro_log(RETRO_LOG_INFO,"core_read_file_save('%s')\n",filename);
	save_path_init();
	return core_read_file(temp_fn2(save_path,filename),size_out);
}

bool core_write_file_save(const char* filename, unsigned int size, const uint8_t* data)
{
	retro_log(RETRO_LOG_INFO,"core_write_file_save('%s',%d)\n",filename,size);
	save_path_init();
	return core_write_file(temp_fn2(save_path,filename), size, data);
}

//
// Setup and system file scan
//

static void core_file_system_add(const char* filename, bool prefix_hatarib)
{
	if (sf_count >= MAX_SYSTEM_FILE) return;
	sf_filename[sf_count][0] = 0;
	if (prefix_hatarib)
		strcpy_trunc(sf_filename[sf_count],"hatarib/",MAX_FILENAME);
	strcat_trunc(sf_filename[sf_count],filename,MAX_FILENAME);
	//retro_log(RETRO_LOG_INFO,"core_file_system_add: %s\n",sf_filename[sf_count]);
	++sf_count;
}

static void core_file_system_add_dir(const char* filename)
{
	if (sf_dir_count >= MAX_SYSTEM_DIR) return;
	if (!strcmp(filename,".")) return;
	if (!strcmp(filename,"..")) return;
	strcpy_trunc(sf_filename[sf_count],"hatarib/",MAX_FILENAME);
	strcat_trunc(sf_dirname[sf_dir_count],filename,MAX_FILENAME);
	strcpy_trunc(sf_dirlabel[sf_dir_count],filename,MAX_FILENAME);
	strcat_trunc(sf_dirlabel[sf_dir_count],"/",MAX_FILENAME);
	++sf_dir_count;
}

void core_file_set_environment(retro_environment_t cb)
{
	struct retro_vfs_interface_info retro_vfs_info = { 3, NULL };
	const char* cp;
	
	sf_count = 0;
	sf_dir_count = 0;
	memset(sf_filename,0,sizeof(sf_filename));
	memset(sf_dirname,0,sizeof(sf_dirname));
	memset(sf_dirlabel,0,sizeof(sf_dirlabel));

#if USE_RETRO_VFS
	if (retro_vfs_version > 0)
	{
		// if already initialized RETRO_ENVIRONMENT_GET_VFS_INTERFACE seems to fail,
		// so just keep what we've got.
		retro_log(RETRO_LOG_INFO,"retro_vfs version: %d\n",retro_vfs_version);
	}
	else if (cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE,(void*)&retro_vfs_info))
	{
		retro_vfs_version = retro_vfs_info.required_interface_version;
		retro_vfs = retro_vfs_info.iface;
		retro_log(RETRO_LOG_INFO,"retro_vfs version: %d\n",retro_vfs_version);
	}
	else
#endif
	{
		(void)retro_vfs_info;
		retro_log(RETRO_LOG_INFO,"retro_vfs not available\n");
	}

	// system path is available immediately
	if (cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY,(void*)&cp) && cp)
	{
		retro_log(RETRO_LOG_DEBUG,"GET_SYSTEM_DIRECTORY success\n");
		strcpy_trunc(system_path, cp, sizeof(system_path));
		if (strlen(system_path) > 0)
			strcat_trunc(system_path, "/", sizeof(system_path));
	}
	retro_log(RETRO_LOG_INFO,"system_path: %s\n",system_path);

	// save path is not ready until retro_load_game
	save_path_ready = false;

	// scan system folder
	if (retro_vfs_version >= 3) // retro_vfs
	{
		struct retro_vfs_file_handle* fh;
		struct retro_vfs_dir_handle* dir;

		// check for tos.img

		fh = retro_vfs->open(temp_fn2(system_path,"tos.img"),RETRO_VFS_FILE_ACCESS_READ,0);
		if (fh)
		{
			core_file_system_add("tos.img",false);
			retro_vfs->close(fh);
		}

		// scan system/hatarib/
		dir = retro_vfs->opendir(temp_fn2(system_path,"hatarib"),false);
		if (dir)
		{
			while(retro_vfs->readdir(dir))
			{
				const char* fn = retro_vfs->dirent_get_name(dir);
				if (fn)
				{
					if (!retro_vfs->dirent_is_dir(dir))
						core_file_system_add(fn,true);
					else
						core_file_system_add_dir(fn);
				}
			}
			retro_vfs->closedir(dir);
		}
	}
	else // posix stat/opendir/readdir/closedir
	{
		DIR* dir;
		struct dirent* de;
		struct stat fs;

		// check for tos.img
		if((0 == stat(temp_fn2(system_path,"tos.img"), &fs)) && !(fs.st_mode & S_IFDIR))
		{
			core_file_system_add("tos.img",false);
		}

		// scan system/hatarib/
		dir = opendir(temp_fn2(system_path,"hatarib"));
		if (dir)
		{
			while ((de = readdir(dir)))
			{
				if(0 == stat(temp_fn3(system_path,"hatarib/",de->d_name), &fs))
				{
					 if (!(fs.st_mode & S_IFDIR))
						core_file_system_add(de->d_name,true);
					else
						core_file_system_add_dir(de->d_name);
				}
			}
			closedir(dir);
		}
	}
}

int core_file_system_count()
{
	return sf_count;
}

const char* core_file_system_filename(int index)
{
	if (index >= sf_count) return "";
	return sf_filename[index];
}

int core_file_system_dir_count()
{
	return sf_dir_count;
}

const char* core_file_system_dirname(int index)
{
	if (index >= sf_dir_count) return "";
	return sf_dirname[index];
}

const char* core_file_system_dirlabel(int index)
{
	if (index >= sf_dir_count) return "";
	return sf_dirlabel[index];
}
