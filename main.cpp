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
#include <stdlib.h>
#include <vlc/vlc.h>
#include <pthread.h>
#include <string.h>
#include "audio_objects.c"

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

void prepareRender(void *p_audio_data, uint8_t **pp_pcm_buffer,
                   unsigned int size);

void handleStream(void *p_audio_data, uint8_t *p_pcm_buffer,
                  unsigned int channels, unsigned int rate, unsigned int nb_samples,
                  unsigned int bits_per_sample, unsigned int size, int64_t pts);

void flushBuffer(vlc_context *ctx);

void useBuffer(vlc_context *ctx);

vlc_context *
caml_init_context(char *uri, int chunkSize) {
    // initialize memory space for new context
    vlc_context *ctx = (vlc_context *) malloc(sizeof(vlc_context));
    ctx->mPlaying = 0;

    char smemOptions[256];

    char *url = uri;

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

    ctx->mVlcInstance = libvlc_new(2, vlcArgs); // deleted in the destructor

    ctx->mMp = libvlc_media_player_new(ctx->mVlcInstance); // deleted in the destructor
    libvlc_audio_set_volume(ctx->mMp, 0);

    // construct media object from file reference
    ctx->mMedia = libvlc_media_new_path(ctx->mVlcInstance, url);
    ctx->mPlaying = 1;
    ctx->mBufferSize = 0;
    ctx->mChannels = 1;
    ctx->mChunkSize = chunkSize; // size with which we iterate over stream frame data
    ctx->mBuffer = (int16_t *) malloc(sizeof(int16_t) * 2 * ctx->mChunkSize);
    ctx->mFramesOverlap = 0.5 * ctx->mChunkSize;
    ctx->mLock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t)); // create and store the mutex lock
    pthread_mutex_init(ctx->mLock, NULL);

    libvlc_media_parse(ctx->mMedia);

    char *retMeta;
    //libvlc_meta_t meta;
    int meta;
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

    for (i = 0; i < (libvlc_meta_Actors - libvlc_meta_Title); i++) {
        printf("%s\n", meta_map[i].meta_str ? meta_map[i].meta_str : strdup("nil"));
    }

    // start the media player, which will callback our handle/prepare functions
    libvlc_media_player_set_media(ctx->mMp, ctx->mMedia);
    libvlc_media_player_play(ctx->mMp);

    return ctx;
}

int main(int argc, char **argv) {
    if (argc < 2)
        printf("Usage: %s filename\n", argv[0]);
    else {
        vlc_context *ctx;

        ctx = caml_init_context(argv[1], 32);

        getchar();
    }

    return 0;
}

// what we call once we have the data prepared for our own processing
void useBuffer(vlc_context *ctx) {
    int i;
    for (i = 0; i < ctx->mChunkSize; i += 2);
    //printf("%d\n", ctx->mBuffer[i]);

}

// Get ready to render the stream to the buffer
void prepareRender(void *p_audio_data, uint8_t **pp_pcm_buffer,
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

// helper function
inline int min(int a, int b) {
    return (a < b ? a : b);
}

// Handles the data once prepared for rendering
void handleStream(void *p_audio_data, uint8_t *p_pcm_buffer,
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



    // The data is sent to us as bytes, but encoded on 2 bytes
    // TODO: dynamicly check that this is the case and that we're not mishandling the data
    /*int16_t* temp = (int16_t*) p_pcm_buffer;
    size /= 2;

    // We implemented a mechanism that takes the data sent by libVLC and cut it into chunks
    // of the same size (a power of two) so that the algorithms can handle it in the right way
    while (copied < size) {
        // copy over a suitable amount of info from the chunk to our buffer until we're done
        unsigned int to_copy = min(
                channels * (sp->mChunkSize - sp->mBufferSize), size - copied);
        // store the data in the next spot, i.e. our current buffer position + the new buffer size
        memcpy(sp->mBuffer + channels * sp->mBufferSize, temp + copied,
                to_copy * sizeof(int16_t));
        copied += to_copy;
        sp->mBufferSize += to_copy / channels;

        // once we have copied enough that the buffer completely encompasses the chunk, send it to the user
        if (sp->mBufferSize >= sp->mChunkSize) {
            // The buffer is sent to the "user"
            useBuffer(sp);

            // Emptying buffer
            flushBuffer(sp);
        }
    }*/
    // unlock the mutex before we finish
    pthread_mutex_unlock(sp->mLock);
}

void flushBuffer(vlc_context *ctx) {
    memcpy(ctx->mBuffer,
           ctx->mBuffer
           + ctx->mChannels * (ctx->mChunkSize - ctx->mFramesOverlap),
           ctx->mChannels * ctx->mFramesOverlap * sizeof(int16_t));
    ctx->mBufferSize = ctx->mFramesOverlap;
}