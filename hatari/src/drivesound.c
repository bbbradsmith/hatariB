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
#include "memorySnapShot.h"

/*-----------------------------------------------------------------------*/

/* fixed point sound mix */
#define MIX_BITS                 12

/* maximum number of disk sound events in a frame */
#define EVENT_MAX                16

/*
 * Synthesized sound parameters
 *
 * SYN_MOTOR_RPM gives the length of the motor sound loop
 * SYN_MOTOR_SPINUP is ms to accelerate from 0 to RPM
 * SYN_MOTOR_SPINDOWN is ms to come to a stop
 * SYN_OVERLAP is microseconds of overlap fadeout to avoid sample pops
 * 
 */
#define SYN_MOTOR_RPM           300
#define SYN_MOTOR_SPINUP        300
#define SYN_MOTOR_SPINDOWN       30
#define SYN_OVERLAP            5000
// TODO more properties

#define SAMPLE_SEEK_VARIATIONS   10

#define SAMPLE_SPINUP             0
#define SAMPLE_SPIN               1
#define SAMPLE_SPINDOWN           2
#define SAMPLE_SEEKUP             3
#define SAMPLE_SEEKDOWN           (SAMPLE_SEEKUP+SAMPLE_SEEK_VARIATIONS)
#define SAMPLE_COUNT              (SAMPLE_SEEKDOWN+SAMPLE_SEEK_VARIATIONS)

#define SAMPLER_MOTOR             0
#define SAMPLER_SEEK              2
#define SAMPLER_OVERLAP           4
#define SAMPLER_COUNT             6

#define MOTOR_PHASE_OFF           0
#define MOTOR_PHASE_SPINUP        1
#define MOTOR_PHASE_SPIN          2
#define MOTOR_PHASE_SPINDOWN      3

#define EVENT_NONE                0
#define EVENT_SEEKUP              3
#define EVENT_SEEKDOWN            4

typedef struct {
	bool      UseWAV;
	int       SampleRate;
	int       Pan[2];
	int       FadeTime;
} DS_CONFIG;

typedef struct {
	int16_t*  Samples;     /* consecutive left/right sample pairs */
	int       Length;      /* length in samples */
	bool      Allocated;   /* true if it must be freed */
} DS_SAMPLE;

typedef struct {
	int16_t*  Samples;
	int       Remain;
	int       Fade;
	int       Index;
} DS_SAMPLER;

typedef struct {
	uint64_t  Cycle;       /* logged emulation cycle timing */
	int       Sample;      /* set by DriveSound_TimeEvents */
	int       Drive;
	int       Event;
} DS_EVENT;

static bool       Initialized = false;
static DS_CONFIG  Config;
static DS_SAMPLE  Sample[2][SAMPLE_COUNT];
static DS_SAMPLER Sampler[SAMPLER_COUNT];
static int        MotorPhase[2];
static bool       MotorOn[2];
static uint32_t   Seed;

static DS_EVENT   Event[EVENT_MAX];
static int        EventCount = 0;
static int        EventNext = 0;
static uint64_t   LastCycle = 0;

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
	memset(&Sampler, 0, sizeof(Sampler));
	memset(&MotorPhase, 0, sizeof(MotorPhase));
	memset(&Event, 0, sizeof(Event));
	EventCount = 0;
	EventNext = 0;
	Seed = 1;
	DriveSound_Reset(CyclesGlobalClockCounter);
}

/*-----------------------------------------------------------------------*/
/**
 * Reset DriveSound timing.
 */
void	DriveSound_Reset ( uint64_t cycle )
{
	LastCycle = cycle;
	memset(&MotorOn, 0, sizeof(MotorOn));
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
	if (
		Initialized &&
		(ConfigureParams.Sound.nPlaybackFreq == Config.SampleRate) &&
		(ConfigureParams.Sound.bDriveSoundWav == Config.UseWAV) &&
		(ConfigureParams.Sound.nDriveSoundPan[0] == Config.Pan[0]) &&
		(ConfigureParams.Sound.nDriveSoundPan[1] == Config.Pan[1]) )
	{
		return; /* already configured */
	}

	/* free allocated memory */
	DriveSound_UnInit();
	memset(&Sample, 0, sizeof(Sample));
	memset(&Sampler, 0, sizeof(Sampler));

	Config.SampleRate = ConfigureParams.Sound.nPlaybackFreq;
	Config.UseWAV = ConfigureParams.Sound.bDriveSoundWav;
	Config.Pan[0] = ConfigureParams.Sound.nDriveSoundPan[0];
	Config.Pan[1] = ConfigureParams.Sound.nDriveSoundPan[1];
	Config.FadeTime = (Config.SampleRate * SYN_OVERLAP) / 1000000;

	// TODO regenerate/reload samples
	// load WAVs if requested
	// load seekup0-9 until one is missing
	// TODO is RoadBlasters crashing???
	Initialized = true;
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
	memset(&Sample, 0, sizeof(Sample));
	memset(&Sampler, 0, sizeof(Sampler));
	Initialized = false;
	Config.SampleRate = 0;
}

