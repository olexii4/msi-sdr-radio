#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include "ui.h"

#define WINDOW_W 1024
#define WINDOW_H 700
#define SPECTRUM_H 200
#define WATERFALL_H 400
#define CONTROLS_H 120
#define DB_MIN (-100.0f)
#define DB_MAX (-20.0f)

typedef struct {
    const char *name;
    uint32_t freq;
    uint32_t bandwidth;
    uint32_t freq_min;
    uint32_t freq_max;
} band_preset_t;

static const band_preset_t band_presets[] = {
    {"LF1",   65000,     200000,   30000,     100000},
    {"LF2",   200000,    300000,   100000,    300000},
    {"MW",    1650000,   1536000,  300000,    3000000},
    {"HF",    16500000,  5000000,  3000000,   30000000},
    {"FM",    100000000, 8000000,  88000000,  108000000},
};
#define NUM_BAND_PRESETS 5

static const uint32_t std_bw[] = {8000000, 7000000, 6000000, 5000000, 1536000, 600000, 300000, 200000};
#define NUM_STD_BW 8

static uint32_t auto_bandwidth(uint32_t freq, uint32_t freq_min, uint32_t freq_max) {
    uint32_t room_lo = freq - freq_min;
    uint32_t room_hi = freq_max - freq;
    uint32_t max_half = room_lo < room_hi ? room_lo : room_hi;
    uint32_t max_bw = max_half * 2;
    for (int i = 0; i < NUM_STD_BW; i++) {
        if (std_bw[i] <= max_bw) return std_bw[i];
    }
    return 200000;
}

static void apply_band_clamp(ui_state_t *ui, uint32_t *freq) {
    if (ui->active_band < 0) return;
    const band_preset_t *bp = &band_presets[ui->active_band];
    if (*freq < bp->freq_min) *freq = bp->freq_min;
    if (*freq > bp->freq_max) *freq = bp->freq_max;
    device_set_freq(ui->device, *freq);
}

typedef struct { float x1, y1, x2, y2; } seg_t;

static const seg_t font_0[] = {{0,0,1,0},{1,0,1,1},{1,1,0,1},{0,1,0,0}};
static const seg_t font_1[] = {{0.5f,0,0.5f,1}};
static const seg_t font_2[] = {{0,0,1,0},{1,0,1,0.5f},{1,0.5f,0,0.5f},{0,0.5f,0,1},{0,1,1,1}};
static const seg_t font_3[] = {{0,0,1,0},{1,0,1,1},{1,1,0,1},{0,0.5f,1,0.5f}};
static const seg_t font_4[] = {{0,0,0,0.5f},{0,0.5f,1,0.5f},{1,0,1,1}};
static const seg_t font_5[] = {{1,0,0,0},{0,0,0,0.5f},{0,0.5f,1,0.5f},{1,0.5f,1,1},{1,1,0,1}};
static const seg_t font_6[] = {{1,0,0,0},{0,0,0,1},{0,1,1,1},{1,1,1,0.5f},{1,0.5f,0,0.5f}};
static const seg_t font_7[] = {{0,0,1,0},{1,0,1,1}};
static const seg_t font_8[] = {{0,0,1,0},{1,0,1,1},{1,1,0,1},{0,1,0,0},{0,0.5f,1,0.5f}};
static const seg_t font_9[] = {{1,1,1,0},{1,0,0,0},{0,0,0,0.5f},{0,0.5f,1,0.5f}};
static const seg_t font_dot[] = {{0.4f,0.9f,0.6f,0.9f},{0.4f,0.9f,0.4f,1},{0.6f,0.9f,0.6f,1},{0.4f,1,0.6f,1}};
static const seg_t font_minus[] = {{0,0.5f,1,0.5f}};
static const seg_t font_pct[] = {{0,0,0.3f,0},{0.3f,0,0.3f,0.3f},{0,0.3f,0,0},{0,1,1,0},{0.7f,0.7f,1,0.7f},{1,0.7f,1,1},{1,1,0.7f,1},{0.7f,1,0.7f,0.7f}};

