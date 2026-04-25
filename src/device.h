#ifndef DEVICE_H
#define DEVICE_H

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

extern FILE *g_logfile;
void sdr_log(const char *fmt, ...);

#define IQ_RING_SIZE (1024 * 1024 * 4)
#define MAX_DEVICES 16

typedef struct {
    char name[256];
    char manufacturer[256];
    char product[256];
    char serial[256];
    uint32_t index;
} device_info_t;

typedef struct {
    device_info_t devices[MAX_DEVICES];
    int count;
} device_list_t;

int device_enumerate(device_list_t *list);

typedef struct {
    int16_t *buffer;
    volatile size_t write_pos;
    volatile size_t read_pos;
    size_t size;
    pthread_mutex_t mutex;
} iq_ring_t;

typedef struct {
    void *dev;
    uint32_t freq;
    uint32_t sample_rate;
    uint32_t bandwidth;
    int gain;
    volatile int running;
    volatile int device_error;
    iq_ring_t ring;
    char serial[256];
    char product[256];
    char manufacturer[256];
} sdr_device_t;

int device_open(sdr_device_t *d, uint32_t index);
void device_close(sdr_device_t *d);
int device_start_streaming(sdr_device_t *d);
void device_stop_streaming(sdr_device_t *d);
int device_set_freq(sdr_device_t *d, uint32_t freq);
int device_set_gain(sdr_device_t *d, int gain);
int device_set_sample_rate(sdr_device_t *d, uint32_t rate);
int device_set_bandwidth(sdr_device_t *d, uint32_t bw);
size_t device_read_iq(sdr_device_t *d, int16_t *out, size_t max_samples);

#endif