/*-----------------------------------------------------------------------*/
/**
 * Save or restore the DriveSound state.
 */
void	DriveSound_MemorySnapShot_Capture ( bool bSave )
{
	int i;
	uint32_t offset;
	bool active = Initialized;

	MemorySnapShot_Store(&active,sizeof(active));
	if (!active)
	{
		if (!bSave)
			DriveSound_UnInit();
		return;
	}
	if (!bSave)
		DriveSound_Reconfigure();

	MemorySnapShot_Store(&MotorPhase,sizeof(MotorPhase));
	MemorySnapShot_Store(&MotorOn,sizeof(MotorOn));
	MemorySnapShot_Store(&Seed,sizeof(Seed));
	MemorySnapShot_Store(&Event,sizeof(Event));
	MemorySnapShot_Store(&EventCount,sizeof(EventCount));
	MemorySnapShot_Store(&LastCycle,sizeof(LastCycle));
	for (i=0; i < SAMPLER_COUNT; ++i)
	{
		int d = i % 2;
		MemorySnapShot_Store(&Sampler[i].Remain,sizeof(Sampler[i].Remain));
		MemorySnapShot_Store(&Sampler[i].Index,sizeof(Sampler[i].Index));
		MemorySnapShot_Store(&Sampler[i].Fade,sizeof(Sampler[i].Fade));
		/* don't store a  pointer to samples, recover it */
		if (bSave)
		{
			const int16_t* pos_sample = Sampler[i].Samples;
			const int16_t* base_sample = Sample[d][Sampler[i].Index].Samples;
			offset = (uint32_t)(pos_sample - base_sample);
			MemorySnapShot_Store(&offset,sizeof(offset));
		}
		else
		{
			MemorySnapShot_Store(&offset,sizeof(offset));
			Sampler[i].Samples = Sample[d][Sampler[i].Index].Samples + offset;
			/* make sure the changing the WAV size won't go out of bounds */
			if ((int)offset + Sampler[i].Remain > Sample[d][Sampler[i].Index].Length)
			{
				Sampler[i].Samples = NULL;
				Sampler[i].Remain = 0;
			}
		}
	}
}

/*-----------------------------------------------------------------------*/
/**
 * Process a motor state change.
 */
void	DriveSound_Motor ( uint64_t cycle, int drive, bool on )
{
	if (!ConfigureParams.Sound.bEnableDriveSound)
		return;

	(void)cycle; /* due to low RPM frame-accurate is okay, don't need to track events */
	MotorOn[drive^0] = on;
	if (on)
	{
		MotorOn[drive^1] = false; /* only one drive can be on at a time */
	}
}

/*-----------------------------------------------------------------------*/
/**
 * Process a track step, attempted step, or just update the track state.
 */
void	DriveSound_Track ( uint64_t cycle, int drive, int track, int step )
{
	if (!ConfigureParams.Sound.bEnableDriveSound)
		return;
	if (EventCount >= EVENT_MAX)
		return;

	Event[EventCount].Cycle = cycle;
	Event[EventCount].Drive = drive;
	Event[EventCount].Event = (step >= 0) ? EVENT_SEEKDOWN : EVENT_SEEKUP;
	Event[EventCount].Sample = 0; /* DriveSound_TimeEvents will fill this */
	(void)track; /* not needed for this implementation, might be needed eventually */
	++EventCount;
}

/*-----------------------------------------------------------------------*/
/**
 * Called by DriveSound_Render, generates sample timing for events
 * logged since last render.
 */
