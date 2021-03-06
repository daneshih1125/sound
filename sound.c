/*
 *   ALSA command line mixer utility
 *   Copyright (c) 1999-2000 by Jaroslav Kysela <perex@perex.cz>
 *
 *   ALSA LIB pratice
 *   Copyright (c) 2014 by Dane <daneshih1125@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <stdio.h>
#include <ctype.h>
#include "sound.h"

static char card[64] = "default";
static int smixer_level = 0;
static struct snd_mixer_selem_regopt smixer_options;

static int parse_simple_id(const char *str, snd_mixer_selem_id_t *sid)
{
	int c, size;
	char buf[128];
	char *ptr = buf;

	while (*str == ' ' || *str == '\t')
		str++;
	if (!(*str))
		return -EINVAL;
	size = 1;	/* for '\0' */
	if (*str != '"' && *str != '\'') {
		while (*str && *str != ',') {
			if (size < (int)sizeof(buf)) {
				*ptr++ = *str;
				size++;
			}
			str++;
		}
	} else {
		c = *str++;
		while (*str && *str != c) {
			if (size < (int)sizeof(buf)) {
				*ptr++ = *str;
				size++;
			}
			str++;
		}
		if (*str == c)
			str++;
	}
	if (*str == '\0') {
		snd_mixer_selem_id_set_index(sid, 0);
		*ptr = 0;
		goto _set;
	}
	if (*str != ',')
		return -EINVAL;
	*ptr = 0;	/* terminate the string */
	str++;
	if (!isdigit(*str))
		return -EINVAL;
	snd_mixer_selem_id_set_index(sid, atoi(str));
       _set:
	snd_mixer_selem_id_set_name(sid, buf);
	return 0;
}

static snd_mixer_elem_t *sound_get_elem(const char *name, snd_mixer_t **mixer_handle)
{
	int ret;
	snd_mixer_t *handle; 

	snd_mixer_selem_id_t *sid;

	setenv("PULSE_INTERNAL", "0", 1);
	snd_mixer_selem_id_alloca(&sid);

	if (parse_simple_id(name, sid)) {
		fprintf(stderr, "Wrong scontrol identifier: %s\n", name);
		return NULL;
	}

	if ((ret = snd_mixer_open(&handle, 0)) < 0) {
		sdbg("Mixer %s open error: %s\n", card, snd_strerror(ret));
		return NULL;
	}
	if (smixer_level == 0 && (ret = snd_mixer_attach(handle, card)) < 0) {
		sdbg("Mixer attach %s error: %s", card, snd_strerror(ret));
		snd_mixer_close(handle);
		handle = NULL;
		return NULL;
	}
	if ((ret = snd_mixer_selem_register(handle, smixer_level > 0 ? &smixer_options : NULL, NULL)) < 0) {
		sdbg("Mixer register error: %s", snd_strerror(ret));
		snd_mixer_close(handle);
		handle = NULL;
		return NULL;
	}
	ret = snd_mixer_load(handle);
	if (ret < 0) {
		sdbg("Mixer %s load error: %s", card, snd_strerror(ret));
		snd_mixer_close(handle);
		handle = NULL;
		return NULL;
	}

	*mixer_handle = handle;

	return snd_mixer_find_selem(handle, sid);
}

static void elem_set_vol_percent(snd_mixer_elem_t *elem, int vol_percent)
{
	int i;
	long pmin, pmax, value;
	
	//sdbg("%s: set mixer element volume\n", __func__);
	for (i = 0; i <= SND_MIXER_SCHN_LAST; i++) {
		if (snd_mixer_selem_has_playback_channel(elem, i) && 
				!snd_mixer_selem_has_common_volume(elem)) {
			snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
		/* playback volume */
			if (snd_mixer_selem_has_playback_volume(elem)) {
				value = (int)rint((double)pmin + 0.01 * vol_percent * ((double)pmax - (double)pmin));
				snd_mixer_selem_set_playback_volume(elem, i, value);
			}
		} else if (snd_mixer_selem_has_capture_channel(elem, i) && 
				!snd_mixer_selem_has_common_volume(elem)) {
		/* capture volume */
			snd_mixer_selem_get_capture_volume_range(elem, &pmin, &pmax);
			if (snd_mixer_selem_has_capture_volume(elem)) {
				value = (int)rint((double)pmin + 0.01 * vol_percent * ((double)pmax - (double)pmin));
				snd_mixer_selem_set_capture_volume(elem, i, value);
			}
		} else {
		/* Misc. ex: Boost */
			snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
			value = (int)rint((double)pmin + 0.01 * vol_percent * ((double)pmax - (double)pmin));
			snd_mixer_selem_set_playback_volume(elem, i, value);

			snd_mixer_selem_get_capture_volume_range(elem, &pmin, &pmax);
			value = (int)rint((double)pmin + 0.01 * vol_percent * ((double)pmax - (double)pmin));
			snd_mixer_selem_set_capture_volume(elem, i, value);
		}
	}
}

