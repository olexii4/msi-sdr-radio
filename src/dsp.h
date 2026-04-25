#ifndef DSP_H
#define DSP_H

#include <stdint.h>
#include <fftw3.h>

#define FFT_SIZE 2048
#define SPECTRUM_SIZE FFT_SIZE

#define AUDIO_RATE 48000

typedef struct {
    fftwf_plan plan;
    fftwf_complex *fft_in;
    fftwf_complex *fft_out;
    float spectrum[SPECTRUM_SIZE];
    float spectrum_avg[SPECTRUM_SIZE];
    float avg_alpha;
    float waterfall[512][SPECTRUM_SIZE];
    int waterfall_row;
    float prev_i;
    float prev_q;
    float deemph_val;
    int16_t audio_buf[48000];
    int audio_len;
} dsp_state_t;

void dsp_init(dsp_state_t *dsp);
void dsp_free(dsp_state_t *dsp);
void dsp_process_iq(dsp_state_t *dsp, const int16_t *iq, int num_iq_pairs);
void dsp_fm_demod(dsp_state_t *dsp, const int16_t *iq, int num_iq_pairs,
                  int16_t *audio_out, int *audio_len, int decimation);

#endif
