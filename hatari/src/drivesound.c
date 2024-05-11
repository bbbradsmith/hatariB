#ifdef __LIBRETRO__
/*
  Hatari - drivesound.c

  Floppy disk drive sound emulation.

  Prototype implementation by NTxC:
    https://github.com/bbbradsmith/hatariB/issues/33
  Video by Stefan Lindberg showingm drive sound and operation, used as reference:
    https://www.youtube.com/watch?v=X_JCWWHKXIc
*/

const char DriveSound_fileid[] = "Hatari drivesound.c";

#include "main.h"
#include "drivesound.h"
#include "cycles.h"
#include "sound.h"
#include "configuration.h"

/*-----------------------------------------------------------------------*/

/* fixed point sound mix */
#define DS_MIX_BITS   12

/*
 * Synthesized sound parameters
 *
 * RPM gives the length of the motor sound loop
 * MOTOR_SPINUP is ms to accelerate from 0 to RPM
 * MOTOR_SPINDOWN is ms to 
 * 
 */
#define DS_MOTOR_RPM        300
#define DS_MOTOR_SPINUP     300
#define DS_MOTOR_SPINDOWN    30
// TODO more properties

#define SAMPLE_SEEK_VARIATIONS 10

#define SAMPLE_SPINUP         0
#define SAMPLE_SPIN           1
#define SAMPLE_SPINDOWN       2
#define SAMPLE_SEEKUP         3
#define SAMPLE_SEEKDOWN       (SAMPLE_SEEKUP+SAMPLE_SEEK_VARIATIONS)
#define SAMPLE_COUNT          (SAMPLE_SEEKDOWN+SAMPLE_SEEK_VARIATIONS)

typedef struct {
	bool      UseWAV;
	int       SampleRate;
	int       Pan[2];
} DS_CONFIG;

typedef struct {
	int16_t*  Samples;     /* consecutive left/right sample pairs */
	int       Length;      /* length in samples */
	bool      Allocated;   /* true if it must be freed */
} DS_SAMPLE;

static bool       Initialized = false;
static DS_CONFIG  Config;
static uint64_t   LastCycle;
static DS_SAMPLE  Sample[2][SAMPLE_COUNT];

/*-----------------------------------------------------------------------*/
/**
 * Initialize variables.
 */
void	DriveSound_Init ( void )
{
	if (Initialized)
		DriveSound_UnInit();
	memset(&Config, 0, sizeof(Config));
	memset(&Sample, 0, sizeof(Sample));
	DriveSound_Reset(CyclesGlobalClockCounter);
}

/*-----------------------------------------------------------------------*/
/**
 * Reset DriveSound timing.
 */
void	DriveSound_Reset ( uint64_t cycle )
{
	LastCycle = cycle;
	DriveSound_Reconfigure();
}

/*-----------------------------------------------------------------------*/
/**
 * Regenerate/reload DriveSound samples if needed.
 */
void	DriveSound_Reconfigure ( void )
{
	if (!ConfigureParams.Sound.bEnableDriveSound)
	{
		DriveSound_UnInit();
		return;
	}
	// compare ConfigureParams vs. Configure
	// regenerate/reload samples if settings have changed
	// load WAVs if requested
	// load seekup0-9 until one is missing
	// TODO
	// TODO is RoadBlasters crashing???
}

/*-----------------------------------------------------------------------*/
/**
 * Free allocated memory.
 */
void	DriveSound_UnInit ( void )
{
	int i,d;
	if (!Initialized)
		return;
	for (i=0; i<SAMPLE_COUNT; ++i)
	{
		for (d=0; d<2; ++d)
		{
			if (Sample[d][i].Allocated)
				free(Sample[d][i].Samples);
		}
	}
	Initialized = false;
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
	if (!ConfigureParams.Sound.bEnableDriveSound)
		return;

	(void)cycle;
	(void)drive;
	(void)on;
	// TODO
	// note only one drive at a time can be on
}

/*-----------------------------------------------------------------------*/
/**
 * Process a track step, attempted step, or just update the track state.
 */
void	DriveSound_Track ( uint64_t cycle, int drive, int track, int step )
{
	if (!ConfigureParams.Sound.bEnableDriveSound)
		return;

	(void)cycle;
	(void)drive;
	(void)track;
	(void)step;
	// TODO
}

/*-----------------------------------------------------------------------*/
/**
 * Called by DriveSound_Render, adds the floppy drive sound
 * to a contiguous segment of the audio buffer.
 * TODO The seek event table has been prepared.
 */
static void	DriveSound_RenderSegment ( int16_t samples[][2], int length )
{
	int i,d;
	int32_t ds[2];
	int32_t mix_ds = (ConfigureParams.Sound.nDriveSoundMix * (1 << DS_MIX_BITS)) / 1000; /* convert mix 0-1000 to fixed point multiplier */
	int32_t mix_st = (1 << DS_MIX_BITS) - mix_ds;
	// TODO ((samples * mix_st) + (drivesound * mix_ds)) >> DS_MIX_BITS

	for (i=0; i<length; ++i)
	{
		/* fetch and attenuate previous sound */
		int32_t s[2] = {
			(int32_t)(samples[i][0]) * mix_st,
			(int32_t)(samples[i][1]) * mix_st
		};

		ds[0] = 0;
		ds[1] = 0;

		for (d=0; d<2; ++d)
		{
			// TODO add spin sound to ds
		}

		// TODO if seek event, start new seek

		for (d=0; d<2; ++d)
		{
			// TODO add seek sounds to ds
		}

		s[0] += ds[0] * mix_ds;
		s[1] += ds[1] * mix_ds;

		samples[i][0] = (int16_t)(s[0] >> DS_MIX_BITS);
		samples[i][1] = (int16_t)(s[1] >> DS_MIX_BITS);
	}

	// TODO
	// sounds are rendered as stereo pair based on panning settings (separate sounds for left and right drive)
	// two drives, each with a motor sound + step sound
	// motor has startup, spin loop (300RPM), release
	//   start spin loop automatically after startup, start release at end of next spin loop when ready
	//   if startup when releasing, just queue it after release
	// step has step up, step down
	// step should have a fade down if cancelling a playing step sound, otherwise plays to end of sound
	//   copy the current sound to a secondary fadeout and process that as the new one plays on the main channel
}

/*-----------------------------------------------------------------------*/
/**
 * Render the drive sound to the audio buffer.
 */
void	DriveSound_Render ( uint64_t cycle, int16_t samples[][2], int index, int length )
{
	if (!ConfigureParams.Sound.bEnableDriveSound)
		return;

	if ((index + length) > AUDIOMIXBUFFER_SIZE) /* Ring buffering may require 2 segments*/
	{
		int split_length = AUDIOMIXBUFFER_SIZE - index;
		DriveSound_RenderSegment(samples + index, split_length);
		length -= split_length;
		index = 0;
		// TODO shift the seek event table by split length
	}
	DriveSound_RenderSegment(samples + index, length);
}

#endif
