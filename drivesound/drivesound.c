
/*
*********************************
* NTxC's DriveSound for hatariB *
*********************************
*/

#ifdef __DRIVESOUND__

#include "../hatari/src/includes/main.h"
#include "../core/core.h"

#include "../drivesound/drivesound.h"

typedef struct drivesound_snd_s
{
	void *buf;			// SDL: Mix_Chunk or Mix_Music, MIX: Uint8[]
	const char *name;
	Uint8 *data;
	int size;
	int playing;
	int position;
	int num_pcm_samples;
}
drivesound_snd_t;

typedef struct wav_header_s
{
	// RIFF header
	char riff_header[ 4 ];
	int wav_size;
	char wave_header[ 4 ];

	// format header
	char fmt_header[ 4 ];
	int fmt_chunk_size;
	short audio_format;
	short num_channels;
	int sample_rate;
	int byte_rate;
	short sample_alignment;
	short bit_depth;

	// data
	char data_header[ 4 ];
	int data_bytes;
	// Uint8 bytes[];
}
wav_header_t;

#if 0
#ifdef DRIVESOUND_DEBUG_PRINT

#define	MAX_VA_STRING	32000
#define MAX_VA_BUFFERS 4

int Q_vsnprintf( char *str, int size, const char *format, va_list ap )
{
	int retval;
	retval = _vsnprintf( str, size, format, ap );
	if( retval < 0 || retval == size )
	{
		str[ size - 1 ] = '\0';
		return size;
	}
	return retval;
}

#if defined( _MSC_VER )
char *__cdecl va( const char *format, ... )
#elif defined( __GNUC__ )
__attribute__ ((cdecl)) char *va( const char *format, ... )
#endif
{
	va_list		argptr;
	static char	string[ MAX_VA_BUFFERS ][ MAX_VA_STRING ];
	static int	index = 0;
	char *buf;

	va_start( argptr, format );
	buf = (char *)&string[ index++ & 3 ];
	Q_vsnprintf( buf, MAX_VA_STRING, format, argptr );
	va_end( argptr );

	return buf;
}
#endif

#endif

bool drivesound_enable = true;	// read from RetroArch settings
int drivesound_volume = 100;	// read from RetroArch settings

int drivesound_loaded = 0;		// can the sounds actually be played?
int drivesound_samplerate = 48000;

#define DRIVESOUND_PATH_PREFIX "system/drivesound"

drivesound_snd_t g_drivesound_snd[ DRIVESOUND_MAX ] =
{
	{ NULL,	"drive_startup", NULL, 0, 0, 0 },
	{ NULL,	"drive_click", NULL, 0, 0, 0 },
	{ NULL,	"drive_seek_back", NULL, 0, 0, 0 },
	{ NULL,	"drive_seek_fwd", NULL, 0, 0, 0 },
	{ NULL,	"drive_spin", NULL, 0, 0, 0 }
};

bool drivesound_is_allowed( void )
{
	return drivesound_enable && drivesound_loaded != 0;
}

