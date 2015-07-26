//
// Created by noah on 7/25/15.
//

#ifndef MUSIC_PROCESSING_AUDIO_DECODING_H
#define MUSIC_PROCESSING_AUDIO_DECODING_H

#include <stdlib.h>
#include <vlc/vlc.h>
#include "audio_structs.h"


/**
 * AUDIO DECODING FUNCTIONS
 */
extern vlc_context *init_vlc_context(char *, int);

#endif //MUSIC_PROCESSING_AUDIO_DECODING_H
