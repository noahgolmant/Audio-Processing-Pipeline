//
// Created by noah on 7/25/15.
//

#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include "audio_decoding.h"

/* FUNCTION PROTOTYPES */
static void prepareRender(void *p_audio_data, uint8_t **pp_pcm_buffer,
                          unsigned int size);

static void handleStream(void *p_audio_data, uint8_t *p_pcm_buffer,
                         unsigned int channels, unsigned int rate, unsigned int nb_samples,
                         unsigned int bits_per_sample, unsigned int size, int64_t pts);

static void flushBuffer(vlc_context *ctx);

/**
 * Initializes the vlc context.
 * It specifies the data stream/render callbacks as strings in its initialization arguments.
 * It then specifies all of the other important raw data for the audio file.
 */
vlc_context *init_vlc_context(char *uri, int chunkSize) {
    // initialize memory space for new context
    vlc_context *ctx = (vlc_context *) malloc(sizeof(vlc_context));
    ctx->mPlaying = 0;

    char smemOptions[256];

    // print the VLC smem options to the smemOptions buffer, communicating function pointers for
    // handling stream rendering preparation, the stream itself, and info about the audio data
    sprintf(smemOptions,
            "#transcode{acodec=s16l}:duplicate{dst=display,dst=smem"
                    "{audio-postrender-callback=%lld,audio-prerender-callback=%lld,audio-data=%lld}}",
            (long long int) (intptr_t) (void *) &handleStream,
            (long long int) (intptr_t) (void *) &prepareRender,
            (long long int) (intptr_t) (void *) ctx);

    // no stream output device
    const char stringSout[] = "--sout", stringNoSoutSmemTimeSync[] =
            "--no-sout-smem-time-sync";
    const char *const vlcArgs[] = {stringSout, smemOptions,
                                   stringNoSoutSmemTimeSync};

    // Initialize the vlc instance with the proper arguments
    ctx->mVlcInstance = libvlc_new(2, vlcArgs); // deleted in the destructor

    // Create a new media player for the actual music
    ctx->mMp = libvlc_media_player_new(ctx->mVlcInstance); // deleted in the destructor
    libvlc_audio_set_volume(ctx->mMp, 0);

    // construct media object from file reference
    ctx->mMedia = libvlc_media_new_path(ctx->mVlcInstance, uri);
    // we are "playing" it to render the data and get an audio buffer
    ctx->mPlaying = 1;
    ctx->mBufferSize = 0;
    // only look at one channel of the audio
    // TODO: process stereo audio files with the average amplitude per sample
    ctx->mChannels = 1;
    ctx->mChunkSize = chunkSize; // size with which we iterate over stream frame data
    // the audio buffer size, holding 16-bit integers representing the audio amplitude.
    ctx->mBuffer = (int16_t *) malloc(sizeof(int16_t) * 2 * ctx->mChunkSize);
    ctx->mFramesOverlap = 0.5 * ctx->mChunkSize;
    // set the thread mutex lock, so that we can lock the ctx object for our usage in the render/stream handling
    // and then unlock it once we are done -- while it is locked, vlc does not have concurrent access to it
    ctx->mLock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(ctx->mLock, NULL);
    // finally, parse some of the media
    libvlc_media_parse(ctx->mMedia);

    int meta; // current libvlc_meta_t enum value in our iterator
    char *retMeta;  // extracted metadata at libvlc_meta_t meta

    // create a map between the metadata enum and its actual char* data
    meta_map_entry meta_map[libvlc_meta_Actors - libvlc_meta_Title];

    int i = 0;
    for (meta = libvlc_meta_Title; meta <= libvlc_meta_Actors; meta++) {
        retMeta = libvlc_media_get_meta(ctx->mMedia, (libvlc_meta_t) meta);
        // create the map entry
        meta_map_entry entry = {(libvlc_meta_t) meta, retMeta};
        // store it in the array
        meta_map[i++] = entry;

    }

    // for testing purposes -- we only want to print non-null data
    for (i = 0; i < (libvlc_meta_Actors - libvlc_meta_Title); i++) {
        printf("%s\n", meta_map[i].meta_str ? meta_map[i].meta_str : strdup("nil"));
    }

    // start the media player, which will callback our handle/prepare functions
    libvlc_media_player_set_media(ctx->mMp, ctx->mMedia);
    libvlc_media_player_play(ctx->mMp);

    return ctx;
}

// Get ready to render the stream to the buffer
static void prepareRender(void *p_audio_data, uint8_t **pp_pcm_buffer,
                          unsigned int size) {
    vlc_context *sp = ((vlc_context *) p_audio_data);
    // Lock the mutex so we can access the data
    pthread_mutex_lock(sp->mLock);
    // Create mAudioData buffer of the correct size for the stream
    if (sp->mAudioDataSize < size) {
        if (sp->mAudioData)
            free(sp->mAudioData);
        sp->mAudioData = (char *) malloc(sizeof(char) * size); // Deleted in the destructor
    }
    // Ensure that the PCM (pulse code modulation) buffer points to the mAudioData
    // buffer we just initialized as a component of our vlc_context
    *pp_pcm_buffer = (uint8_t *) (sp->mAudioData);
}

// Handles the data once prepared for rendering
static void handleStream(void *p_audio_data, uint8_t *p_pcm_buffer,
                         unsigned int channels, unsigned int rate, unsigned int nb_samples,
                         unsigned int bits_per_sample, unsigned int size, int64_t pts) {
    unsigned int copied = 0;
    vlc_context *sp = ((vlc_context *) p_audio_data);

    // Update the frequency if needed
    if (rate != sp->mFrequency)
        sp->mFrequency = rate;
    sp->mChannels = channels;

    printf("PCM BUFFER: \n");
    int i;
    for (i = 0; i < sizeof(p_pcm_buffer) / sizeof(uint8_t); i++) {
        printf("	%d\n", p_pcm_buffer[i]);
    }

    // unlock the mutex before we finish
    pthread_mutex_unlock(sp->mLock);
}

static void flushBuffer(vlc_context *ctx) {
    memcpy(ctx->mBuffer,
           ctx->mBuffer
           + ctx->mChannels * (ctx->mChunkSize - ctx->mFramesOverlap),
           ctx->mChannels * ctx->mFramesOverlap * sizeof(int16_t));
    ctx->mBufferSize = ctx->mFramesOverlap;
}