int drivesound_play_from_track( int snd, int fdc_track )
{
	if( !drivesound_is_allowed() )
	{
		return -1;
	}

	if( snd < 0 || snd >= DRIVESOUND_MAX )
	{
		return -2;
	}

	if( snd == DRIVESOUND_SEEK_BACK )
	{
		float num_pcm_samples = (float)( g_drivesound_snd[ DRIVESOUND_SEEK_BACK ].num_pcm_samples );
		float back_position = ( (float)fdc_track / 82.0f ) * num_pcm_samples;

		back_position = num_pcm_samples - back_position;

		if( back_position + 1 >= num_pcm_samples )
		{
			back_position = num_pcm_samples - 1.0f;
		}

		g_drivesound_snd[ DRIVESOUND_SEEK_FWD ].playing = 0;

		g_drivesound_snd[ DRIVESOUND_SEEK_BACK ].position = (int)back_position * 2;
		g_drivesound_snd[ DRIVESOUND_SEEK_BACK ].playing = 1;
	}
	else if( snd == DRIVESOUND_SEEK_FWD )
	{
		float num_pcm_samples = (float)( g_drivesound_snd[ DRIVESOUND_SEEK_FWD ].num_pcm_samples );
		float fwd_position = ( (float)fdc_track / 82.0f ) * num_pcm_samples;

		if( fwd_position + 1 >= num_pcm_samples )
		{
			fwd_position = num_pcm_samples - 1.0f;
		}

		g_drivesound_snd[ DRIVESOUND_SEEK_BACK ].playing = 0;

		g_drivesound_snd[ DRIVESOUND_SEEK_FWD ].position = (int)fwd_position * 2;
		g_drivesound_snd[ DRIVESOUND_SEEK_FWD ].playing = 1;
	}
	else
	{
		if( snd == DRIVESOUND_SPIN )
		{
			if( g_drivesound_snd[ DRIVESOUND_SPIN ].playing )
			{
				// if already playing, don't restart
				return 0;
			}
		}

		g_drivesound_snd[ snd ].playing = 1;
		g_drivesound_snd[ snd ].position = 0;
	}

	return 0;
}

int drivesound_play( int snd )
{
	return drivesound_play_from_track( snd, 0 );
}

int drivesound_stop( int snd )
{
	if( !drivesound_is_allowed() )
	{
		return -1;
	}

	if( snd < 0 || snd >= DRIVESOUND_MAX )
	{
		return -2;
	}

	g_drivesound_snd[ snd ].playing = 0;

	return 0;
}

int drivesound_stop_seek( void )
{
	if( !drivesound_is_allowed() )
	{
		return -1;
	}

	g_drivesound_snd[ DRIVESOUND_SEEK_BACK ].playing = 0;
	g_drivesound_snd[ DRIVESOUND_SEEK_FWD ].playing = 0;

	return 0;
}

int drivesound_stop_all( int stop_spin )
{
	if( !drivesound_is_allowed() )
	{
		return -1;
	}

	for( int i = 0 ; i < DRIVESOUND_MAX ; i++ )
	{
		if( i == DRIVESOUND_SPIN && !stop_spin )
		{
			continue;
		}

		drivesound_stop( i );
	}

	return 0;
}

#ifdef DRIVESOUND_DEBUG_PRINT_MSG

int drivesound_msg( const char *text )
{
	core_signal_alert( text );

	return 0;
}

#endif

// ====================================

#ifndef CORE_AUDIO_BUFFER_LEN
#define CORE_AUDIO_BUFFER_LEN   (4*2*96000/50)
#endif

int drivesound_mix_update( int index, int length )
{
	if( drivesound_samplerate != core_audio_samplerate )
	{
		drivesound_uninit();

		int result = drivesound_init();

		if( result )
		{
			// reinit failed, don't try anymore until the sample rate changes again
			drivesound_samplerate = core_audio_samplerate;
		}
	}

	if( !drivesound_is_allowed() )
	{
		return -1;
	}

	for( int i = 0 ; i < DRIVESOUND_MAX ; i++ )
	{
		if( g_drivesound_snd[ i ].playing )
		{
			drivesound_mix_update_snd( index, length, i );
		}
	}

	return 0;
}

