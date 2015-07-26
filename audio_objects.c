/*
 * audio_objects.c
 *
 *  Created on: Jun 16, 2015
 *      Author: Noah
 */

#include <stdlib.h>
#include <vlc/vlc.h>

typedef struct vlc_context {
    libvlc_instance_t *mVlcInstance;
    libvlc_media_player_t *mMp;
    libvlc_media_t *mMedia;

    libvlc_meta_t mMeta;

    int mPlaying;
    int16_t *mBuffer;

    int mChunkSize;

    char *mAudioData;
    unsigned int mAudioDataSize;
    unsigned int mBufferSize;
    unsigned int mFrequency;
    unsigned int mFramesOverlap;

    int mChannels;

    pthread_mutex_t *mLock;
} vlc_context;

typedef struct meta_map_entry {
    libvlc_meta_t e_meta;
    char *meta_str;
} meta_map_entry;


typedef enum {
    SUB_BASS,
    BASS,
    LOW_MIDS,
    HIGH_MIDS,
    PRESENCE,
    BRILLIANCE,
    OUT_OF_RANGE
} frequency_band;

// major frequency bands to track
typedef struct frequency_band_distribution {
    float sub_bass,
            bass,
            low_mids,
            high_mids,
            presence,
            brilliance;
} frequency_band_distribution;

// where 1 = halftone
// 		 0 = semitone
// an input code could be bit-shifted until it matches the
// base diatonic major scale -- the number of shifts required
// will indicate the scale (minor, mixolydian, dorian, etc)
//int diatonic_scale = 0b1101110;

struct audio_data {
    meta_map_entry meta_information[libvlc_meta_Actors - libvlc_meta_Title];
    int bpm;
    frequency_band_distribution freq_dists;


};





