#ifndef UI_H
#define UI_H

#include <SDL.h>
#include "device.h"
#include "dsp.h"
#include "audio.h"

typedef struct {
    SDL_Window *window;
    SDL_GLContext gl;
    int width;
    int height;
    int running;
    sdr_device_t *device;
    dsp_state_t *dsp;
    audio_state_t *audio;
    int demod_enabled;
    int fm_decimation;
    int active_band;
} ui_state_t;

int ui_init(ui_state_t *ui, sdr_device_t *dev, dsp_state_t *dsp, audio_state_t *audio);
void ui_free(ui_state_t *ui);
void ui_render(ui_state_t *ui);
int ui_handle_events(ui_state_t *ui);
int ui_device_selector(device_list_t *list);

#endif
