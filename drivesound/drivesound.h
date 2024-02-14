
/*
*********************************
* NTxC's DriveSound for hatariB *
*********************************
*/

#ifdef __DRIVESOUND__

enum
{
	DRIVESOUND_STARTUP = 0,
	DRIVESOUND_CLICK,
	DRIVESOUND_SEEK_BACK,
	DRIVESOUND_SEEK_FWD,
	DRIVESOUND_SPIN,

	DRIVESOUND_MAX
};

//#define DRIVESOUND_ENABLE_SDL
#define DRIVESOUND_DEBUG_PRINT
//DRIVESOUND_DEBUG_PRINT_MSG

#ifdef DRIVESOUND_DEBUG_PRINT
#include <stdarg.h>
extern int Q_vsnprintf( char *str, int size, const char *format, va_list ap );
char *__cdecl va( const char *format, ... );
#else
#define va(x) ;
#endif

extern bool drivesound_is_permitted( void );

extern int drivesound_init( void );
extern int drivesound_uninit( void );

extern int drivesound_play( int snd );
extern int drivesound_play_from_track( int snd, int fdc_track );
extern int drivesound_stop( int snd );
extern int drivesound_stop_seek( void );
extern int drivesound_stop_all( int stop_spin );

#ifdef DRIVESOUND_DEBUG_PRINT_MSG
extern int drivesound_msg( const char *text );
#else
#define drivesound_msg(x) ;
#endif

extern int drivesound_mix_update( int index, int length );
extern int drivesound_mix_update_snd( int index, int length, int which_snd );

#endif

