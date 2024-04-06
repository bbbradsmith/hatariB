/*
  Hatari - memorySnapShot.h

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.
*/


extern void MemorySnapShot_Skip(int Nb);
#ifndef __LIBRETRO__
extern void MemorySnapShot_Store(void *pData, int Size);
#endif
extern void MemorySnapShot_Capture(const char *pszFileName, bool bConfirm);
extern void MemorySnapShot_Capture_Immediate(const char *pszFileName, bool bConfirm);
extern void MemorySnapShot_Capture_Do(void);
extern void MemorySnapShot_Restore(const char *pszFileName, bool bConfirm);
extern void MemorySnapShot_Restore_Do(void);

#ifdef __LIBRETRO__
// inline implementation to accelerate memory snapshots
extern bool bCaptureSave;
inline void MemorySnapShot_Store(void *pData, int Size)
{
	if (bCaptureSave) core_snapshot_write(pData, Size);
  else              core_snapshot_read( pData, Size);
}
inline void MemorySnapShot_StoreFilename(char *pData, int Size)
{
  if (Size > CORE_MAX_FILENAME) Size = CORE_MAX_FILENAME;
	if (bCaptureSave)
  {
    core_snapshot_write(pData, Size-1);
  }
  else
  {
    core_snapshot_read(pData, Size-1);
    pData[Size-1] = 0;
  }
}
#endif
