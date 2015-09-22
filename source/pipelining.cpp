//
// Created by noah on 9/3/15.
//
#include "audio_decoding.h"

typedef struct {
    int pipeline_func(uint8_t *);
    int blocking = 0;

    pipeline_component reference;
} pipeline_component;

