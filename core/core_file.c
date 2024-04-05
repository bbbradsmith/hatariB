#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "../hatari/src/includes/main.h"
#include "../libretro/libretro.h"
#include "core.h"
#include "core_internal.h"

#define MAX_PATH          2048
#define MAX_FILENAME      256
#define MAX_SYSTEM_FILE   128
#define MAX_SYSTEM_DIR     16

// set to 0 to only use standard filesystem
// (unfortunately I can't do this as a core option because it has to be activated before options are available to read)
#define USE_RETRO_VFS   1

// set to 1 to log most low-level events using core_file interface
#define CORE_FILE_DEBUG   0

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

int core_hard_readonly = 1;
bool core_hard_content = false; // if hard disk image is loaded as content, this overrides the system path
int core_hard_content_type;
char core_hard_content_path[2048];

#if CORE_FILE_DEBUG
	#define CFD(_stuff_) {_stuff_;}
#else
	#define CFD(_stuff_) {}
#endif

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

// compare against exts list of null terminated strings, ended with a double null
bool has_extension(const char* fn, const char* exts)
{
	//retro_log(RETRO_LOG_DEBUG,"has_extension('%s',%p)\n",fn,exts);
	size_t e = strlen(fn);
	for(;e>0;--e)
	{
		if(fn[e] == '.') break;
	}
	if (fn[e] == '.') ++e;
	else return false; // no extension found
	const char* ext = fn + e;
	while (exts[0] != 0)
	{
		//retro_log(RETRO_LOG_DEBUG,"exts: '%s' (%p)\n",exts,exts);
		if (!strcasecmp(ext,exts)) return true;
		exts = exts + strlen(exts) + 1;
	}
	return false;
}

//
// Basic file system abstraction
//

static char temp_fn[MAX_PATH];

static const char* temp_fn_sepfix(const char* path) // adjust path separators for system, NULL to reuse temp_fn
{
	if (path != NULL && path != temp_fn)
		strcpy_trunc(temp_fn, path, sizeof(temp_fn));

	// assume the first found separator is correct and set the rest accordingly
	char sep = '/';
	for (const char* c = temp_fn; *c; ++c)
	{
		if      (*c == '\\') { sep = '\\'; break; }
		else if (*c == '/') break;
	}
	for (char* c = temp_fn; *c; ++c)
	{
		if (*c == '/' || *c == '\\') *c = sep;
	}
	return temp_fn;
}

static const char* temp_fn2(const char* path1, const char* path2)
{
	if(path1) strcpy_trunc(temp_fn, path1, sizeof(temp_fn));
	else temp_fn[0] = 0;
	if(path2) strcat_trunc(temp_fn, path2, sizeof(temp_fn));
	return temp_fn_sepfix(NULL);
}

static const char* temp_fn3(const char* path1, const char* path2, const char* path3)
{
	temp_fn2(path1,path2);
	if(path3) strcat_trunc(temp_fn, path3, sizeof(temp_fn));
	return temp_fn_sepfix(NULL);
}

const char* get_temp_fn()
{
	return temp_fn;
}

