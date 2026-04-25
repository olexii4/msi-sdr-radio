#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include "device.h"
#include "mirisdr.h"

#define STREAM_MAX_RETRIES 10
#define STREAM_RETRY_DELAY_US 1000000

FILE *g_logfile = NULL;

void sdr_log(const char *fmt, ...) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm;
    localtime_r(&ts.tv_sec, &tm);
    char timebuf[32];
    snprintf(timebuf, sizeof(timebuf), "%02d:%02d:%02d.%03ld",
             tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);

    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[%s] ", timebuf);
    vfprintf(stderr, fmt, args);
    va_end(args);

    if (g_logfile) {
        va_start(args, fmt);
        fprintf(g_logfile, "[%s] ", timebuf);
        vfprintf(g_logfile, fmt, args);
        fflush(g_logfile);
        va_end(args);
    }
}

int device_enumerate(device_list_t *list) {
    memset(list, 0, sizeof(*list));
    uint32_t count = mirisdr_get_device_count();
    if (count > MAX_DEVICES) count = MAX_DEVICES;
    list->count = (int)count;

    for (uint32_t i = 0; i < count; i++) {
        device_info_t *d = &list->devices[i];
        d->index = i;
        const char *name = mirisdr_get_device_name(i);
        if (name) {
            strncpy(d->name, name, sizeof(d->name) - 1);
        }
        mirisdr_get_device_usb_strings(i, d->manufacturer, d->product, d->serial);
    }

    return list->count;
}

static void ring_init(iq_ring_t *r) {
    r->size = IQ_RING_SIZE;
    r->buffer = calloc(r->size, sizeof(int16_t));
    r->write_pos = 0;
    r->read_pos = 0;
    pthread_mutex_init(&r->mutex, NULL);
}

static void ring_free(iq_ring_t *r) {
    free(r->buffer);
    r->buffer = NULL;
    pthread_mutex_destroy(&r->mutex);
}

static void ring_write(iq_ring_t *r, const int16_t *data, size_t count) {
    for (size_t i = 0; i < count; i++) {
        r->buffer[r->write_pos % r->size] = data[i];
        r->write_pos++;
    }
}

static void async_callback(unsigned char *buf, uint32_t len, void *ctx) {
    sdr_device_t *d = (sdr_device_t *)ctx;
    if (!d->running) return;
    size_t samples = len / 2;
    ring_write(&d->ring, (int16_t *)buf, samples);
}

int device_open(sdr_device_t *d, uint32_t index) {
    memset(d, 0, sizeof(*d));
    ring_init(&d->ring);

    uint32_t count = mirisdr_get_device_count();
    if (count == 0) {
        sdr_log("No SDR devices found.\n");
        return -1;
    }

    sdr_log("Found %u device(s).\n", count);

    mirisdr_dev_t *dev = NULL;
    int r = mirisdr_open(&dev, index);
    if (r < 0) {
        sdr_log("Failed to open device #%u.\n", index);
        return -1;
    }

    d->dev = dev;
    mirisdr_get_usb_strings(dev, d->manufacturer, d->product, d->serial);
    sdr_log("Opened: %s %s (SN: %s)\n", d->manufacturer, d->product, d->serial);

    d->freq = 100000000;
    d->sample_rate = 2048000;
    d->bandwidth = 8000000;
    d->gain = 400;

    mirisdr_set_hw_flavour(dev, MIRISDR_HW_DEFAULT);

    mirisdr_set_sample_rate(dev, d->sample_rate);
    mirisdr_set_center_freq(dev, d->freq);
    mirisdr_set_sample_format(dev, "AUTO");
    mirisdr_set_if_freq(dev, 0);
    mirisdr_set_bandwidth(dev, 8000000);

    mirisdr_set_tuner_gain_mode(dev, 1);
    mirisdr_set_tuner_gain(dev, d->gain / 10);

    return 0;
}

void device_close(sdr_device_t *d) {
    if (d->dev) {
        device_stop_streaming(d);
        int wait = 0;
        while (d->running && wait < 30) {
            usleep(100000);
            wait++;
        }
        if (!d->device_error) {
            mirisdr_close((mirisdr_dev_t *)d->dev);
        }
        d->dev = NULL;
    }
    ring_free(&d->ring);
}

static void *streaming_thread(void *arg) {
    sdr_device_t *d = (sdr_device_t *)arg;
    int retries = 0;

    mirisdr_reset_buffer((mirisdr_dev_t *)d->dev);

    while (d->running && retries < STREAM_MAX_RETRIES) {
        int r = mirisdr_read_async((mirisdr_dev_t *)d->dev, async_callback, d, 32, 16 * 16384);

        if (!d->running) break;

        if (mirisdr_reset_buffer((mirisdr_dev_t *)d->dev) < 0) {
            sdr_log("USB device disconnected.\n");
            d->device_error = 1;
            break;
        }

        retries++;
        sdr_log("Streaming error (code %d), retry %d/%d...\n",
                r, retries, STREAM_MAX_RETRIES);
        usleep(STREAM_RETRY_DELAY_US);
    }

    if (d->running && retries >= STREAM_MAX_RETRIES) {
        sdr_log("Device lost after %d retries.\n", retries);
        d->device_error = 1;
    }
    d->running = 0;
    return NULL;
}

int device_start_streaming(sdr_device_t *d) {
    if (d->running) return 0;
    d->running = 1;
    pthread_t thread;
    pthread_create(&thread, NULL, streaming_thread, d);
    pthread_detach(thread);
    return 0;
}

void device_stop_streaming(sdr_device_t *d) {
    if (!d->running) return;
    d->running = 0;
    if (!d->device_error && d->dev) {
        mirisdr_cancel_async((mirisdr_dev_t *)d->dev);
    }
}

int device_set_freq(sdr_device_t *d, uint32_t freq) {
    if (d->device_error || !d->dev) return -1;
    d->freq = freq;
    return mirisdr_set_center_freq((mirisdr_dev_t *)d->dev, freq);
}

int device_set_gain(sdr_device_t *d, int gain) {
    if (d->device_error || !d->dev) return -1;
    d->gain = gain;
    return mirisdr_set_tuner_gain((mirisdr_dev_t *)d->dev, gain / 10);
}

int device_set_sample_rate(sdr_device_t *d, uint32_t rate) {
    if (d->device_error || !d->dev) return -1;
    d->sample_rate = rate;
    return mirisdr_set_sample_rate((mirisdr_dev_t *)d->dev, rate);
}

int device_set_bandwidth(sdr_device_t *d, uint32_t bw) {
    if (d->device_error || !d->dev) return -1;
    d->bandwidth = bw;
    return mirisdr_set_bandwidth((mirisdr_dev_t *)d->dev, bw);
}

size_t device_read_iq(sdr_device_t *d, int16_t *out, size_t max_samples) {
    iq_ring_t *r = &d->ring;
    size_t avail = r->write_pos - r->read_pos;
    if (avail > max_samples) avail = max_samples;
    for (size_t i = 0; i < avail; i++) {
        out[i] = r->buffer[r->read_pos % r->size];
        r->read_pos++;
    }
    return avail;
}
