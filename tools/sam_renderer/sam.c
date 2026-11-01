/* CLI wrapper around vendored s-macke/SAM sources (see upstream/README.md). */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "reciter.h"
#include "sam.h"

int debug = 0;

static void write_wav(const char* filename, const char* buffer, int bufferlength) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        return;
    }

    fwrite("RIFF", 4, 1, file);
    unsigned int filesize = (unsigned int)bufferlength + 12 + 16 + 8 - 8;
    fwrite(&filesize, 4, 1, file);
    fwrite("WAVE", 4, 1, file);

    fwrite("fmt ", 4, 1, file);
    unsigned int fmtlength = 16;
    fwrite(&fmtlength, 4, 1, file);
    unsigned short int format = 1;
    fwrite(&format, 2, 1, file);
    unsigned short int channels = 1;
    fwrite(&channels, 2, 1, file);
    unsigned int samplerate = 22050;
    fwrite(&samplerate, 4, 1, file);
    fwrite(&samplerate, 4, 1, file);
    unsigned short int blockalign = 1;
    fwrite(&blockalign, 2, 1, file);
    unsigned short int bitspersample = 8;
    fwrite(&bitspersample, 2, 1, file);

    fwrite("data", 4, 1, file);
    fwrite(&bufferlength, 4, 1, file);
    fwrite(buffer, (size_t)bufferlength, 1, file);
    fclose(file);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <text> <out.wav>\n", argv[0]);
        return 1;
    }

    char input[256];
    memset(input, 0, sizeof(input));
    strncpy(input, argv[1], sizeof(input) - 2);

    for (size_t i = 0; input[i] != 0; ++i) {
        input[i] = (char)toupper((unsigned char)input[i]);
    }

    strncat(input, "[", sizeof(input) - 1);
    if (!TextToPhonemes((unsigned char*)input)) {
        fprintf(stderr, "reciter failed\n");
        return 1;
    }

    SetInput(input);
    if (!SAMMain()) {
        fprintf(stderr, "sam synthesis failed\n");
        return 1;
    }

    const int sample_count = GetBufferLength() / 50;
    write_wav(argv[2], GetBuffer(), sample_count);
    return 0;
}