void save_path_init()
{
	const char* cp;
	if (save_path_ready) return;
	if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY,(void*)&cp) && cp)
	{
		//retro_log(RETRO_LOG_DEBUG,"GET_SAVE_DIRECTORY success\n");
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
	filename = temp_fn_sepfix(filename);

	if (size_out) *size_out = 0;
	if (retro_vfs_version >= 3)
	{
		int64_t rs;
		struct retro_vfs_file_handle* f = retro_vfs->open(filename,RETRO_VFS_FILE_ACCESS_READ,0);
		if (f == NULL)
		{
			retro_log(RETRO_LOG_DEBUG,"core_read_file (VFS) not found: %s\n",filename);
			return NULL; // note: not necessarily an error
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
			retro_log(RETRO_LOG_DEBUG,"core_read_file not found: %s\n",filename);
			return NULL; // note: not necessarily an error
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
		(void)!fread(d,1,size,f); // (void)! suppresses warn_unused_result
		fclose(f);
	}
	if (size_out) *size_out = size;
	return d;
}

bool core_write_file(const char* filename, unsigned int size, const uint8_t* data)
{
	retro_log(RETRO_LOG_INFO,"core_write_file('%s',%d)\n",filename,size);
	filename = temp_fn_sepfix(filename);

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
	return core_read_file(temp_fn2(system_path,filename),size_out);
}

uint8_t* core_read_file_save(const char* filename, unsigned int* size_out)
{
	save_path_init();
	return core_read_file(temp_fn2(save_path,filename),size_out);
}

uint8_t* core_read_file_hard(const char* filename, unsigned int* size_out)
{
	if (core_hard_content) return core_read_file(filename, size_out);
	else                   return core_read_file_system(filename, size_out);
}

bool core_write_file_system(const char* filename, unsigned int size, const uint8_t* data)
{
	save_path_init();
	return core_write_file(temp_fn2(system_path,filename), size, data);
}

bool core_write_file_save(const char* filename, unsigned int size, const uint8_t* data)
{
	save_path_init();
	return core_write_file(temp_fn2(save_path,filename), size, data);
}

//
// Direct file system abstraction
//

corefile* core_file_open(const char* path, int access)
{
	CFD(retro_log(RETRO_LOG_INFO,"core_file_open('%s',%d)\n",path,access));
	path = temp_fn_sepfix(path);

	corefile* handle = NULL;
	if (retro_vfs_version >= 3)
	{
		struct retro_vfs_file_handle* f;
		unsigned mode = RETRO_VFS_FILE_ACCESS_READ;
		if      (access == CORE_FILE_WRITE   ) mode = RETRO_VFS_FILE_ACCESS_WRITE;
		else if (access == CORE_FILE_REVISE  ) mode = RETRO_VFS_FILE_ACCESS_READ_WRITE | RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING;
		else if (access == CORE_FILE_TRUNCATE) mode = RETRO_VFS_FILE_ACCESS_READ_WRITE;
		f = retro_vfs->open(path,mode,0);
		handle = (corefile*)f;
	}
	else
	{
		FILE* f;
		const char* mode = "rb";
		if      (access == CORE_FILE_WRITE   ) mode = "wb";
		else if (access == CORE_FILE_REVISE  ) mode = "rb+";
		else if (access == CORE_FILE_TRUNCATE) mode = "wb+";
		f = fopen(path,mode);
		handle = (corefile*)f;
	}
	CFD(retro_log(RETRO_LOG_DEBUG,"core_file_open('%s',%d) = %p\n",path,access,handle));
	return handle;
}

corefile* core_file_open_system(const char* path, int access)
{
	return core_file_open(temp_fn2(system_path,path),access);
}

corefile* core_file_open_hard(const char* path, int access)
{
	if (core_hard_content) return core_file_open(path,access);
	else                   return core_file_open_system(path,access);
}

corefile* core_file_open_save(const char* path, int access)
{
	save_path_init();
	return core_file_open(temp_fn2(save_path,path),access);
}

bool core_file_exists(const char* path)
{
	CFD(retro_log(RETRO_LOG_DEBUG,"core_file_exists('%s')\n",path));
	path = temp_fn_sepfix(path);

	if (retro_vfs_version >= 3)
	{
		int vst = retro_vfs->stat(path,NULL);
		if (!(vst & RETRO_VFS_STAT_IS_VALID)) return false;
		if (vst & RETRO_VFS_STAT_IS_DIRECTORY) return false;
		return true;
	}
	else
	{
		// conditions based on hatari/src/file.c File_Exists
		struct stat fs;
		if ((0 == stat(path, &fs)) && (fs.st_mode & (S_IRUSR|S_IWUSR)) && !S_ISDIR(fs.st_mode))
		{
			return true;
		}
	}
	return false;
}

bool core_file_exists_save(const char* filename)
{
	save_path_init();
	return core_file_exists(temp_fn2(save_path,filename));
}


void core_file_close(corefile* file)
{
	CFD(retro_log(RETRO_LOG_DEBUG,"core_file_close(%p)\n",file));
	if (retro_vfs_version >= 3)
	{
		retro_vfs->close((struct retro_vfs_file_handle*)file);
	}
	else
	{
		fclose((FILE*)file);
	}
}

int core_file_seek(corefile* file, int64_t offset, int dir)
{
	CFD(retro_log(RETRO_LOG_DEBUG,"core_file_seek(%p,%d,%d)\n",file,(int)offset,dir));
	if (retro_vfs_version >= 3)
	{
		int mode = RETRO_VFS_SEEK_POSITION_START;
		if      (dir == SEEK_CUR) mode = RETRO_VFS_SEEK_POSITION_CURRENT;
		else if (dir == SEEK_END) mode = RETRO_VFS_SEEK_POSITION_END;

		if (retro_vfs->seek((struct retro_vfs_file_handle*)file,offset,mode) < 0)
			return -1;
		else
			return 0;
	}
	else
	{
		return fseek((FILE*)file,(long)offset,dir);
	}
}

int64_t core_file_tell(corefile* file)
{
	CFD(retro_log(RETRO_LOG_DEBUG,"core_file_tell(%p)\n",file));
	if (retro_vfs_version >= 3)
	{
		return retro_vfs->tell((struct retro_vfs_file_handle*)file);
	}
	else
	{
		return ftell((FILE*)file);
	}
}

int64_t core_file_read(void* buf, int64_t size, int64_t count, corefile* file)
{
	CFD(retro_log(RETRO_LOG_DEBUG,"core_file_read(%p,%d,%d,%p)\n",buf,(int)size,(int)count,file));
	if (retro_vfs_version >= 3)
	{
		int64_t result = retro_vfs->read((struct retro_vfs_file_handle*)file,buf,(size*count));
		if (result < 0) return 0;
		return result / size;
	}
	else
	{
		return fread(buf,size,count,(FILE*)file);
	}
}

int64_t core_file_write(const void* buf, int64_t size, int64_t count, corefile* file)
{
	CFD(retro_log(RETRO_LOG_DEBUG,"core_file_write(%p,%d,%d,%p)\n",buf,(int)size,(int)count,file));
	if (retro_vfs_version >= 3)
	{
		int64_t result = retro_vfs->write((struct retro_vfs_file_handle*)file,buf,(size*count));
		if (result < 0) return 0;
		return result / size;
	}
	else
	{
		return fwrite(buf,size,count,(FILE*)file);
	}
}

int core_file_flush(corefile* file)
{
	CFD(retro_log(RETRO_LOG_DEBUG,"core_file_flush(%p)\n",file));
	if (retro_vfs_version >= 3)
	{
		return retro_vfs->flush((struct retro_vfs_file_handle*)file);
	}
	else
	{
		return fflush((FILE*)file);
	}
}

int core_file_remove(const char* path)
{
	CFD(retro_log(RETRO_LOG_DEBUG,"core_file_remove('%s')\n",path));
	path = temp_fn_sepfix(path);

	if (retro_vfs_version >= 3)
	{
		// RetroArch VFS will faill to remove directories on windows (1.15.0)
		// but will probably be fixed in a later version.
		// https://github.com/libretro/RetroArch/issues/15578
		return retro_vfs->remove(path);
	}
	else
	{
		// microsoft's version of remove will give EACCESS error for directories,
		// so we must redirect to rmdir instead.
		struct stat s;
		if ((core_file_stat(path,&s) == 0) && (S_ISDIR(s.st_mode)))
		{
			return rmdir(path);
		}
		else
		{
			return remove(path);
		}
	}
}

int core_file_remove_system(const char* path)
{
	return core_file_remove(temp_fn2(system_path,path));
}

int core_file_remove_hard(const char* path)
{
	if (core_hard_content) return core_file_remove(path);
	else                   return core_file_remove_system(path);
}

int core_file_mkdir(const char* path)
{
	CFD(retro_log(RETRO_LOG_DEBUG,"core_file_mkdir('%s')\n",path));
	path = temp_fn_sepfix(path);

	if (retro_vfs_version >= 3)
	{
		return retro_vfs->mkdir(path);
	}
	else
	{
#if defined(WIN32) // gemdos.c was doing this, does Win64 really define WIN32? Does mkdir really have different numbers of parameters??
		return mkdir(path);
#else
		return mkdir(path,0755); // wow a use for octal >:P
#endif
		// (permissions the same as those used in gemdos.c)
	}
}

int core_file_mkdir_system(const char* path)
{
	return core_file_mkdir(temp_fn2(system_path,path));
}

int core_file_mkdir_hard(const char* path)
{
	if (core_hard_content) return core_file_mkdir(path);
	else                   return core_file_mkdir_system(path);
}

int core_file_rename(const char* old_path, const char* new_path)
{
	char op_fix[MAX_PATH];
	CFD(retro_log(RETRO_LOG_DEBUG,"core_file_rename('%s','%s')\n",old_path,new_path));

	strcpy_trunc(op_fix,temp_fn_sepfix(old_path),sizeof(op_fix));
	old_path = op_fix;
	new_path = temp_fn_sepfix(new_path);

	if (retro_vfs_version >= 3)
	{
		return retro_vfs->rename(old_path, new_path);
	}
	else
	{
		return rename(old_path, new_path);
	}
}

int core_file_rename_system(const char* old_path, const char* new_path)
{
	char ops[MAX_PATH];
	char nps[MAX_PATH];
	strcpy_trunc(ops,temp_fn2(system_path,old_path),sizeof(ops));
	strcpy_trunc(nps,temp_fn2(system_path,new_path),sizeof(nps));
	return core_file_rename(ops,nps);
}

int core_file_rename_hard(const char* old_path, const char* new_path)
{
	if (core_hard_content) return core_file_rename(old_path, new_path);
	else                   return core_file_rename_system(old_path, new_path);
}

int core_file_stat(const char* path, struct stat* fs)
{
	CFD(retro_log(RETRO_LOG_DEBUG,"core_file_stat('%s',%p)\n",path,fs));
	path = temp_fn_sepfix(path);

	if (retro_vfs_version >= 3)
	{
		int32_t vsize;
		int vst = retro_vfs->stat(path,&vsize);
		if (!(vst & RETRO_VFS_STAT_IS_VALID)) return -1;
		if (fs)
		{
			memset(fs,0,sizeof(struct stat));
			// gemdos.c uses:
			//   st_mtime
			//   st_atime
			//   st_mode with: S_IWURS, S_IFDIR, and S_ISDIR
			//   st_size
			fs->st_mtime = 0; // don't have this
			fs->st_atime = 0; // don't have this
			fs->st_mode |= S_IWUSR; // don't have this, assume write permissions always
			if (vst & RETRO_VFS_STAT_IS_DIRECTORY)
			{
			    fs->st_mode |= S_IFDIR;
				fs->st_mode |= S_ISDIR(~0);
			}
			if (vst & RETRO_VFS_STAT_IS_CHARACTER_SPECIAL) fs->st_mode |= S_IFCHR; // not used but retro vfs has it
			fs->st_size = (off_t)vsize;
			// note that off_t is likely 32-bit, so we may have a 2GB boundary problem for very large hard disk images
		}
		return 0;
	}
	else
	{
		return stat(path, fs);
	}
}

int core_file_stat_system(const char* path, struct stat* fs)
{
	return core_file_stat(temp_fn2(system_path,path),fs);	
}

int core_file_stat_hard(const char* path, struct stat* fs)
{
	if (core_hard_content) return core_file_stat(path, fs);
	else                   return core_file_stat_system(path, fs);
}

int64_t core_file_size(const char* path)
{
	CFD(retro_log(RETRO_LOG_DEBUG,"core_file_size('%s')\n",path));
	struct stat fs;
	if (0 == core_file_stat(path, &fs))
		return fs.st_size;
	return -1;
}

int64_t core_file_size_system(const char* path)
{
	return core_file_size(temp_fn2(system_path,path));
}

int64_t core_file_size_hard(const char* path)
{
	if (core_hard_content) return core_file_size(path);
	else                   return core_file_size_system(path);
}

static int opendir_debug_count = 0; // for diagnosing unpaired opendir/closedir

coredir* core_file_opendir(const char* path)
{
	coredir* result;
	CFD(retro_log(RETRO_LOG_DEBUG,"core_file_opendir('%s')\n",path));
	path = temp_fn_sepfix(path);
	if (retro_vfs_version >= 3)
	{
		result = (coredir*)retro_vfs->opendir(path,true);
	}
	else
	{
		result = (coredir*)opendir(path);
	}
	if (result) ++opendir_debug_count;
	//retro_log(RETRO_LOG_DEBUG,"(opendir) open directories: %d\n",opendir_debug_count);
	return result;
}

coredir* core_file_opendir_system(const char* path)
{
	return core_file_opendir(temp_fn2(system_path,path));
}

coredir* core_file_opendir_hard(const char* path)
{
	if (core_hard_content) return core_file_opendir(path);
	else                   return core_file_opendir_system(path);
}

struct coredirent* core_file_readdir(coredir* dir)
{
	// only used by gemdos.c and all we need is the name
	static struct coredirent d;
	const char* name = NULL;
	CFD(retro_log(RETRO_LOG_DEBUG,"core_file_readdir('%p')\n",dir));
	memset(&d,0,sizeof(d));
	if (retro_vfs_version >= 3)
	{
		if (!retro_vfs->readdir((struct retro_vfs_dir_handle*)dir))
			return NULL;
		name = retro_vfs->dirent_get_name((struct retro_vfs_dir_handle*)dir);
	}
	else
	{
		struct dirent* df;
		df = readdir((DIR*)dir);
		if (df == NULL)
			return NULL;
		name = df->d_name;
	}
	if (name)
		strcpy_trunc(d.d_name,name,sizeof(d.d_name));
	return &d;
}

int core_file_closedir(coredir* dir)
{
	int result;
	CFD(retro_log(RETRO_LOG_DEBUG,"core_file_closedir('%p')\n",dir));
	if (retro_vfs_version >= 3)
	{
		result = retro_vfs->closedir((struct retro_vfs_dir_handle*)dir);
	}
	else
	{
		result = closedir((DIR*)dir);
	}
	if (result == 0) --opendir_debug_count;
	//retro_log(RETRO_LOG_DEBUG,"(closedir) open directories: %d\n",opendir_debug_count);
	return result;
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
	//retro_log(RETRO_LOG_DEBUG,"core_file_system_add: %s\n",sf_filename[sf_count]);
	++sf_count;
}

static void core_file_system_add_dir(const char* filename)
{
	if (sf_dir_count >= MAX_SYSTEM_DIR) return;
	if (!strcmp(filename,".")) return;
	if (!strcmp(filename,"..")) return;
	strcpy_trunc(sf_dirname[sf_dir_count],"hatarib/",MAX_FILENAME);
	strcat_trunc(sf_dirname[sf_dir_count],filename,MAX_FILENAME);
	strcpy_trunc(sf_dirlabel[sf_dir_count],filename,MAX_FILENAME);
	strcat_trunc(sf_dirlabel[sf_dir_count],"/",MAX_FILENAME);
	//retro_log(RETRO_LOG_DEBUG,"core_file_system_add_dir: %s\n",sf_filename[sf_count]);
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
		//retro_log(RETRO_LOG_DEBUG,"GET_SYSTEM_DIRECTORY success\n");
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