static const seg_t font_A[] = {{0,1,0,0.3f},{0,0.3f,0.5f,0},{0.5f,0,1,0.3f},{1,0.3f,1,1},{0,0.5f,1,0.5f}};
static const seg_t font_B[] = {{0,0,0,1},{0,0,0.8f,0},{0.8f,0,1,0.15f},{1,0.15f,0.8f,0.5f},{0,0.5f,0.8f,0.5f},{0.8f,0.5f,1,0.65f},{1,0.65f,1,0.85f},{1,0.85f,0.8f,1},{0.8f,1,0,1}};
static const seg_t font_D[] = {{0,0,0,1},{0,0,0.7f,0},{0.7f,0,1,0.3f},{1,0.3f,1,0.7f},{1,0.7f,0.7f,1},{0.7f,1,0,1}};
static const seg_t font_F[] = {{0,0,1,0},{0,0,0,1},{0,0.5f,0.7f,0.5f}};
static const seg_t font_G[] = {{1,0.2f,0.8f,0},{0.8f,0,0.2f,0},{0.2f,0,0,0.2f},{0,0.2f,0,0.8f},{0,0.8f,0.2f,1},{0.2f,1,0.8f,1},{0.8f,1,1,0.8f},{1,0.8f,1,0.5f},{1,0.5f,0.5f,0.5f}};
static const seg_t font_H[] = {{0,0,0,1},{1,0,1,1},{0,0.5f,1,0.5f}};
static const seg_t font_K[] = {{0,0,0,1},{1,0,0,0.5f},{0,0.5f,1,1}};
static const seg_t font_M[] = {{0,1,0,0},{0,0,0.5f,0.4f},{0.5f,0.4f,1,0},{1,0,1,1}};
static const seg_t font_N[] = {{0,1,0,0},{0,0,1,1},{1,1,1,0}};
static const seg_t font_O[] = {{0.2f,0,0.8f,0},{0.8f,0,1,0.2f},{1,0.2f,1,0.8f},{1,0.8f,0.8f,1},{0.8f,1,0.2f,1},{0.2f,1,0,0.8f},{0,0.8f,0,0.2f},{0,0.2f,0.2f,0}};
static const seg_t font_S[] = {{1,0.1f,0.8f,0},{0.8f,0,0.2f,0},{0.2f,0,0,0.15f},{0,0.15f,0,0.35f},{0,0.35f,0.2f,0.5f},{0.2f,0.5f,0.8f,0.5f},{0.8f,0.5f,1,0.65f},{1,0.65f,1,0.85f},{1,0.85f,0.8f,1},{0.8f,1,0.2f,1},{0.2f,1,0,0.9f}};
static const seg_t font_V[] = {{0,0,0.5f,1},{0.5f,1,1,0}};
static const seg_t font_E[] = {{0,0,1,0},{0,0,0,1},{0,0.5f,0.7f,0.5f},{0,1,1,1}};
static const seg_t font_I[] = {{0.2f,0,0.8f,0},{0.5f,0,0.5f,1},{0.2f,1,0.8f,1}};
static const seg_t font_L[] = {{0,0,0,1},{0,1,1,1}};
static const seg_t font_T[] = {{0,0,1,0},{0.5f,0,0.5f,1}};
static const seg_t font_U[] = {{0,0,0,0.8f},{0,0.8f,0.2f,1},{0.2f,1,0.8f,1},{0.8f,1,1,0.8f},{1,0.8f,1,0}};
static const seg_t font_C[] = {{0.8f,0,0.2f,0},{0.2f,0,0,0.2f},{0,0.2f,0,0.8f},{0,0.8f,0.2f,1},{0.2f,1,0.8f,1}};
static const seg_t font_J[] = {{0.3f,0,1,0},{0.7f,0,0.7f,0.8f},{0.7f,0.8f,0.5f,1},{0.5f,1,0.2f,1},{0.2f,1,0,0.8f}};
static const seg_t font_P[] = {{0,0,0,1},{0,0,0.8f,0},{0.8f,0,1,0.15f},{1,0.15f,1,0.35f},{1,0.35f,0.8f,0.5f},{0.8f,0.5f,0,0.5f}};
static const seg_t font_Q[] = {{0.2f,0,0.8f,0},{0.8f,0,1,0.2f},{1,0.2f,1,0.7f},{1,0.7f,0.8f,0.9f},{0.8f,0.9f,0.8f,1},{0.8f,1,0.2f,1},{0.2f,1,0,0.8f},{0,0.8f,0,0.2f},{0,0.2f,0.2f,0},{0.6f,0.7f,1,1}};
static const seg_t font_R[] = {{0,0,0,1},{0,0,0.8f,0},{0.8f,0,1,0.15f},{1,0.15f,1,0.35f},{1,0.35f,0.8f,0.5f},{0.8f,0.5f,0,0.5f},{0.5f,0.5f,1,1}};
static const seg_t font_W[] = {{0,0,0.25f,1},{0.25f,1,0.5f,0.5f},{0.5f,0.5f,0.75f,1},{0.75f,1,1,0}};
static const seg_t font_X[] = {{0,0,1,1},{1,0,0,1}};
static const seg_t font_Y[] = {{0,0,0.5f,0.5f},{1,0,0.5f,0.5f},{0.5f,0.5f,0.5f,1}};
static const seg_t font_z[] = {{0,0.4f,1,0.4f},{1,0.4f,0,1},{0,1,1,1}};
static const seg_t font_lbracket[] = {{0.7f,0,0.3f,0},{0.3f,0,0.3f,1},{0.3f,1,0.7f,1}};
static const seg_t font_rbracket[] = {{0.3f,0,0.7f,0},{0.7f,0,0.7f,1},{0.7f,1,0.3f,1}};
static const seg_t font_lparen[] = {{0.7f,0,0.3f,0.3f},{0.3f,0.3f,0.3f,0.7f},{0.3f,0.7f,0.7f,1}};
static const seg_t font_rparen[] = {{0.3f,0,0.7f,0.3f},{0.7f,0.3f,0.7f,0.7f},{0.7f,0.7f,0.3f,1}};
static const seg_t font_pipe[] = {{0.5f,0,0.5f,1}};
static const seg_t font_slash[] = {{0,1,1,0}};
static const seg_t font_colon[] = {{0.4f,0.2f,0.6f,0.2f},{0.4f,0.35f,0.6f,0.35f},{0.4f,0.65f,0.6f,0.65f},{0.4f,0.8f,0.6f,0.8f}};