int drivesound_mix_update_snd( int index, int length, int which_snd )
{
	if( !drivesound_is_allowed() )
	{
		return -1;
	}

	Sint16 sample1 = 0;
	Sint16 sample2 = 0;
	drivesound_snd_t *snd = NULL;

	int len = length * 2;
	int pos = core_audio_samples_pending - len;
	int max = CORE_AUDIO_BUFFER_LEN - pos;
	if( len > max ) len = max;

	int l2 = len / 2;

	if( which_snd < 0 || which_snd >= DRIVESOUND_MAX )
	{
		return -2;
	}

	snd = &g_drivesound_snd[ which_snd ];

	// get config volume

	float volume = (float)drivesound_volume / 100.0f;
	if( volume < 0.0f ) volume = 0.0f;
	else if( volume > 2.0f ) volume = 2.0f;

	for( int i = 0 ; i < l2 ; ++i )
	{
		// get the samples

		sample1 = *(Sint16 *)( (Uint8 *)( snd->data + snd->position ) );
		sample2 = *(Sint16 *)( (Uint8 *)( snd->data + snd->position + 2 ) );

		// adjust volume

		float adjusted = (float)sample1 * volume;
		sample1 = (Sint16)adjusted;
		adjusted = (float)sample2 * volume;
		sample2 = (Sint16)adjusted;

		// hard limiter

		if( (int)core_audio_buffer[ pos + 0 ] + (int)sample1 > 32767 )
		{
			core_audio_buffer[ pos + 0 ] = 32767;
		}
		else if( (int)core_audio_buffer[ pos + 0 ] + (int)sample1 < -32768 )
		{
			core_audio_buffer[ pos + 0 ] = -32768;
		}
		else
		{
			core_audio_buffer[ pos + 0 ] += sample1;
		}

		if( (int)core_audio_buffer[ pos + 1 ] + (int)sample2 > 32767 )
		{
			core_audio_buffer[ pos + 1 ] = 32767;
		}
		else if( (int)core_audio_buffer[ pos + 1 ] + (int)sample2 < -32768 )
		{
			core_audio_buffer[ pos + 1 ] = -32768;
		}
		else
		{
			core_audio_buffer[ pos + 1 ] += sample2;
		}

		pos += 2;
		snd->position += 2;

		if( snd->position >= (int)( snd->size - sizeof( wav_header_t ) ) )
		{
			snd->position = 0;

			// spin must be stopped explicitly
			if( which_snd != DRIVESOUND_SPIN )
			{
				snd->playing = 0;
				break;
			}
		}
	}

	return 0;
}

int drivesound_init( void )
{
	wav_header_t *wav = NULL;
	drivesound_snd_t *snd = NULL;
	FILE *file = NULL;
	unsigned int bytes_read = 0;
	char path[ 512 ] = "";
	char msg[ 512 ] = "";

	drivesound_loaded = 0;

	for( int i = 0 ; i < DRIVESOUND_MAX ; i++ )
	{
		snd = &g_drivesound_snd[ i ];

		snprintf( path, 512, DRIVESOUND_PATH_PREFIX "/%s_%i.wav",
			snd->name, core_audio_samplerate );

		file = fopen( path, "rb+" );

		if( !file )
		{
			snprintf( msg, 512, "[DriveSound] Failed to open %s!", path );
			core_signal_alert( msg );
			return -1;
		}

		fseek( file, 0, SEEK_END );
		snd->size = ftell( file );
		fseek( file, 0, SEEK_SET );

		if( !snd->size )
		{
			fclose( file );
			snprintf( msg, 512, "[DriveSound] %s is empty!", path );
			core_signal_alert( msg );
			return -2;
		}

		snd->buf = malloc( snd->size );

		if( !snd->buf )
		{
			fclose( file );
			core_signal_alert( "[DriveSound] Memory allocation failed!" );
			return -3;
		}

		bytes_read = fread( snd->buf, 1, snd->size, file );
		fclose( file );

		//

		snd->data = (Uint8 *)( (Uint8 *)snd->buf + sizeof( wav_header_t ) );
		snd->position = 0;
		snd->playing = 0;

		wav = (wav_header_t *)snd->buf;
		snd->num_pcm_samples = wav->data_bytes / ( wav->num_channels * wav->bit_depth / 8 );
	}

	drivesound_samplerate = core_audio_samplerate;
	drivesound_loaded = 1;
	//core_signal_alert( "[DriveSound] OK" );

	return 0;
}

int drivesound_uninit( void )
{
	drivesound_snd_t *snd = NULL;

	for( int i = 0 ; i < DRIVESOUND_MAX ; i++ )
	{
		snd = &g_drivesound_snd[ i ];

		if( snd->buf )
		{
			free( snd->buf );
			snd->buf = NULL;
			snd->position = 0;
			snd->data = NULL;
			snd->playing = 0;
			snd->size = 0;
		}
	}

	drivesound_loaded = 0;

	return 0;
}

#endif

