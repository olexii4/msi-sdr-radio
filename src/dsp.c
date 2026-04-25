#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "dsp.h"

void dsp_init(dsp_state_t *dsp) {
    memset(dsp, 0, sizeof(*dsp));
    dsp->fft_in = fftwf_alloc_complex(FFT_SIZE);
    dsp->fft_out = fftwf_alloc_complex(FFT_SIZE);
    dsp->plan = fftwf_plan_dft_1d(FFT_SIZE, dsp->fft_in, dsp->fft_out,
                                   FFTW_FORWARD, FFTW_MEASURE);
    dsp->avg_alpha = 0.3f;
    dsp->waterfall_row = 0;
    memset(dsp->spectrum_avg, 0, sizeof(dsp->spectrum_avg));
}

void dsp_free(dsp_state_t *dsp) {
    if (dsp->plan) fftwf_destroy_plan(dsp->plan);
    if (dsp->fft_in) fftwf_free(dsp->fft_in);
    if (dsp->fft_out) fftwf_free(dsp->fft_out);
}

static void apply_hann_window(fftwf_complex *data, int n) {
    for (int i = 0; i < n; i++) {
        float w = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (n - 1)));
        data[i][0] *= w;
        data[i][1] *= w;
    }
}

void dsp_process_iq(dsp_state_t *dsp, const int16_t *iq, int num_iq_pairs) {
    if (num_iq_pairs < FFT_SIZE) return;

    int offset = num_iq_pairs - FFT_SIZE;
    for (int i = 0; i < FFT_SIZE; i++) {
        dsp->fft_in[i][0] = iq[(offset + i) * 2] / 32768.0f;
        dsp->fft_in[i][1] = iq[(offset + i) * 2 + 1] / 32768.0f;
    }

    apply_hann_window(dsp->fft_in, FFT_SIZE);
    fftwf_execute(dsp->plan);

    for (int i = 0; i < SPECTRUM_SIZE; i++) {
        int idx = (i + FFT_SIZE / 2) % FFT_SIZE;
        float re = dsp->fft_out[idx][0];
        float im = dsp->fft_out[idx][1];
        float mag = sqrtf(re * re + im * im) / FFT_SIZE;
        float db = 20.0f * log10f(mag + 1e-10f);
        dsp->spectrum[i] = db;
        dsp->spectrum_avg[i] = dsp->spectrum_avg[i] * (1.0f - dsp->avg_alpha)
                             + db * dsp->avg_alpha;
    }

    memcpy(dsp->waterfall[dsp->waterfall_row], dsp->spectrum_avg,
           sizeof(float) * SPECTRUM_SIZE);
    dsp->waterfall_row = (dsp->waterfall_row + 1) % 512;
}

void dsp_fm_demod(dsp_state_t *dsp, const int16_t *iq, int num_iq_pairs,
                  int16_t *audio_out, int *audio_len, int decimation) {
    int out_idx = 0;
    float audio_sum = 0;
    int count = 0;

    float deemph_a = 1.0f / (1.0f + (1.0f / (2.0f * M_PI * 75e-6f * AUDIO_RATE)));

    for (int i = 0; i < num_iq_pairs; i++) {
        float ci = iq[i * 2] / 32768.0f;
        float cq = iq[i * 2 + 1] / 32768.0f;

        float di = ci * dsp->prev_i + cq * dsp->prev_q;
        float dq = cq * dsp->prev_i - ci * dsp->prev_q;
        dsp->prev_i = ci;
        dsp->prev_q = cq;

        float angle = atan2f(dq, di);
        audio_sum += angle;
        count++;

        if (count >= decimation) {
            float sample = (audio_sum / count) / M_PI;
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;

            dsp->deemph_val = dsp->deemph_val * (1.0f - deemph_a) + sample * deemph_a;

            audio_out[out_idx++] = (int16_t)(dsp->deemph_val * 24000);

            audio_sum = 0;
            count = 0;
        }
    }
    *audio_len = out_idx;
}