struct glyph { char ch; const seg_t *segs; int n; };
#define G(c, arr) { c, arr, sizeof(arr)/sizeof(arr[0]) }
static const struct glyph glyphs[] = {
    G('0',font_0),G('1',font_1),G('2',font_2),G('3',font_3),G('4',font_4),
    G('5',font_5),G('6',font_6),G('7',font_7),G('8',font_8),G('9',font_9),
    G('.',font_dot),G('-',font_minus),G('%',font_pct),G(':',font_colon),
    G('/',font_slash),G('|',font_pipe),
    G('[',font_lbracket),G(']',font_rbracket),
    G('(',font_lparen),G(')',font_rparen),
    G('A',font_A),G('B',font_B),G('C',font_C),G('D',font_D),G('E',font_E),
    G('F',font_F),G('G',font_G),G('H',font_H),G('I',font_I),G('J',font_J),
    G('K',font_K),G('L',font_L),G('M',font_M),G('N',font_N),G('O',font_O),
    G('P',font_P),G('Q',font_Q),G('R',font_R),G('S',font_S),G('T',font_T),
    G('U',font_U),G('V',font_V),G('W',font_W),G('X',font_X),G('Y',font_Y),
    G('z',font_z),
};
#undef G

static void draw_char(char ch, float x, float y, float w, float h) {
    if (ch == ' ') return;
    for (int i = 0; i < (int)(sizeof(glyphs)/sizeof(glyphs[0])); i++) {
        if (glyphs[i].ch == ch) {
            glBegin(GL_LINES);
            for (int j = 0; j < glyphs[i].n; j++) {
                glVertex2f(x + glyphs[i].segs[j].x1 * w, y + glyphs[i].segs[j].y1 * h);
                glVertex2f(x + glyphs[i].segs[j].x2 * w, y + glyphs[i].segs[j].y2 * h);
            }
            glEnd();
            return;
        }
    }
}

static void draw_text(const char *str, float x, float y, float char_h) {
    float cw = char_h * 0.6f;
    float gap = char_h * 0.15f;
    for (int i = 0; str[i]; i++) {
        draw_char(str[i], x, y, cw, char_h);
        x += cw + gap;
    }
}

static float text_width(const char *str, float char_h) {
    float cw = char_h * 0.6f;
    float gap = char_h * 0.15f;
    int len = (int)strlen(str);
    if (len == 0) return 0;
    return len * cw + (len - 1) * gap;
}

static void color_from_db(float db, float *r, float *g, float *b) {
    float t = (db - DB_MIN) / (DB_MAX - DB_MIN);
    if (t < 0) t = 0;
    if (t > 1) t = 1;

    if (t < 0.25f) {
        *r = 0; *g = 0; *b = t * 4;
    } else if (t < 0.5f) {
        *r = 0; *g = (t - 0.25f) * 4; *b = 1;
    } else if (t < 0.75f) {
        *r = (t - 0.5f) * 4; *g = 1; *b = 1.0f - (t - 0.5f) * 4;
    } else {
        *r = 1; *g = 1.0f - (t - 0.75f) * 4; *b = 0;
    }
}

