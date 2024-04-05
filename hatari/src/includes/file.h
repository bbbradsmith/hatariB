/*
  Hatari - file.h

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.
*/

#ifndef HATARI_FILE_H
#define HATARI_FILE_H

#include "config.h"
#include <sys/types.h>		/* Needed for off_t */

#ifndef HAVE_FSEEKO
#define fseeko fseek
#endif
#ifndef HAVE_FTELLO
#define ftello ftell
#endif

extern void File_CleanFileName(char *pszFileName);
extern void File_AddSlashToEndFileName(char *pszFileName);
extern bool File_DoesFileExtensionMatch(const char *pszFileName, const char *pszExtension);
extern bool File_ChangeFileExtension(const char *Filename_old, const char *Extension_old , char *Filename_new , const char *Extension_new);
extern const char *File_RemoveFileNameDrive(const char *pszFileName);
extern bool File_DoesFileNameEndWithSlash(char *pszFileName);
extern Uint8 *File_ZlibRead(const char *pszFileName, long *pFileSize);
extern Uint8 *File_ReadAsIs(const char *pszFileName, long *pFileSize);
extern Uint8 *File_Read(const char *pszFileName, long *pFileSize, const char * const ppszExts[]);
extern bool File_Save(const char *pszFileName, const Uint8 *pAddress, size_t Size, bool bQueryOverwrite);
extern off_t File_Length(const char *pszFileName);
extern bool File_Exists(const char *pszFileName);
extern bool File_DirExists(const char *psDirName);
extern bool File_QueryOverwrite(const char *pszFileName);
extern char* File_FindPossibleExtFileName(const char *pszFileName,const char * const ppszExts[]);
extern void File_SplitPath(const char *pSrcFileName, char *pDir, char *pName, char *Ext);
extern char* File_MakePath(const char *pDir, const char *pName, const char *pExt);
extern int File_MakePathBuf(char *buf, size_t buflen, const char *pDir,
                            const char *pName, const char *pExt);
extern void File_ShrinkName(char *pDestFileName, const char *pSrcFileName, int maxlen);
extern FILE *File_Open(const char *path, const char *mode);
extern FILE *File_Close(FILE *fp);
extern bool File_Lock(FILE *fp);
extern void File_UnLock(FILE *fp);
extern bool File_InputAvailable(FILE *fp);
extern void File_MakeAbsoluteSpecialName(char *pszFileName);
extern void File_MakeAbsoluteName(char *pszFileName);
extern void File_MakeValidPathName(char *pPathName);
extern void File_PathShorten(char *path, int dirs);
extern void File_HandleDotDirs(char *path);
#if defined(WIN32)
extern char* WinTmpFile(void);
#endif
#ifdef __LIBRETRO__
// simulated low level access
#define CORE_FILE_READ       0
#define CORE_FILE_WRITE      1
#define CORE_FILE_REVISE     2
#define CORE_FILE_TRUNCATE   3
#define CORE_FILE_READ       0
#define CORE_FILE_WRITE      1
#define CORE_FILE_REVISE     2
#define CORE_FILE_TRUNCATE   3
struct stat;
extern int core_hard_readonly;
extern corefile* core_file_open(const char* path, int access);
extern corefile* core_file_open_system(const char* path, int access);
extern corefile* core_file_open_hard(const char* path, int access);
extern corefile* core_file_open_save(const char* path, int access);
extern bool core_file_exists(const char* path); // returns true if file exists and is not a directory (and is read or writable)
extern bool core_file_exists_save(const char* filename);
extern void core_file_close(corefile* file);
extern int core_file_seek(corefile* file, int64_t offset, int dir);
extern int64_t core_file_tell(corefile* file);
extern int64_t core_file_read(void* buf, int64_t size, int64_t count, corefile* file);
extern int64_t core_file_write(const void* buf, int64_t size, int64_t count, corefile* file);
extern int core_file_flush(corefile* file);
extern int core_file_remove(const char* path);
extern int core_file_remove_system(const char* path);
extern int core_file_remove_hard(const char* path);
extern int core_file_mkdir(const char* path);
extern int core_file_mkdir_system(const char* path);
extern int core_file_mkdir_hard(const char* path);
extern int core_file_rename(const char* old_path, const char* new_path);
extern int core_file_rename_system(const char* old_path, const char* new_path);
extern int core_file_rename_hard(const char* old_path, const char* new_path);
extern int core_file_stat(const char* path, struct stat* fs);
extern int core_file_stat_system(const char* path, struct stat* fs);
extern int core_file_stat_hard(const char* path, struct stat* fs);
extern int64_t core_file_size(const char* path);
extern int64_t core_file_size_system(const char* path);
extern int64_t core_file_size_hard(const char* path);
extern coredir* core_file_opendir(const char* path);
extern coredir* core_file_opendir_system(const char* path);
extern coredir* core_file_opendir_hard(const char* path);
extern struct coredirent* core_file_readdir(coredir* dir);
extern int core_file_closedir(coredir* dir);
// replaces File_Read
extern uint8_t* core_read_file_system(const char* filename, unsigned int* size_out);
extern uint8_t* core_read_file_hard(const char* filename, unsigned int* size_out);
#endif

#endif /* HATARI_FILE_H */