static void	DriveSound_TimeEvents ( uint64_t cycle, int samples )
{
	int i;
	uint64_t cycle_delta = (cycle >= LastCycle) ? (cycle - LastCycle) : 0;
	if (cycle_delta == 0) /* If no cycles elapsed, flatten all timing to 0 samples, prevent division by zero */
	{
		cycle_delta = 1;
		samples = 0;
	}
	for (i=0; i<EventCount; ++i)
	{
		Event[i].Sample = (int)(((Event[i].Cycle - LastCycle) * (uint64_t)samples) / cycle_delta);
		if (Event[i].Sample < 0 || Event[i].Sample > samples) /* Treat unexpected timings as beginning of frame */
			Event[i].Sample = 0;
	}
	LastCycle = cycle; /* Mark starting cycle for next frame */
	EventNext = 0;
}

/*-----------------------------------------------------------------------*/
/**
 * Called by DriveSound_RenderSegment, initiates a sample playback.
 */
static inline void	DriveSound_SetSample ( int drive, int sampler, int sample )
{
	Sampler[sampler+drive].Samples = Sample[drive][sample].Samples;
	Sampler[sampler+drive].Remain  = Sample[drive][sample].Length;
	Sampler[sampler+drive].Fade    = 0;
	Sampler[sampler+drive].Index   = sample;
}

/*-----------------------------------------------------------------------*/
/**
 * Called by DriveSound_RenderSegment, initiates a sample playback,
 * but transfers to overlap
 */
static inline void	DriveSound_SetOverlapSample ( int drive, int sampler, int overlap, int sample )
{
	if (Sampler[sampler+drive].Remain > 0 && Sampler[sampler+drive].Fade > 0)
	{
		/* if a sample is already playing, move it to the overlap fadeout channel */
		Sampler[overlap+drive] = Sampler[sampler+drive];
	}
	Sampler[sampler+drive].Samples = Sample[drive][sample].Samples;
	Sampler[sampler+drive].Remain  = Sample[drive][sample].Length;
	Sampler[sampler+drive].Fade    = 0; /* 0 marks that it hasn't played any samples yet */
	Sampler[sampler+drive].Index   = sample;
}

/*-----------------------------------------------------------------------*/
/**
 * Called by DriveSound_Render, adds the floppy drive sound
 * to a contiguous segment of the audio buffer.
 */