static void render_spectrum(ui_state_t *ui) {
    dsp_state_t *dsp = ui->dsp;
    float w = ui->width;
    float y_top = ui->height - SPECTRUM_H - WATERFALL_H - CONTROLS_H;
    float y_bot = y_top + SPECTRUM_H;

    glColor3f(0.1f, 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex2f(0, y_top); glVertex2f(w, y_top);
    glVertex2f(w, y_bot); glVertex2f(0, y_bot);
    glEnd();

    glColor4f(0.3f, 0.3f, 0.3f, 1.0f);
    glBegin(GL_LINES);
    for (int db = (int)DB_MIN; db <= (int)DB_MAX; db += 10) {
        float y = y_bot - ((db - DB_MIN) / (DB_MAX - DB_MIN)) * SPECTRUM_H;
        glVertex2f(0, y); glVertex2f(w, y);
    }
    glEnd();

    glColor3f(0.2f, 0.8f, 0.2f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < SPECTRUM_SIZE; i++) {
        float x = (float)i / SPECTRUM_SIZE * w;
        float db = dsp->spectrum_avg[i];
        float y = y_bot - ((db - DB_MIN) / (DB_MAX - DB_MIN)) * SPECTRUM_H;
        if (y < y_top) y = y_top;
        if (y > y_bot) y = y_bot;
        glVertex2f(x, y);
    }
    glEnd();

    glColor4f(0.1f, 0.5f, 0.1f, 0.3f);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i < SPECTRUM_SIZE; i++) {
        float x = (float)i / SPECTRUM_SIZE * w;
        float db = dsp->spectrum_avg[i];
        float y = y_bot - ((db - DB_MIN) / (DB_MAX - DB_MIN)) * SPECTRUM_H;
        if (y < y_top) y = y_top;
        if (y > y_bot) y = y_bot;
        glVertex2f(x, y);
        glVertex2f(x, y_bot);
    }
    glEnd();

    float center_freq = ui->device->freq / 1e6f;
    float bw = ui->device->sample_rate / 1e6f;
    glColor3f(0.6f, 0.6f, 0.6f);
    int n_labels = 5;
    for (int i = 0; i <= n_labels; i++) {
        float frac = (float)i / n_labels;
        float freq_label = center_freq - bw / 2 + frac * bw;
        float lx = frac * w;
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", freq_label);
        float tw = text_width(buf, 10);
        float tx = lx - tw / 2;
        if (tx < 0) tx = 0;
        if (tx + tw > w) tx = w - tw;
        draw_text(buf, tx, y_bot - 14, 10);
    }

    glColor4f(1.0f, 0.3f, 0.3f, 0.6f);
    float cx = w / 2;
    glBegin(GL_LINES);
    glVertex2f(cx, y_top);
    glVertex2f(cx, y_bot);
    glEnd();
}

static void render_waterfall(ui_state_t *ui) {
    dsp_state_t *dsp = ui->dsp;
    float w = ui->width;
    float y_top = ui->height - WATERFALL_H - CONTROLS_H;
    float row_h = (float)WATERFALL_H / 512;
    float col_w = w / SPECTRUM_SIZE;

    glBegin(GL_QUADS);
    for (int row = 0; row < 512; row++) {
        int idx = (dsp->waterfall_row - 1 - row + 512) % 512;
        float y = y_top + row * row_h;
        for (int col = 0; col < SPECTRUM_SIZE; col++) {
            float r, g, b;
            color_from_db(dsp->waterfall[idx][col], &r, &g, &b);
            glColor3f(r, g, b);
            float x = col * col_w;
            glVertex2f(x, y);
            glVertex2f(x + col_w, y);
            glVertex2f(x + col_w, y + row_h);
            glVertex2f(x, y + row_h);
        }
    }
    glEnd();
}

