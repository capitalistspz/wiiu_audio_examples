#pragma once

typedef struct
{
   unsigned int sample_rate;
   int channels;

   unsigned int setup_memory_required;
   unsigned int setup_temp_memory_required;
   unsigned int temp_memory_required;

   int max_frame_size;
} stb_vorbis_info;

struct stb_vorbis;
struct stb_vorbis_alloc;
int stb_vorbis_decode_filename(const char *filename, int *channels, int *sample_rate, short **output);
struct stb_vorbis * stb_vorbis_open_filename(const char *filename, int *error, const struct stb_vorbis_alloc *alloc_buffer);
stb_vorbis_info stb_vorbis_get_info(struct stb_vorbis *f);
void stb_vorbis_close(struct stb_vorbis *f);
int stb_vorbis_get_samples_short(struct stb_vorbis *f, int channels, short **buffer, int num_samples);
unsigned int stb_vorbis_stream_length_in_samples(struct stb_vorbis *f);