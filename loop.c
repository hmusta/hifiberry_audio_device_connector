/*
 * Copyright (c) 2012 Daniel Mack
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

/*
 * See README
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <alsa/asoundlib.h>
#include <math.h>

#define BUFSIZE (1024)
#define POLLTIME (1000)
#define DEFAULTSR 44100

snd_pcm_t *playback_handle, *capture_handle;
int buf[BUFSIZE * 2];

static unsigned int format = SND_PCM_FORMAT_S16_LE;

static int open_stream(snd_pcm_t **handle, const char *name, int dir, unsigned int rate)
{
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;
	const char *dirname = (dir == SND_PCM_STREAM_PLAYBACK) ? "PLAYBACK" : "CAPTURE";
	int err;

	if ((err = snd_pcm_open(handle, name, dir, 0)) < 0) {
		fprintf(stderr, "%s (%s): cannot open audio device (%s)\n", 
			name, dirname, snd_strerror(err));
		return err;
	}
	   
	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		fprintf(stderr, "%s (%s): cannot allocate hardware parameter structure(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}
			 
	if ((err = snd_pcm_hw_params_any(*handle, hw_params)) < 0) {
		fprintf(stderr, "%s (%s): cannot initialize hardware parameter structure(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params_set_access(*handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf(stderr, "%s (%s): cannot set access type(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params_set_format(*handle, hw_params, format)) < 0) {
		fprintf(stderr, "%s (%s): cannot set sample format(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params_set_rate_near(*handle, hw_params, &rate, NULL)) < 0) {
		fprintf(stderr, "%s (%s): cannot set sample rate(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params_set_channels(*handle, hw_params, 2)) < 0) {
		fprintf(stderr, "%s (%s): cannot set channel count(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params(*handle, hw_params)) < 0) {
		fprintf(stderr, "%s (%s): cannot set parameters(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}

	snd_pcm_hw_params_free(hw_params);

	if ((err = snd_pcm_sw_params_malloc(&sw_params)) < 0) {
		fprintf(stderr, "%s (%s): cannot allocate software parameters structure(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}
	if ((err = snd_pcm_sw_params_current(*handle, sw_params)) < 0) {
		fprintf(stderr, "%s (%s): cannot initialize software parameters structure(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}
	if ((err = snd_pcm_sw_params_set_avail_min(*handle, sw_params, BUFSIZE)) < 0) {
		fprintf(stderr, "%s (%s): cannot set minimum available count(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}
	if ((err = snd_pcm_sw_params_set_start_threshold(*handle, sw_params, 0U)) < 0) {
		fprintf(stderr, "%s (%s): cannot set start mode(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}
	if ((err = snd_pcm_sw_params(*handle, sw_params)) < 0) {
		fprintf(stderr, "%s (%s): cannot set software parameters(%s)\n",
			name, dirname, snd_strerror(err));
		return err;
	}

	return 0;
}

static int start_pcm(unsigned int rate, const char *pdevice, const char *cdevice)
{
    int err;
    if ((err = open_stream(&playback_handle, pdevice, SND_PCM_STREAM_PLAYBACK, rate)) < 0)
		return err;

	if ((err = open_stream(&capture_handle, cdevice, SND_PCM_STREAM_CAPTURE, rate)) < 0)
		return err;

	if ((err = snd_pcm_prepare(playback_handle)) < 0) {
		fprintf(stderr, "cannot prepare audio interface for use(%s)\n",
			 snd_strerror(err));
		return err;
	}
	
	if ((err = snd_pcm_start(capture_handle)) < 0) {
		fprintf(stderr, "cannot prepare audio interface for use(%s)\n",
			 snd_strerror(err));
		return err;
	}

	memset(buf, 0, sizeof(buf));
    return 0;
}

static int read_pcm(void) 
{
    int avail;
    int err;

	if ((err = snd_pcm_wait(playback_handle, POLLTIME)) < 0) {
		fprintf(stderr, "poll failed(%s)\n", strerror(errno));
		return 1;
	}	           

	avail = snd_pcm_avail_update(capture_handle);
	if (avail > 0) {
		if (avail > BUFSIZE)
			avail = BUFSIZE;

		snd_pcm_readi(capture_handle, buf, avail);
	}
    return 0;
}

static int write_pcm(void)
{
    int avail;

	avail = snd_pcm_avail_update(playback_handle);
	if (avail > 0) {
		if (avail > BUFSIZE)
			avail = BUFSIZE;

		snd_pcm_writei(playback_handle, buf, avail);
	}
    return 0;
}

char *line;
char a[64],b[64];
size_t len = 256;

static int check_rate(unsigned int *rate, const char *pdevice, const char *cdevice)
{
    int err;
    FILE *fp = fopen("/proc/asound/sndrpihifiberry/registers","r");
    if (fp == NULL) {
        fprintf(stderr, "failed to read registers(%s)\n", strerror(errno));
        return 1;
    }
    float newrate_k;
    unsigned int newrate=*rate;
    line = NULL;
    while (getline(&line, &len, fp) != -1) {
        // ind freq X Khz
        if (strncmp(line, " ind freq", 9) == 0) {
            sscanf(line, " %s %s %f", a, b, &newrate_k);
            newrate = (unsigned int)round(newrate_k*1000);
            break;
        }
    }
    fclose(fp);
    if (line) {
        free(line);
    }
    if (newrate != *rate) {
        fprintf(stderr, "Switching to sample rate %d\n", newrate);
	    snd_pcm_close(playback_handle);
    	snd_pcm_close(capture_handle);
        *rate = newrate;
        if ((err = start_pcm(*rate, pdevice, cdevice)) < 0)
            return err;
    }
    return 0;
}
  
int main(int argc, char *argv[])
{
	int err;

    unsigned int rate = DEFAULTSR;
    if (argc < 3) {
        fprintf(stderr, "Please specify an input and output device\n");
        return 1;
    }
    char *pdevice = argv[1];
    char *cdevice = argv[2];

    if ((err = start_pcm(rate, pdevice, cdevice)) < 0)
        return err;

    if ((err = read_pcm()) < 0)
        return err;

    while ((err = check_rate(&rate, pdevice, cdevice)) == 0) {
        if ((err = read_pcm()) < 0)
            return err;
        write_pcm();
    }
	snd_pcm_close(playback_handle);
	snd_pcm_close(capture_handle);
    return err;
}