static void render_controls(ui_state_t *ui) {
    float w = ui->width;
    float y_top = ui->height - CONTROLS_H;

    glColor3f(0.15f, 0.15f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(0, y_top); glVertex2f(w, y_top);
    glVertex2f(w, ui->height); glVertex2f(0, ui->height);
    glEnd();

    char freq_str[32];
    if (ui->device->freq >= 1000000) {
        snprintf(freq_str, sizeof(freq_str), "%.3f MHZ", ui->device->freq / 1e6f);
    } else {
        snprintf(freq_str, sizeof(freq_str), "%.1f KHZ", ui->device->freq / 1e3f);
    }
    float freq_h = 22;
    float freq_tw = text_width(freq_str, freq_h);
    float freq_x = (w - freq_tw) / 2;
    float freq_y = y_top + 10;

    glColor3f(1.0f, 0.5f, 0.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(freq_x - 10, freq_y - 4);
    glVertex2f(freq_x + freq_tw + 10, freq_y - 4);
    glVertex2f(freq_x + freq_tw + 10, freq_y + freq_h + 4);
    glVertex2f(freq_x - 10, freq_y + freq_h + 4);
    glEnd();

    glColor3f(1.0f, 0.8f, 0.0f);
    draw_text(freq_str, freq_x, freq_y, freq_h);

    char gain_str[32];
    snprintf(gain_str, sizeof(gain_str), "GAIN: %.1f DB", ui->device->gain / 10.0f);
    float gain_x = w - 200;
    float gain_frac = ui->device->gain / 1020.0f;

    glColor3f(0.6f, 0.8f, 1.0f);
    draw_text(gain_str, gain_x, y_top + 10, 12);

    glColor3f(0.3f, 0.3f, 0.4f);
    glBegin(GL_QUADS);
    glVertex2f(gain_x, y_top + 28);
    glVertex2f(gain_x + 160, y_top + 28);
    glVertex2f(gain_x + 160, y_top + 40);
    glVertex2f(gain_x, y_top + 40);
    glEnd();
    glColor3f(0.2f, 0.7f, 0.9f);
    glBegin(GL_QUADS);
    glVertex2f(gain_x, y_top + 28);
    glVertex2f(gain_x + 160 * gain_frac, y_top + 28);
    glVertex2f(gain_x + 160 * gain_frac, y_top + 40);
    glVertex2f(gain_x, y_top + 40);
    glEnd();

    char vol_str[32];
    snprintf(vol_str, sizeof(vol_str), "VOL: %.0f%%", ui->audio->volume * 100);
    float vol_x = 30;
    float vol_frac = ui->audio->volume;

    glColor3f(0.5f, 1.0f, 0.5f);
    draw_text(vol_str, vol_x, y_top + 10, 12);

    glColor3f(0.3f, 0.3f, 0.4f);
    glBegin(GL_QUADS);
    glVertex2f(vol_x, y_top + 28);
    glVertex2f(vol_x + 160, y_top + 28);
    glVertex2f(vol_x + 160, y_top + 40);
    glVertex2f(vol_x, y_top + 40);
    glEnd();
    glColor3f(0.2f, 0.9f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(vol_x, y_top + 28);
    glVertex2f(vol_x + 160 * vol_frac, y_top + 28);
    glVertex2f(vol_x + 160 * vol_frac, y_top + 40);
    glVertex2f(vol_x, y_top + 40);
    glEnd();

    float btn_w = 60, btn_h = 18, btn_gap = 8;
    float total_btn_w = NUM_BAND_PRESETS * btn_w + (NUM_BAND_PRESETS - 1) * btn_gap;
    float btn_x0 = (w - total_btn_w) / 2;
    float btn_y = y_top + 48;

    for (int i = 0; i < NUM_BAND_PRESETS; i++) {
        float bx = btn_x0 + i * (btn_w + btn_gap);
        if (ui->active_band == i) {
            glColor3f(1.0f, 0.5f, 0.0f);
            glBegin(GL_QUADS);
            glVertex2f(bx, btn_y); glVertex2f(bx + btn_w, btn_y);
            glVertex2f(bx + btn_w, btn_y + btn_h); glVertex2f(bx, btn_y + btn_h);
            glEnd();
            glColor3f(1.0f, 1.0f, 1.0f);
        } else {
            glColor3f(0.4f, 0.4f, 0.5f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(bx, btn_y); glVertex2f(bx + btn_w, btn_y);
            glVertex2f(bx + btn_w, btn_y + btn_h); glVertex2f(bx, btn_y + btn_h);
            glEnd();
            glColor3f(0.6f, 0.6f, 0.7f);
        }
        float tw = text_width(band_presets[i].name, 12);
        draw_text(band_presets[i].name, bx + (btn_w - tw) / 2, btn_y + 3, 12);
    }

    char demod_str[32];
    snprintf(demod_str, sizeof(demod_str), "FM: %s", ui->demod_enabled ? "ON" : "OFF");
    float demod_x = (w - text_width(demod_str, 14)) / 2;
    if (ui->demod_enabled) {
        glColor3f(0.2f, 1.0f, 0.4f);
    } else {
        glColor3f(0.6f, 0.3f, 0.3f);
    }
    draw_text(demod_str, demod_x, y_top + 70, 14);

    if (ui->audio->muted) {
        glColor3f(1.0f, 0.2f, 0.2f);
        draw_text("MUTED", vol_x, y_top + 70, 12);
    }

    char bw_str[32];
    if (ui->device->bandwidth >= 1000000) {
        snprintf(bw_str, sizeof(bw_str), "BW: %.1f MHZ", ui->device->bandwidth / 1e6f);
    } else {
        snprintf(bw_str, sizeof(bw_str), "BW: %u KHZ", ui->device->bandwidth / 1000);
    }

    char sr_str[32];
    snprintf(sr_str, sizeof(sr_str), "%.3f MS/S", ui->device->sample_rate / 1e6f);
    glColor3f(0.5f, 0.5f, 0.6f);
    float sr_tw = text_width(sr_str, 10);
    draw_text(sr_str, w - sr_tw - 10, y_top + 100, 10);
    float bw_tw = text_width(bw_str, 10);
    draw_text(bw_str, w - sr_tw - bw_tw - 30, y_top + 100, 10);

    char sn_str[32];
    snprintf(sn_str, sizeof(sn_str), "SN:%s", ui->device->serial);
    glColor3f(0.4f, 0.4f, 0.5f);
    draw_text(sn_str, 10, y_top + 100, 10);
}

int ui_init(ui_state_t *ui, sdr_device_t *dev, dsp_state_t *dsp, audio_state_t *audio) {
    memset(ui, 0, sizeof(*ui));
    ui->device = dev;
    ui->dsp = dsp;
    ui->audio = audio;
    ui->width = WINDOW_W;
    ui->height = WINDOW_H;
    ui->demod_enabled = 1;
    ui->fm_decimation = dev->sample_rate / AUDIO_RATE;
    ui->active_band = 4;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    ui->window = SDL_CreateWindow(
        "MCi-SDR Receiver",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        ui->width, ui->height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!ui->window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        return -1;
    }

    ui->gl = SDL_GL_CreateContext(ui->window);
    if (!ui->gl) {
        fprintf(stderr, "GL context failed: %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_SetSwapInterval(1);
    ui->running = 1;

    return 0;
}

void ui_free(ui_state_t *ui) {
    if (ui->gl) SDL_GL_DeleteContext(ui->gl);
    if (ui->window) SDL_DestroyWindow(ui->window);
}

void ui_render(ui_state_t *ui) {
    int draw_w, draw_h;
    SDL_GL_GetDrawableSize(ui->window, &draw_w, &draw_h);

    glViewport(0, 0, draw_w, draw_h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, ui->width, ui->height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    render_spectrum(ui);
    render_waterfall(ui);
    render_controls(ui);

    SDL_GL_SwapWindow(ui->window);
}

int ui_handle_events(ui_state_t *ui) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT:
            ui->running = 0;
            return 0;

        case SDL_WINDOWEVENT:
            if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                ui->width = ev.window.data1;
                ui->height = ev.window.data2;
            }
            break;

        case SDL_KEYDOWN:
            switch (ev.key.keysym.sym) {
            case SDLK_ESCAPE:
            case SDLK_q:
                ui->running = 0;
                return 0;
            case SDLK_UP: {
                uint32_t step = ui->device->bandwidth / 200;
                if (step < 1000) step = 1000;
                uint32_t freq = ui->device->freq + step;
                apply_band_clamp(ui, &freq);
                if (ui->active_band < 0)
                    device_set_freq(ui->device, freq);
                sdr_log("Freq: %.3f MHz\n", ui->device->freq / 1e6);
                break;
            }
            case SDLK_DOWN: {
                uint32_t step = ui->device->bandwidth / 200;
                if (step < 1000) step = 1000;
                uint32_t freq = ui->device->freq > step ? ui->device->freq - step : 0;
                apply_band_clamp(ui, &freq);
                if (ui->active_band < 0 && freq > 0)
                    device_set_freq(ui->device, freq);
                sdr_log("Freq: %.3f MHz\n", ui->device->freq / 1e6);
                break;
            }
            case SDLK_RIGHT: {
                uint32_t step = ui->device->bandwidth / 20;
                if (step < 1000) step = 1000;
                uint32_t freq = ui->device->freq + step;
                apply_band_clamp(ui, &freq);
                if (ui->active_band < 0)
                    device_set_freq(ui->device, freq);
                sdr_log("Freq: %.3f MHz\n", ui->device->freq / 1e6);
                break;
            }
            case SDLK_LEFT: {
                uint32_t step = ui->device->bandwidth / 20;
                if (step < 1000) step = 1000;
                uint32_t freq = ui->device->freq > step ? ui->device->freq - step : 0;
                apply_band_clamp(ui, &freq);
                if (ui->active_band < 0 && freq > 0)
                    device_set_freq(ui->device, freq);
                sdr_log("Freq: %.3f MHz\n", ui->device->freq / 1e6);
                break;
            }
            case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4: case SDLK_5: {
                int band = ev.key.keysym.sym - SDLK_1;
                ui->active_band = band;
                device_set_freq(ui->device, band_presets[band].freq);
                device_set_bandwidth(ui->device, band_presets[band].bandwidth);
                sdr_log("Band: %s (%.3f MHz, BW %u kHz)\n",
                        band_presets[band].name,
                        band_presets[band].freq / 1e6,
                        band_presets[band].bandwidth / 1000);
                break;
            }
            case SDLK_PAGEUP:
                if (ui->device->gain < 1020) {
                    device_set_gain(ui->device, ui->device->gain + 50);
                    sdr_log("Gain: %.1f dB\n", ui->device->gain / 10.0);
                }
                break;
            case SDLK_PAGEDOWN:
                if (ui->device->gain > 0) {
                    device_set_gain(ui->device, ui->device->gain - 50);
                    sdr_log("Gain: %.1f dB\n", ui->device->gain / 10.0);
                }
                break;
            case SDLK_m:
                ui->audio->muted = !ui->audio->muted;
                sdr_log("Audio: %s\n", ui->audio->muted ? "MUTED" : "ON");
                break;
            case SDLK_d:
                ui->demod_enabled = !ui->demod_enabled;
                sdr_log("FM Demod: %s\n", ui->demod_enabled ? "ON" : "OFF");
                break;
            case SDLK_PLUS:
            case SDLK_EQUALS:
                ui->audio->volume = fminf(1.0f, ui->audio->volume + 0.1f);
                sdr_log("Volume: %.0f%%\n", ui->audio->volume * 100);
                break;
            case SDLK_MINUS:
                ui->audio->volume = fmaxf(0.0f, ui->audio->volume - 0.1f);
                sdr_log("Volume: %.0f%%\n", ui->audio->volume * 100);
                break;
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (ev.button.button == SDL_BUTTON_LEFT) {
                int my = ev.button.y;
                int mx = ev.button.x;
                float ctrl_top = ui->height - CONTROLS_H;
                float gain_x = ui->width - 200;
                float vol_x = 30;

                if (my >= ctrl_top + 24 && my <= ctrl_top + 44) {
                    if (mx >= gain_x && mx <= gain_x + 160) {
                        float frac = (float)(mx - gain_x) / 160.0f;
                        if (frac < 0) frac = 0;
                        if (frac > 1) frac = 1;
                        int new_gain = (int)(frac * 1020);
                        device_set_gain(ui->device, new_gain);
                        sdr_log("Gain: %.1f dB\n", ui->device->gain / 10.0);
                        break;
                    }
                    if (mx >= vol_x && mx <= vol_x + 160) {
                        float frac = (float)(mx - vol_x) / 160.0f;
                        if (frac < 0) frac = 0;
                        if (frac > 1) frac = 1;
                        ui->audio->volume = frac;
                        sdr_log("Volume: %.0f%%\n", ui->audio->volume * 100);
                        break;
                    }
                }

                if (my >= ctrl_top + 44 && my <= ctrl_top + 70) {
                    float btn_w = 60, btn_gap = 8;
                    float total_btn_w = NUM_BAND_PRESETS * btn_w + (NUM_BAND_PRESETS - 1) * btn_gap;
                    float btn_x0 = (ui->width - total_btn_w) / 2;
                    for (int i = 0; i < NUM_BAND_PRESETS; i++) {
                        float bx = btn_x0 + i * (btn_w + btn_gap);
                        if (mx >= bx && mx <= bx + btn_w) {
                            ui->active_band = i;
                            device_set_freq(ui->device, band_presets[i].freq);
                            device_set_bandwidth(ui->device, band_presets[i].bandwidth);
                            sdr_log("Band: %s (%.3f MHz, BW %u kHz)\n",
                                    band_presets[i].name,
                                    band_presets[i].freq / 1e6,
                                    band_presets[i].bandwidth / 1000);
                            break;
                        }
                    }
                    break;
                }

                if (my >= ctrl_top + 66 && my <= ctrl_top + 88) {
                    float demod_tw = text_width("FM: ON", 14);
                    float demod_x = (ui->width - demod_tw) / 2;
                    if (mx >= demod_x - 5 && mx <= demod_x + demod_tw + 5) {
                        ui->demod_enabled = !ui->demod_enabled;
                        sdr_log("FM Demod: %s\n", ui->demod_enabled ? "ON" : "OFF");
                        break;
                    }
                }

                float y_waterfall_bot = SPECTRUM_H + WATERFALL_H;
                if (my >= 0 && my < y_waterfall_bot) {
                    float frac = (float)mx / ui->width;
                    float bw = ui->device->sample_rate;
                    float center = ui->device->freq;
                    uint32_t new_freq = (uint32_t)(center - bw/2 + frac * bw);
                    if (ui->active_band >= 0) {
                        apply_band_clamp(ui, &new_freq);
                    } else {
                        device_set_freq(ui->device, new_freq);
                    }
                    sdr_log("Clicked freq: %.3f MHz\n", ui->device->freq / 1e6);
                }
            }
            break;

        case SDL_MOUSEWHEEL: {
            uint32_t step = ui->device->bandwidth / 200;
            if (step < 1000) step = 1000;
            uint32_t freq = ui->device->freq;
            if (ev.wheel.y > 0) {
                freq += step;
            } else if (ev.wheel.y < 0) {
                freq = freq > step ? freq - step : 0;
            }
            if (ui->active_band >= 0) {
                apply_band_clamp(ui, &freq);
            } else {
                if (freq > 0) device_set_freq(ui->device, freq);
            }
            sdr_log("Freq: %.3f MHz\n", ui->device->freq / 1e6);
            break;
        }
        }
    }
    return 1;
}

#define SEL_W 640
#define SEL_H 480
#define ROW_H 40
#define ROW_PAD 8
#define TITLE_H 30
#define FOOTER_H 20

static void sel_render(SDL_Window *win, device_list_t *list, int selected, int hover) {
    int draw_w, draw_h, win_w, win_h;
    SDL_GL_GetDrawableSize(win, &draw_w, &draw_h);
    SDL_GetWindowSize(win, &win_w, &win_h);

    glViewport(0, 0, draw_w, draw_h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, win_w, win_h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.06f, 0.06f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor3f(1.0f, 0.7f, 0.0f);
    draw_text("MCI-SDR - SELECT DEVICE", 20, 15, TITLE_H);

    glColor3f(0.4f, 0.4f, 0.5f);
    glBegin(GL_LINES);
    glVertex2f(10, 55); glVertex2f(win_w - 10, 55);
    glEnd();

    float list_top = 65;

    if (list->count == 0) {
        glColor3f(0.7f, 0.3f, 0.3f);
        draw_text("NO DEVICES FOUND", (win_w - text_width("NO DEVICES FOUND", 20)) / 2, list_top + 60, 20);
        glColor3f(0.5f, 0.5f, 0.5f);
        draw_text("PRESS R TO REFRESH", (win_w - text_width("PRESS R TO REFRESH", 14)) / 2, list_top + 100, 14);
    } else {
        for (int i = 0; i < list->count; i++) {
            float ry = list_top + i * (ROW_H + ROW_PAD);
            device_info_t *d = &list->devices[i];

            if (i == selected) {
                glColor4f(1.0f, 0.5f, 0.0f, 0.2f);
                glBegin(GL_QUADS);
                glVertex2f(15, ry); glVertex2f(win_w - 15, ry);
                glVertex2f(win_w - 15, ry + ROW_H); glVertex2f(15, ry + ROW_H);
                glEnd();
                glColor3f(1.0f, 0.5f, 0.0f);
                glBegin(GL_LINE_LOOP);
                glVertex2f(15, ry); glVertex2f(win_w - 15, ry);
                glVertex2f(win_w - 15, ry + ROW_H); glVertex2f(15, ry + ROW_H);
                glEnd();
            } else if (i == hover) {
                glColor4f(0.3f, 0.4f, 0.6f, 0.2f);
                glBegin(GL_QUADS);
                glVertex2f(15, ry); glVertex2f(win_w - 15, ry);
                glVertex2f(win_w - 15, ry + ROW_H); glVertex2f(15, ry + ROW_H);
                glEnd();
            }

            char idx_str[8];
            snprintf(idx_str, sizeof(idx_str), "[%d]", d->index);
            glColor3f(0.5f, 0.5f, 0.6f);
            draw_text(idx_str, 25, ry + 4, 14);

            char dev_str[256];
            snprintf(dev_str, sizeof(dev_str), "%s %s", d->manufacturer, d->product);
            if (i == selected) {
                glColor3f(1.0f, 0.8f, 0.2f);
            } else {
                glColor3f(0.9f, 0.9f, 0.95f);
            }
            draw_text(dev_str, 80, ry + 4, 14);

            char sn_str[256];
            snprintf(sn_str, sizeof(sn_str), "SN: %s", d->serial);
            glColor3f(0.5f, 0.7f, 0.5f);
            draw_text(sn_str, 80, ry + 22, 11);
        }
    }

    glColor3f(0.4f, 0.4f, 0.5f);
    glBegin(GL_LINES);
    glVertex2f(10, win_h - 35); glVertex2f(win_w - 10, win_h - 35);
    glEnd();

    glColor3f(0.5f, 0.5f, 0.6f);
    const char *hint = "CLICK/ARROWS SELECT | ENTER OPEN | R REFRESH | ESC QUIT";
    float hw = text_width(hint, 10);
    draw_text(hint, (win_w - hw) / 2, win_h - 25, 10);

    SDL_GL_SwapWindow(win);
}

int ui_device_selector(device_list_t *list) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window *win = SDL_CreateWindow(
        "MCi-SDR - Select Device",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SEL_W, SEL_H,
        SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!win) return -1;

    SDL_GLContext gl = SDL_GL_CreateContext(win);
    if (!gl) { SDL_DestroyWindow(win); return -1; }
    SDL_GL_SetSwapInterval(1);

    int selected = list->count > 0 ? 0 : -1;
    int hover = -1;
    int result = -1;
    int running = 1;

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT:
                running = 0;
                break;
            case SDL_KEYDOWN:
                switch (ev.key.keysym.sym) {
                case SDLK_ESCAPE:
                case SDLK_q:
                    running = 0;
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    if (selected >= 0 && selected < list->count) {
                        result = list->devices[selected].index;
                        running = 0;
                    }
                    break;
                case SDLK_UP:
                    if (selected > 0) selected--;
                    break;
                case SDLK_DOWN:
                    if (selected < list->count - 1) selected++;
                    break;
                case SDLK_r:
                    device_enumerate(list);
                    selected = list->count > 0 ? 0 : -1;
                    hover = -1;
                    break;
                }
                break;
            case SDL_MOUSEMOTION: {
                int my = ev.motion.y;
                float list_top = 65;
                hover = -1;
                for (int i = 0; i < list->count; i++) {
                    float ry = list_top + i * (ROW_H + ROW_PAD);
                    if (my >= ry && my < ry + ROW_H && ev.motion.x >= 15 && ev.motion.x <= SEL_W - 15) {
                        hover = i;
                        break;
                    }
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    int my = ev.button.y;
                    float list_top = 65;
                    for (int i = 0; i < list->count; i++) {
                        float ry = list_top + i * (ROW_H + ROW_PAD);
                        if (my >= ry && my < ry + ROW_H && ev.button.x >= 15 && ev.button.x <= SEL_W - 15) {
                            if (selected == i) {
                                result = list->devices[i].index;
                                running = 0;
                            } else {
                                selected = i;
                            }
                            break;
                        }
                    }
                }
                break;
            }
        }

        sel_render(win, list, selected, hover);
        SDL_Delay(16);
    }

    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(win);
    return result;
}