static void	DriveSound_RenderSegment ( int16_t samples[][2], int length, int offset )
{
	int i,d,si;
	int32_t ds[2];
	const int32_t mix_ds = (ConfigureParams.Sound.nDriveSoundMix * (1 << MIX_BITS)) / 1000; /* convert mix 0-1000 to fixed point multiplier */
	const int32_t mix_st = (1 << MIX_BITS) - mix_ds;

	i = 0;
	while (i < length)
	{
		int max_span = length - i;
		int span = max_span; /* If no more events, process all remaining samples */
		while (EventNext < EventCount)
		{
			if ((i + offset) < Event[EventNext].Sample) /* Future event, run samples until it is reached */
			{
				span = Event[EventNext].Sample - (i + offset);
				if (span > max_span)
					span = max_span;
				break;
			}
			/* Due event, process now */
			switch (Event[EventNext].Event)
			{
			default:
			case EVENT_NONE:
				break;
			case EVENT_SEEKUP:
				DriveSound_SetOverlapSample(Event[EventNext].Drive, SAMPLER_SEEK, SAMPLER_OVERLAP, SAMPLE_SEEKUP);
				// TODO randomized seekup
				// random should be internal/deterministic and savestated
				// (Seed variable is already savestated)
				break;
			case EVENT_SEEKDOWN:
				DriveSound_SetOverlapSample(Event[EventNext].Drive, SAMPLER_SEEK, SAMPLER_OVERLAP, SAMPLE_SEEKDOWN);
				// TODO randomized seekdown
				break;
			}
			++EventNext;
		}

		/* motor sound sequence */
		for (d=0; d<2; ++d)
		{
			if (Sampler[SAMPLER_MOTOR+d].Remain > 0) /* stop span at next motor sample change */
			{
				if (Sampler[SAMPLER_MOTOR+d].Remain < span)
					span = Sampler[SAMPLER_MOTOR+d].Remain;
			}
			else /* motor sound is finished, advance sequence */
			{
				switch (MotorPhase[d])
				{
				default:
				case MOTOR_PHASE_OFF:
					if (MotorOn[d]) /* start up if turned on */
					{
						DriveSound_SetSample(d,SAMPLER_MOTOR,SAMPLE_SPINUP);
						MotorPhase[d] = MOTOR_PHASE_SPINUP;
					}
					/* otherwise can stay off */
					break;
				case MOTOR_PHASE_SPINUP:
					if (MotorOn[d]) /* proceed to steady spin */
					{
						DriveSound_SetSample(d,SAMPLER_MOTOR,SAMPLE_SPIN);
						MotorPhase[d] = MOTOR_PHASE_SPIN;
					}
					else /* turned off */
					{
						DriveSound_SetSample(d,SAMPLER_MOTOR,SAMPLE_SPINDOWN);
						MotorPhase[d] = MOTOR_PHASE_SPINDOWN;
					}
					break;
				case MOTOR_PHASE_SPIN:
					if (MotorOn[d]) /* keep spinning */
					{
						DriveSound_SetSample(d,SAMPLER_MOTOR,SAMPLE_SPIN);
					}
					else /* turned off */
					{
						DriveSound_SetSample(d,SAMPLER_MOTOR,SAMPLE_SPINDOWN);
						MotorPhase[d] = MOTOR_PHASE_SPINDOWN;
					}
					break;
				case MOTOR_PHASE_SPINDOWN:
					if (MotorOn[d]) /* start up again */
					{
						DriveSound_SetSample(d,SAMPLER_MOTOR,SAMPLE_SPINUP);
						MotorPhase[d] = MOTOR_PHASE_SPINUP;
					}
					/* otherwise is finished */
					break;
				}
			}
		}

		/* Fade marks SEEK samplers as having played at least one sample */
		if (span > 0)
		{
			for (si=SAMPLER_SEEK; si<(SAMPLER_SEEK+2); ++si)
			{
				if (Sampler[si].Remain > 0)
					Sampler[si].Fade = Config.FadeTime;
			}
		}

		for (; span > 0; ++i, --span)
		{
			/* fetch and attenuate previous sound */
			int32_t s[2] = {
				(int32_t)(samples[i][0]) * mix_st,
				(int32_t)(samples[i][1]) * mix_st
			};

			ds[0] = 0;
			ds[1] = 0;
			/* mix drive sound samples (no fadeout) */
			for (si=0; si<SAMPLER_OVERLAP; ++si)
			{
				if (Sampler[si].Remain > 0)
				{
					ds[0] += (int32_t)(Sampler[si].Samples[0]);
					ds[1] += (int32_t)(Sampler[si].Samples[1]);
					Sampler[si].Samples += 2;
					Sampler[si].Remain -= 1;
				}
			}
			/* mix overlapping samples with fadeout */
			for (si=SAMPLER_OVERLAP; si<SAMPLER_COUNT; ++si)
			{
				if (Sampler[si].Remain > 0)
				{
					ds[0] += (((int32_t)(Sampler[si].Samples[0])) * Sampler[si].Fade) / Config.FadeTime;
					ds[1] += (((int32_t)(Sampler[si].Samples[1])) * Sampler[si].Fade) / Config.FadeTime;
					Sampler[si].Samples += 2;
					Sampler[si].Remain -= 1;
					Sampler[si].Fade -= 1;
					if (Sampler[si].Fade <= 0)
						Sampler[si].Remain = 0;
				}
			}

			/* mix with previous sound */
			s[0] += ds[0] * mix_ds;
			s[1] += ds[1] * mix_ds;
			samples[i][0] = (int16_t)(s[0] >> MIX_BITS);
			samples[i][1] = (int16_t)(s[1] >> MIX_BITS);
		}
	}
}

/*-----------------------------------------------------------------------*/
/**
 * Render the drive sound to the audio buffer.
 */
void	DriveSound_Render ( uint64_t cycle, int16_t samples[][2], int index, int length )
{
	if (!ConfigureParams.Sound.bEnableDriveSound || !Initialized)
		return;

	DriveSound_TimeEvents(cycle, length); /* Prepare the event timings */

	if ((index + length) > AUDIOMIXBUFFER_SIZE) /* Ring buffering may require 2 segments */
	{
		int split_length = AUDIOMIXBUFFER_SIZE - index;
		DriveSound_RenderSegment(samples + index, split_length, 0);
		DriveSound_RenderSegment(samples, length - split_length, split_length);
	}
	else
	{
		DriveSound_RenderSegment(samples + index, length, 0);
	}
	EventCount = 0;
}

#endif // __LIBRETRO__