static int elem_get_vol_percent(snd_mixer_elem_t *elem)
{
	int i, percent; 
	long pmin, pmax, volume;

	for (i = 0; i <= SND_MIXER_SCHN_LAST; i++) {
		/* playback volume */
		if (snd_mixer_selem_has_playback_channel(elem, i) && 
				!snd_mixer_selem_has_common_volume(elem)) {
			snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
			if (snd_mixer_selem_has_playback_volume(elem))
				snd_mixer_selem_get_playback_volume(elem, i, &volume);

		}
		/* capture volume */
		else if (snd_mixer_selem_has_capture_channel(elem, i) && 
				!snd_mixer_selem_has_common_volume(elem)) {
			snd_mixer_selem_get_capture_volume_range(elem, &pmin, &pmax);
			if (snd_mixer_selem_has_capture_volume(elem))
				snd_mixer_selem_get_capture_volume(elem, i, &volume);
		} else {
		/* Misc. ex: Boost */
			snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
			snd_mixer_selem_get_playback_volume(elem, i, &volume);
		}
	}

	percent = (int) ((double)(volume - pmin) / (double)(pmax - pmin) * 100);

	return percent;
}

static int elem_get_status(snd_mixer_elem_t *elem)
{
	int i, psw = -1;

	for (i = 0; i <= SND_MIXER_SCHN_LAST; i++) {
		if (snd_mixer_selem_has_playback_channel(elem, i) && 
				snd_mixer_selem_has_playback_switch(elem))
			snd_mixer_selem_get_playback_switch(elem, i, &psw);
		else if (snd_mixer_selem_has_capture_channel(elem, i) && 
				snd_mixer_selem_has_capture_switch(elem))
			snd_mixer_selem_get_capture_switch(elem, i, &psw);
	}

	return psw;
}

MUTEST sound_mixer_mute_status(char *mixer_name)
{
	int status;

	snd_mixer_t *handle = NULL;
	snd_mixer_elem_t *elem = NULL;

	elem = sound_get_elem(mixer_name, &handle);
	if (!elem) {
		sdbg("could not found elem\n");
		return -1;
	}
	status = elem_get_status(elem);
	snd_mixer_close(handle);

	return status;
}

/* mute: 0 -> mute,  1 -> unmute */
void sound_mixer_mute(const char *mixer, MUTEST mute)
{
	int i;
	snd_mixer_t *handle = NULL;
	snd_mixer_elem_t *elem = NULL;

	elem = sound_get_elem(mixer, &handle);
	if (!elem)
		return;

	for (i = 0; i <= SND_MIXER_SCHN_LAST; i++) {
		if (snd_mixer_selem_has_playback_channel(elem, i) &&
				snd_mixer_selem_has_playback_switch(elem))
			snd_mixer_selem_set_playback_switch(elem, i, mute);
		else if (snd_mixer_selem_has_capture_channel(elem, i) &&
				snd_mixer_selem_has_capture_switch(elem))
			snd_mixer_selem_set_capture_switch(elem, i, mute);
	}
	snd_mixer_close(handle);
}

int sound_mixer_get_volume(const char *mixer_name)
{
	int percent = -1;
	snd_mixer_t *handle = NULL;
	snd_mixer_elem_t *elem = NULL;

	elem = sound_get_elem(mixer_name, &handle);

	if (!elem) {
		sdbg("%s Could not find element\n", mixer_name);
		return -1;
	}

	if ((percent = elem_get_vol_percent(elem)) < 0) {
		sdbg("%s Could not get volume\n", mixer_name);
		return -1;
	}

	/* free mixer */
	snd_mixer_close(handle);

	return percent;
}

int sound_mixer_set_volume(char *mixer_name, int vol)
{
	snd_mixer_t *handle = NULL;
	snd_mixer_elem_t *elem = NULL;

	if (vol < 0 || vol > 100)
		return -1;

	elem = sound_get_elem(mixer_name, &handle);
	if (!elem)
		return -1;
	elem_set_vol_percent(elem, vol);
	snd_mixer_close(handle);

	return 0;
}


#ifdef SOUND_TEST

int main(int argc, char **argv)
{

	int st;
	
	if (argc < 2)
		return -1;

	if (!strcmp("get", argv[1])) {
		printf("get %s vol %d\n", argv[2], sound_mixer_get_volume(argv[2]));
	} else if (!strcmp("set", argv[1])) {
		printf("previous %s vol %d\n", argv[2], sound_mixer_get_volume(argv[2]));
		sound_mixer_set_volume(argv[2], atoi(argv[3]));
		printf("current %s vol %d\n", argv[2], sound_mixer_get_volume(argv[2]));
	} else if (!strcmp("mute", argv[1])) {
		sound_mixer_mute(argv[2], MUTE);
		printf("mute %s\n", argv[2]);
	} else if (!strcmp("unmute", argv[1])) {
		sound_mixer_mute(argv[2], UNMUTE);
		printf("unmute %s\n", argv[2]);
	} else if (!strcmp("mutest", argv[1])) {
		st = sound_mixer_mute_status(argv[2]);
		printf("%s is %s\n", argv[1],
				st == MUTE ? "mute" : "unmute");
	}

	return 0;
}

#endif /* SOUND_TEST */
