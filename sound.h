#ifndef _SOUNDCTL_H_
#define _SOUNDCTL_H_

#include <alsa/asoundlib.h>
#include <math.h>
#ifdef SOUNDCTL_DEBUG

#define sdbg(format, arg... ) fprintf(stderr, format, ##arg )
#else
#define sdbg(format, arg... ) \
({ \
	if(0) \
		fprintf(stderr, format, ##arg ); \
})

#endif

int soundctl_mic_get_volume();
void soundctl_mic_set_volume(int vol_percent);

int soundctl_master_get_volume();
void soundctl_master_set_volume(int vol_percent);

void soundctl_mic_mute();
void soundctl_mic_unmute();

void soundctl_master_mute();
void soundctl_master_unmute();

#endif /* _SOUNDCTL_H_ */
