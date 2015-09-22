//
// Created by noah on 7/25/15.
//

#ifndef MUSIC_PROCESSING_AUDIO_DECODING_H
#define MUSIC_PROCESSING_AUDIO_DECODING_H

#include <stdlib.h>
#include <vlc/vlc.h>
#include "audio_structs.h"

// 44.1khz sample frequency -- also the number of samples in a second.
// coincidentally (not), we want a segment of 1 second for the short fast fourier transform
// to allow realtime sample processing
#define SAMPLING_FREQUENCY  44100
#define NUM_SAMPLES_PER_FFT SAMPLING_FREQUENCY / 4
#define WINDOW_MS_DURATION  5
#define WINDOW_SAMPLE_SIZE  WINDOW_MS_DURATION * SAMPLING_FREQUENCY / 1000

/**
 * AUDIO DECODING FUNCTIONS
 */
extern vlc_context *init_vlc_context(char *, int);

extern int fft_spawn(uint8_t *);

struct pipeline_component;

pipeline_component fft {
        fft_spawn,
        0,
        NULL
};


#endif //MUSIC_PROCESSING_AUDIO_DECODING_H
