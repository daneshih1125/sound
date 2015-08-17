#ifndef _SOUND_H_
#define _SOUNE_H_

#include <alsa/asoundlib.h>
#include <math.h>
#ifdef SOUND_DEBUG

#define sdbg(format, arg... ) fprintf(stderr, format, ##arg )
#else
#define sdbg(format, arg... ) \
({ \
	if(0) \
		fprintf(stderr, format, ##arg ); \
})

#endif

typedef enum mute_st{
	MUTE = 0,
	UNMUTE = 1,
} MUTEST;


/*
 * mixer name 
 * playback devices: Master, Mic, Headphone, Mic Boost
 * Capture devices: Capture
 */

int sound_mixer_get_volume(const char *mixer_name);
int sound_mixer_set_volume(char *mixer_name, int vol);

MUTEST sound_mixer_mute_status(char *mixer_name);
void sound_mixer_mute(const char *mixer, MUTEST mute);

#endif /* _SOUND_H_ */
