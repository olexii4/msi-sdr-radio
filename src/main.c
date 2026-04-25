#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <SDL.h>
#include "device.h"
#include "dsp.h"
#include "audio.h"
#include "ui.h"

static volatile int g_running = 1;

static void sighandler(int sig) {
    (void)sig;
    g_running = 0;
}

int main(int argc, char *argv[]) {
    uint32_t freq = 100000000;
    int gain = 400;

    int argi = 1;
    if (argi < argc && strcmp(argv[argi], "-L") == 0) {
        if (argi + 1 < argc) {
            g_logfile = fopen(argv[argi + 1], "a");
            if (!g_logfile)
                fprintf(stderr, "Warning: cannot open log file %s\n", argv[argi + 1]);
            argi += 2;
        } else {
            fprintf(stderr, "Usage: %s [-L logfile] [freq_mhz] [gain_db]\n", argv[0]);
            return 1;
        }
    }
    if (argi < argc) freq = (uint32_t)(atof(argv[argi++]) * 1e6);
    if (argi < argc) gain = atoi(argv[argi++]) * 10;

    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    device_list_t dev_list;
    device_enumerate(&dev_list);

    int dev_index = 0;
    if (dev_list.count == 0) {
        dev_index = ui_device_selector(&dev_list);
        if (dev_index < 0) {
            SDL_Quit();
            return 0;
        }
    } else if (dev_list.count > 1) {
        dev_index = ui_device_selector(&dev_list);
        if (dev_index < 0) {
            SDL_Quit();
            return 0;
        }
    } else {
        dev_index = 0;
        sdr_log("Auto-selecting device: %s %s (SN: %s)\n",
                dev_list.devices[0].manufacturer, dev_list.devices[0].product,
                dev_list.devices[0].serial);
    }

    sdr_device_t device;
    dsp_state_t dsp;
    audio_state_t audio;
    ui_state_t ui;

    if (device_open(&device, dev_index) < 0) {
        sdr_log("Failed to open SDR device %d.\n", dev_index);
        SDL_Quit();
        return 1;
    }

    device_set_freq(&device, freq);
    device_set_gain(&device, gain);

    dsp_init(&dsp);

    int audio_rate = AUDIO_RATE;
    if (audio_init(&audio, audio_rate) < 0) {
        sdr_log("Audio init failed, continuing without audio.\n");
    }

    if (ui_init(&ui, &device, &dsp, &audio) < 0) {
        sdr_log("UI init failed.\n");
        device_close(&device);
        SDL_Quit();
        return 1;
    }

    device_start_streaming(&device);

    sdr_log("\n=== MSi-SDR Receiver ===\n");
    sdr_log("Freq: %.3f MHz | Gain: %.1f dB | Rate: %.3f MS/s\n",
            device.freq / 1e6, device.gain / 10.0, device.sample_rate / 1e6);
    fprintf(stderr, "Controls:\n");
    fprintf(stderr, "  Up/Down     - tune +/- 100 kHz\n");
    fprintf(stderr, "  Left/Right  - tune +/- 1 MHz\n");
    fprintf(stderr, "  Mouse wheel - tune +/- 100 kHz\n");
    fprintf(stderr, "  Click       - tune to frequency on display\n");
    fprintf(stderr, "  PgUp/PgDn   - gain +/- 5 dB\n");
    fprintf(stderr, "  +/-         - volume\n");
    fprintf(stderr, "  M           - mute/unmute\n");
    fprintf(stderr, "  D           - toggle FM demod\n");
    fprintf(stderr, "  1-5         - band presets (LF1/LF2/MW/HF/FM)\n");
    fprintf(stderr, "  Q/Esc       - quit\n\n");

    int16_t *iq_buf = malloc(sizeof(int16_t) * 1024 * 1024);
    int16_t *audio_buf = malloc(sizeof(int16_t) * 256 * 1024);

    while (g_running && ui.running) {
        if (!ui_handle_events(&ui)) break;

        if (!device.device_error) {
            size_t got = device_read_iq(&device, iq_buf, 512 * 1024);
            int iq_pairs = got / 2;

            if (iq_pairs >= FFT_SIZE) {
                dsp_process_iq(&dsp, iq_buf, iq_pairs);

                if (ui.demod_enabled && audio.dev) {
                    int audio_len = 0;
                    dsp_fm_demod(&dsp, iq_buf, iq_pairs,
                                 audio_buf, &audio_len, ui.fm_decimation);
                    if (audio_len > 0) {
                        audio_push(&audio, audio_buf, audio_len);
                    }
                }
            }
        }

        ui_render(&ui);
        SDL_Delay(16);
    }

    sdr_log("\nShutting down...\n");

    free(audio_buf);
    free(iq_buf);
    device_stop_streaming(&device);
    ui_free(&ui);
    audio_free(&audio);
    dsp_free(&dsp);
    device_close(&device);
    if (g_logfile) fclose(g_logfile);
    SDL_Quit();

    return 0;
}
