#ifndef __CDI2NERO_H__
#define __CDI2NERO_H__

/* Track format */

#define FORMAT_DAOI 0x01
#define FORMAT_ETNF 0x02

/* Save format */

#define DAO       1
#define TAO       2
#define CANCELLED 0xFF

/* Buffers */

#define READ_BUF_SIZE  1024*1024
#define WRITE_BUF_SIZE 1024*1024

/* Structures */

typedef struct opts_s
       {
       unsigned char ask_source_image;
       unsigned char ask_dest_image;
       unsigned char track_format;
       unsigned char image_format;
       unsigned char do_cut;
       unsigned char do_convert;
       } opts_s;

/* Functions */

void savetrack(FILE *fsource, FILE *fdest, track_s *track, image_s *image, opts_s *opts);

unsigned long lba_to_msf(unsigned long lba);
unsigned long bcd_conv(unsigned long value);

unsigned char askfilename(char *string);
unsigned char asksavefilename(char *string);

void mem_write(void *data, long size, int *index, char *buffer);
void mem_write_as_big(void *data, long size, int *index, char *buffer);
void mem_write_int_big(long value, long size, int *index, char *buffer);

void NRG_write_cues_hdr (char *fcues, int *fcues_i, image_s *image, unsigned long track_type);
void NRG_write_daoi_hdr (char *fdaoi, int *fdaoi_i, image_s *image);
void NRG_write_cues_track (char *fcues, int *fcues_i, track_s *track);
void NRG_write_daoi_track (char *fdaoi, int *fdaoi_i, track_s *track, long nrg_offset);
void NRG_write_cues_tail (char *fcues, int *fcues_i, track_s *track);
void NRG_write_sinf (char *fdaoi, int *fdaoi_i, image_s *image);
void NRG_write_etnf_hdr (char *fdaoi, int *fdaoi_i, image_s *image);
void NRG_write_etnf_track (char *fdaoi, int *fdaoi_i, track_s *track, opts_s *opts, long nrg_offset);

#endif

