#ifdef __LIBRETRO__
/*
  Hatari - drivesound.c

  Floppy disk drive sound emulation.
*/

const char DriveSound_fileid[] = "Hatari drivesound.c";

#include "main.h"
#include "drivesound.h"
#include "cycles.h"

/*-----------------------------------------------------------------------*/
/**
 * Initialize variables.
 */
void	DriveSound_Init ( void )
{
	// TODO
	DriveSound_Reset(CyclesGlobalClockCounter);
}

/*-----------------------------------------------------------------------*/
/**
 * Reset DriveSound timing and regenerate/reload samples if needed.
 */
void	DriveSound_Reset ( uint64_t cycle )
{
	// check samplerate
	// regenerate/reload samples if settings have changed
	(void)cycle;
}

/*-----------------------------------------------------------------------*/
/**
 * Free allocated memory.
 */
void	DriveSound_UnInit ( void )
{
	// TODO
}

/*-----------------------------------------------------------------------*/
/**
 * Save or restore the DriveSound state.
 */
void	DriveSound_MemorySnapShot_Capture ( bool bSave )
{
	(void)bSave;
	// TODO
}

/*-----------------------------------------------------------------------*/
/**
 * Process a motor state change.
 */
void	DriveSound_Motor ( uint64_t cycle, int drive, bool on )
{
	(void)cycle;
	(void)drive;
	(void)on;
}

/*-----------------------------------------------------------------------*/
/**
 * Process a track step, attempted step, or just update the track state.
 */
void	DriveSound_Track ( uint64_t cycle, int drive, int track, int step )
{
	(void)cycle;
	(void)drive;
	(void)track;
	(void)step;
	// TODO
}

/*-----------------------------------------------------------------------*/
/**
 * Render the drive sound to the audio buffer.
 */
void	DriveSound_Render ( uint64_t cycle, int16_t samples[][2], int index, int length )
{
	// TODO
	// sounds are rendered as stereo pair based on panning settings (separate sounds for left and right drive)
	// two drives, each with a motor sound + step sound
	// motor has startup, spin loop (300RPM), release
	//   start spin loop automatically after startup, start release at end of next spin loop when ready
	//   if startup when releasing, just queue it after release
	// step has step up, step down, and maybe a blocked step sound
	// step should have a fade down if cancelling a playing step sound, otherwise plays to end of sound
	//   copy the current sound to a secondary fadeout and process that as the new one plays on the main channel
	// volume setting should be a mix percentage, e.g. 25% drivesound, 75% original audio
}

#endif
