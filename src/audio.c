#include <string.h>
#include <stdio.h>
#include "audio.h"

static void audio_callback(void *userdata, Uint8 *stream, int len) {
    audio_state_t *a = (audio_state_t *)userdata;
    int16_t *out = (int16_t *)stream;
    int samples = len / 2;

    size_t avail = a->write_pos - a->read_pos;

    for (int i = 0; i < samples; i++) {
        if (i < (int)avail && !a->muted) {
            float s = a->ring[a->read_pos % AUDIO_RING_SIZE] * a->volume;
            out[i] = (int16_t)s;
            a->read_pos++;
        } else {
            out[i] = 0;
        }
    }
}

int audio_init(audio_state_t *a, int sample_rate) {
    memset(a, 0, sizeof(*a));
    a->sample_rate = sample_rate;
    a->volume = 0.5f;

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = sample_rate;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    want.callback = audio_callback;
    want.userdata = a;

    a->dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (a->dev == 0) {
        fprintf(stderr, "Audio open failed: %s\n", SDL_GetError());
        return -1;
    }

    SDL_PauseAudioDevice(a->dev, 0);
    return 0;
}

void audio_free(audio_state_t *a) {
    if (a->dev) {
        SDL_CloseAudioDevice(a->dev);
        a->dev = 0;
    }
}

void audio_push(audio_state_t *a, const int16_t *samples, int count) {
    for (int i = 0; i < count; i++) {
        a->ring[a->write_pos % AUDIO_RING_SIZE] = samples[i];
        a->write_pos++;
    }
}
