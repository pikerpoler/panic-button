/* Minimal SAM-style placeholder CLI: writes 8-bit mono 22050 Hz WAV from text length. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void write_wav(FILE* f, const uint8_t* samples, uint32_t count) {
    uint32_t data_size = count;
    uint32_t riff_size = 36 + data_size;
    uint16_t audio_format = 1;
    uint16_t channels = 1;
    uint32_t sample_rate = 22050;
    uint16_t bits = 8;
    uint32_t byte_rate = sample_rate * channels * bits / 8;
    uint16_t block_align = channels * bits / 8;

    fwrite("RIFF", 1, 4, f);
    fwrite(&riff_size, 4, 1, f);
    fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f);
    uint32_t fmt_size = 16;
    fwrite(&fmt_size, 4, 1, f);
    fwrite(&audio_format, 2, 1, f);
    fwrite(&channels, 2, 1, f);
    fwrite(&sample_rate, 4, 1, f);
    fwrite(&byte_rate, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    fwrite(&bits, 2, 1, f);
    fwrite("data", 1, 4, f);
    fwrite(&data_size, 4, 1, f);
    fwrite(samples, 1, count, f);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <text> <out.wav>\n", argv[0]);
        return 1;
    }
    const char* text = argv[1];
    const char* out_path = argv[2];
    size_t len = strlen(text);
    double duration = 0.04 * (double)len;
    if (duration < 0.6) {
        duration = 0.6;
    }
    if (duration > 1.8) {
        duration = 1.8;
    }

    const uint32_t sample_rate = 22050;
    const uint32_t count = (uint32_t)(duration * sample_rate);
    uint8_t* samples = (uint8_t*)malloc(count);
    if (!samples) {
        return 1;
    }

    for (uint32_t i = 0; i < count; ++i) {
        const double t = (double)i / sample_rate;
        const double base = 220.0 + 30.0 * sin(t * 3.1);
        const double mod = 0.5 + 0.5 * sin(t * 12.0 + len);
        const int v = (int)(127.0 + 90.0 * sin(2.0 * M_PI * base * t) * mod);
        samples[i] = (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
    }

    FILE* f = fopen(out_path, "wb");
    if (!f) {
        free(samples);
        return 1;
    }
    write_wav(f, samples, count);
    fclose(f);
    free(samples);
    return 0;
}
