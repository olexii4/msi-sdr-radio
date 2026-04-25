#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <SDL.h>

#define AUDIO_RING_SIZE (48000 * 4)

typedef struct {
    SDL_AudioDeviceID dev;
    int16_t ring[AUDIO_RING_SIZE];
    volatile size_t write_pos;
    volatile size_t read_pos;
    int sample_rate;
    float volume;
    int muted;
} audio_state_t;

int audio_init(audio_state_t *a, int sample_rate);
void audio_free(audio_state_t *a);
void audio_push(audio_state_t *a, const int16_t *samples, int count);

#endif
