//
// Created by noah on 7/25/15.
//
#include <fftw3.h>
#include <pthread.h>
#include <string.h>
#include <tgmath.h>
#include "audio_decoding.h"

#define HAMMING_WINDOW_ALPHA 0.54
#define HAMMING_WINDOW_BETA  0.46

/* EXPORTED FUNCTIONS */
int fft_spawn(uint8_t *);

/* STATIC FUNCTIONS */
static void *fft_segment(void *audio_segment_buffer);


/* Spawns a thread to process the audio segment in an FFT */
int fft_spawn(uint8_t *audio_segment_buffer) {
    pthread_t fft_thread;
    return pthread_create(&fft_thread, NULL, fft_segment, audio_segment_buffer);
}

static inline double hamming_window(int current_sample, int num_samples) {
    return HAMMING_WINDOW_ALPHA - HAMMING_WINDOW_BETA * cos((2 * M_PI * current_sample) / (num_samples - 1));
}

/* Processes the audio segment into discrete frequency bins with an FFT */
static void *fft_segment(void *audio_segment_buffer_orig) {
    // create a new copy of the buffer so we don't access the space being used by VLC
    uint8_t *audio_segment_buffer = (uint8_t *) malloc(sizeof(audio_segment_buffer_orig));
    memcpy(audio_segment_buffer, audio_segment_buffer_orig, sizeof(audio_segment_buffer_orig));

    // create and input into an FFT
    // an fftw_complex is 2D matrix -- one dimension represents the real components, the other complex.
    fftw_complex in[NUM_SAMPLES_PER_FFT];
    fftw_complex out[NUM_SAMPLES_PER_FFT];

    int win_start_index = 0;
    int win_n = 0;

    while (win_start_index < NUM_SAMPLES_PER_FFT) {
        while (win_n < WINDOW_SAMPLE_SIZE) {
            in[win_start_index + win_n][0] = audio_segment_buffer[win_n] * hamming_window(win_n, WINDOW_SAMPLE_SIZE);
            win_n++;
        }

        win_n = 0;
        win_start_index += WINDOW_SAMPLE_SIZE / 2;
    }

    // establish complex component of the input matrix
    for (int i = 0; i < NUM_SAMPLES_PER_FFT; i++) {
        in[i][1] = 0;
    }

    fftw_plan plan;
    plan = fftw_plan_dft_1d(NUM_SAMPLES_PER_FFT, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    fftw_execute(plan);

    double frequency_magnitude_dB[NUM_SAMPLES_PER_FFT];

    // test output of frequency data
    for (int i = 0; i < NUM_SAMPLES_PER_FFT; i++) {
        // calculate the magnitude of the sample, then convert it to a logarithmic scale
        // to represent as a decibel
        frequency_magnitude_dB[i] = 20 * log10(sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]));
        printf("FREQUENCY: %3d\tdB: %9.5f\n", i * (SAMPLING_FREQUENCY / NUM_SAMPLES_PER_FFT),
               frequency_magnitude_dB[i]);
    }

}