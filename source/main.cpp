/*
 ============================================================================
 Name        : MP3.c
 Author      : Noah Golmant
 Version     : 0.1
 Copyright   :
 Description : Decode an arbitrary audio file to PCM through libVLC
 ============================================================================
 */

#include <stdio.h>
#include "audio_decoding.h"

#define SUB_BASS_MAX   60
#define BASS_MAX       250
#define LOW_MIDS_MAX   2000
#define HIGH_MIDS_MAX  4000
#define PRESENCE_MAX   6000
#define BRILLIANCE_MAX 16000

// matches given frequency to correct frequency band
// see http://bobbyowsinski.blogspot.com/2012/06/description-of-audio-frequency-bands.html
frequency_band getQuantizedFrequencyBand(int frequency) {
    if (frequency <= SUB_BASS_MAX)
        return SUB_BASS;
    else if (frequency <= BASS_MAX)
        return BASS;
    else if (frequency <= LOW_MIDS_MAX)
        return LOW_MIDS;
    else if (frequency <= HIGH_MIDS_MAX)
        return HIGH_MIDS;
    else if (frequency <= PRESENCE_MAX)
        return PRESENCE;
    else if (frequency <= BRILLIANCE_MAX)
        return BRILLIANCE;
    else
        return OUT_OF_RANGE;
}


int main(int argc, char **argv) {
    if (argc < 2)
        printf("Usage: %s filename\n", argv[0]);
    else {
        vlc_context *ctx = init_vlc_context(argv[1], 32);

        getchar();
    }

    return 0;
}

