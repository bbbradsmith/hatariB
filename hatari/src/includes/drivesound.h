#ifdef __LIBRETRO__
/*
  Hatari - drivesound.h
*/

#ifndef HATARI_DRIVESOUND_H
#define HATARI_DRIVESOUND_H

extern void	DriveSound_Init ( void );
extern void	DriveSound_Reset ( uint64_t cycle );
extern void	DriveSound_UnInit ( void );
extern void	DriveSound_MemorySnapShot_Capture ( bool bSave );
extern void	DriveSound_Motor ( uint64_t cycle, int drive, bool on );
extern void	DriveSound_Track ( uint64_t cycle, int drive, int track, int step );
extern void	DriveSound_State ( uint64_t cycle, int drive, int track, int step, bool motor );
extern void	DriveSound_Render ( uint64_t cycle, int16_t samples[][2], int index, int length );

#endif
#endif // __LIBRETRO__
