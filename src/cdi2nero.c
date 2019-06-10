#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "common.h"
#include "cdi2nero.h"
#include "buffer.h"
#include "cdi.h"

// Global variables

char *global_read_buffer_ptr;
char *global_write_buffer_ptr;

char CUES_S[] = "CUES";
char DAOI_S[] = "DAOI";
char SINF_S[] = "SINF";
char ETNF_S[] = "ETNF";

// --- BEGIN SiZ! modification (14 nov 06) ----
void printhelp() {
     printf("usage : cdi2nero <input.cdi> [output.nrg] [-t]\n");
     printf("\t-t : generate a TAO image instead of DAO\n");
}
// --- END SiZ! modif ---

// Main function ---------------------------------------------------------------

int main( int argc, char **argv )
{

unsigned long i;
long          nrg_offset;
int           fcues_i = 0, fdaoi_i = 0;
char          fsourcename[MAX_PATH], fdestname[MAX_PATH];
char          *fcues, *fdaoi;
FILE          *fsource, *fdest;

image_s image = { 0, };
track_s track = { 0, };
opts_s  opts  = { 0, };

image.global_current_session = 0;
track.global_current_track   = 0;
track.position               = 0;

opts.track_format = FORMAT_DAOI;

// Opciones y ayuda ------------------------------------------------------------

       printf("CDI2nero 0.9.9 - (C) 2002 by DeXT\n");
       printf("Modified by SiZ! - 14 nov 06\n\n");

       if (argc < 2) // no params
          {
          opts.ask_source_image = 1;
          opts.ask_dest_image = 1;
          }

       if (argc == 2) // only input.cdi specified
          {
          strcpy(fsourcename, argv[1]);
          strcpy(fdestname, fsourcename);
          if (stricmp(fsourcename+(strlen(fsourcename)-4), ".cdi"))
             strcpy(fdestname+(strlen(fdestname)), ".nrg");
          else
             strcpy(fdestname+(strlen(fdestname)-4), ".nrg");
          }
        
// SiZ! modifications
        if (argc == 3) { // input.cdi and (output.nrg OR TAO option)
           strcpy(fsourcename, argv[1]);
           strcpy(fdestname, argv[2]);

           if (stricmp(fdestname, "-t") == 0) {// is the TAO option NOT the output filename
              
              strcpy(fdestname, fsourcename);

              if (stricmp(fsourcename+(strlen(fsourcename)-4), ".cdi"))
                 strcpy(fdestname+(strlen(fdestname)), ".nrg");
              else
                  strcpy(fdestname+(strlen(fdestname)-4), ".nrg");
             } else {
               
             }
        }

       if (argc > 3) // input.cdi, output.nrg and TAO option
          {
          strcpy(fsourcename, argv[1]);
          strcpy(fdestname, argv[2]);

          if (stricmp(fdestname+(strlen(fdestname)-4), ".nrg"))
             strcpy(fdestname+(strlen(fdestname)), ".nrg");
          }
        
       if (opts.ask_source_image)
          {
          if (askfilename(fsourcename) == CANCELLED)
             {
             printf("Exit requested by user");
#ifdef _WIN32
             FreeConsole();
             ExitProcess(0);
#else
             exit (EXIT_SUCCESS);
#endif
             }
          }

       if (opts.ask_dest_image)
          {
          opts.image_format = asksavefilename(fdestname);
          if (opts.image_format == CANCELLED)
             {
             printf("Exit requested by user");
#ifdef _WIN32
             FreeConsole();
             ExitProcess(0);
#else
             exit (EXIT_SUCCESS);
#endif
             }
          }

       if (opts.image_format != TAO) opts.image_format = DAO;

// --- BEGIN SiZ! modification (14 nov 06) ----
       int c = 0;
       while((c = getopt(argc, argv, "th")) != EOF) {
               switch(c) {
                         case 't' : opts.image_format = TAO; // TAO mode instead of DAO
                         break;
                         
                         case 'h' : printhelp(); // help
#ifdef _WIN32
                                    FreeConsole();
                                    ExitProcess(0);
#else
                                    exit (EXIT_SUCCESS);
#endif    
                         }
       }
// --- END SiZ! modif ---

// Abrir fichero ---------------------------------------------------------------

       printf("Searching file: '%s'\n",fsourcename);

       fsource = fopen(fsourcename,"rb");

       if (fsource == NULL)
          {
          if (errno == ENOENT)  // "No such file or directory"
             {
             strcat(fsourcename, ".cdi");
             printf("Not found. Trying '%s' instead...\n",fsourcename);
             fsource = fopen(fsourcename,"rb");
             }
          }

       if (fsource == NULL) error_exit(ERR_OPENIMAGE, fsourcename);

       errno = 0;
       printf("Found image file. Opening...\n");

       CDI_init (fsource, &image, fsourcename);

       if (image.version == CDI_V2)
          printf("This is a v2.0 image\n");
       else if (image.version == CDI_V3)
          printf("This is a v3.0 image\n");
       else if (image.version == CDI_V35)
          printf("This is a v4.0 image\n"); // SiZ! modification (useless, but...)
       else
          error_exit(ERR_GENERIC, "Unsupported image version");

//     printf("Header offset: %ld\n",image.header_offset);

       printf("\nAnalyzing image...\n");

       CDI_get_sessions (fsource, &image);

       if (image.sessions == 0)  error_exit(ERR_GENERIC, "Bad format: Could not find header");

       printf("Found %d session(s)\n",image.sessions);

//       printf("Creating NRG file...\n");

       switch(opts.image_format)
       {
          case TAO: printf("Creating NRG file (TAO format)...\n"); break;
          case DAO: printf("Creating NRG file (DAO format)...\n"); break;
       }

       fdest = fopen(fdestname, "wb");
       if (fdest == NULL) error_exit(ERR_SAVEIMAGE, fsourcename);

// Allocating buffers

       global_read_buffer_ptr = (char *) malloc (READ_BUF_SIZE);
       global_write_buffer_ptr = (char *) malloc (WRITE_BUF_SIZE);

       if (global_read_buffer_ptr == NULL || global_write_buffer_ptr == NULL)
          error_exit(ERR_GENERIC, "Not enough free memory for buffers!");

       image.remaining_sessions = image.sessions;

/////////////////////////////////////////////////////////////// Bucle sessions

       while (image.remaining_sessions > 0)
             {
             image.global_current_session++;

             CDI_get_tracks (fsource, &image);

             image.header_position = ftell(fsource); // inicio de pista

             printf("\nSession %d has %d track(s)\n",image.global_current_session,image.tracks);

             if (image.tracks == 0)
                {
                printf("Open session\n");
                break;                      // terminar bucle si open session
                }

             if (image.global_current_session == 1 && opts.image_format == DAO)
                opts.track_format = FORMAT_DAOI;
             else
                opts.track_format = FORMAT_ETNF;

             if (opts.track_format == FORMAT_DAOI)                          // header
                {                                                           //
                track.mode = ask_type(fsource, image.header_position);      //
                fcues = (char *) malloc (((image.tracks + 1) * 16) + 1024); //
                NRG_write_cues_hdr (fcues, &fcues_i, &image, track.mode);   //
                fdaoi = (char *) malloc ((image.tracks * 30) + 22 + 2048);  //  incluye SINF, ETNF
                NRG_write_daoi_hdr (fdaoi, &fdaoi_i, &image);               //
                }                                                           //
             else
                {
                if (opts.image_format == TAO)
                   {
                   if (image.global_current_session == 1)
                      fdaoi = (char *) malloc ((image.tracks * 20) + 8 + 2048); // SINF, ETNF
                   }
                NRG_write_etnf_hdr (fdaoi, &fdaoi_i, &image);
                }

             image.remaining_tracks = image.tracks;

///////////////////////////////////////////////////////////////// Bucle tracks

             while (image.remaining_tracks > 0)
                   {
                   track.global_current_track++;
                   track.number = image.tracks - image.remaining_tracks + 1;

// Leer datos

                   CDI_read_track (fsource, &image, &track);

                   image.header_position = ftell(fsource);

// Mostrar info

                   printf("Saving  ");
                   printf("Track: %2d  ",track.global_current_track);
                   printf("Type: ");
                   switch (track.mode)
                          {
                          case 0 : printf("Audio/"); break;
                          case 1 : printf("Mode1/"); break;
                          case 2 :
                          default: printf("Mode2/");
                          }
                   printf("%d  ",track.sector_size);
//                 printf("Pregap: %ld  ",track.pregap_length);
                   printf("Size: %-6ld  ",track.length);
                   printf("LBA: %-6ld",track.start_lba);

//                 printf("\n");

//                 if (track.pregap_length != 150) printf("Warning! This track seems to have a non-standard pregap...\n");

                   if (track.length < 0 && opts.track_format == FORMAT_ETNF)
                      error_exit(ERR_GENERIC, "Negative track size found\n"
                                              "These tracks cannot be saved to a TAO image");

                   nrg_offset = ftell(fdest);                                          //

                   if (image.global_current_session > 1 && track.mode != 0 && opts.image_format == DAO)
                      opts.do_cut = 1;
                   else
                      opts.do_cut = 0;

                   if (opts.track_format == FORMAT_ETNF && track.sector_size == 2352 && track.mode == 2)
                      opts.do_convert = 1;   // 2352 -> 2336
                   else
                      opts.do_convert = 0;

                   if (opts.track_format == FORMAT_DAOI)                               // header
                      {                                                                //
                      NRG_write_cues_track (fcues, &fcues_i, &track);                  //

                      NRG_write_daoi_track (fdaoi, &fdaoi_i, &track, nrg_offset);      //
                      }                                                                //
                   else // if (opts.image_format == TAO)
                      {
                      NRG_write_etnf_track(fdaoi, &fdaoi_i, &track, &opts, nrg_offset);
                      }

// Guardar la pista

//                 last_track_position = track.position;

                   savetrack(fsource, fdest, &track, &image, &opts);
                   track.position = ftell(fsource);

                   fseek(fsource, image.header_position, SEEK_SET);

// Cerrar bucles

                   image.remaining_tracks--;
                   }

             if (opts.track_format == FORMAT_DAOI)               // header
                {                                                //
                NRG_write_cues_tail (fcues, &fcues_i, &track);   //
                }                                                //

             NRG_write_sinf (fdaoi, &fdaoi_i, &image);           //
/*
             if (opts.track_format == FORMAT_ETNF && opts.image_format == DAO)
                {                                                //
                NRG_write_etnf (fdaoi, &fdaoi_i, &track, &opts, nrg_offset);
                }                                                //
*/
             CDI_skip_next_session (fsource, &image);

             image.remaining_sessions--;
             }

             printf("\nInjecting NRG header...\n");                 // header

             nrg_offset = ftell(fdest);                             //

             if (opts.image_format == DAO) fwrite(fcues, fcues_i, 1, fdest);     // solo DAO
             fwrite(fdaoi, fdaoi_i, 1, fdest);                      //

             fwrite("END!", 4, 1, fdest);                           //
             fwrite("\0\0\0\0", 4, 1, fdest);                       //
             fwrite("NERO", 4, 1, fdest);                           //
             fwrite_as_big(&nrg_offset, 4, fdest);                  //

// Liberar buffers

             free(global_write_buffer_ptr);
             free(global_read_buffer_ptr);

// Finalizar

             printf("\nAll done!\n");

             printf("Good burnin'...\n");

             fclose(fdest);
             fclose(fsource);

             return 0;
}


//////////////////////////////////////////////////////////////////////////////


void savetrack(FILE *fsource, FILE *fdest, track_s *track, image_s *image, opts_s *opts)
{

unsigned long i, progress, total_progress;
long          track_length;
int           all_fine;
char          cvalue;
char          buffer[2352], filename [13];

struct buffer_s read_buffer;
struct buffer_s write_buffer;

     read_buffer.file = fsource;
     read_buffer.size = READ_BUF_SIZE;
     read_buffer.index = 0;
     read_buffer.ptr = global_read_buffer_ptr;
     write_buffer.file = fdest;
     write_buffer.size = WRITE_BUF_SIZE;
     write_buffer.index = 0;
     write_buffer.ptr = global_write_buffer_ptr;
     
     fseek(fsource, track->position, SEEK_SET);

     if (opts->track_format == FORMAT_ETNF && opts->do_cut)
        track_length = track->length - 2;
     else
        track_length = track->length;

     if (opts->track_format == FORMAT_ETNF)
        fseek(fsource, track->pregap_length*track->sector_size, SEEK_CUR);
     else
        track_length += track->pregap_length;   // add pregap

/* track->length no puede ser negativo cuando track_format == FORMAT_ETNF */
/* ETNF no soporta pistas < 150 sectores (<pregap) */

     printf("\n");

     for(i = 0; i < track_length; i++)
        {
#ifndef DEBUG_CDI
              if (!(i%128)) {
                            progress = (i*100/track_length);
                            total_progress = ((ftell(fsource)>>10)*100)/(image->length>>10);
                            printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
                                   "[Current: %02d%%  Total: %02d%%]  ",progress, total_progress);
                            }
#endif
              BufRead (buffer, track->sector_size, &read_buffer, image->length);

              if (opts->do_convert)
                 {
                 all_fine = BufWrite (buffer + 16, 2336, &write_buffer);
                 }
              else
                 {
                 all_fine = BufWrite (buffer, track->sector_size, &write_buffer);
                 }

              if (!all_fine) error_exit(ERR_SAVETRACK, filename);
        }

     printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
            "                            "
            "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");

// Calcular posicion (BufRead no seguro, tb cuttrack & pregap)

     fseek(fsource, track->position, SEEK_SET);
//     fseek(fsource, track->pregap_length * track->sector_size, SEEK_CUR);
//     fseek(fsource, track->length * track->sector_size, SEEK_CUR);
     fseek(fsource, track->total_length * track->sector_size, SEEK_CUR);

// Vaciar buffers

     BufWriteFlush(&write_buffer);

}


//////////////////////////////////////////////////////////////////////////////


unsigned char askfilename(char *string)
{

#ifdef _WIN32
HWND hWnd = NULL;
OPENFILENAME ofn;
char szFile[MAX_PATH];

     memset(&ofn, 0L, sizeof(ofn));
     szFile[0] = 0;

#ifdef __BORLANDC__
     ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
#else
     ofn.lStructSize = sizeof(ofn);
#endif
     ofn.hwndOwner = hWnd;
     ofn.lpstrFile = szFile;
     ofn.nMaxFile = sizeof(szFile);
     ofn.lpstrFilter = "DiscJuggler image (*.cdi;*.cdr)\0*.cdi;*.cdr\0All files (*.*)\0*.*\0\0";
     ofn.nFilterIndex = 1;
     ofn.lpstrDefExt = "cdi";
     ofn.lpstrTitle = "Select CDI image to open...";
     ofn.lpstrFileTitle = NULL;
     ofn.nMaxFileTitle = 0;
     ofn.lpstrInitialDir = NULL;
     ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;

     if (GetOpenFileName(&ofn))
        strcpy (string, szFile);
     else
        return CANCELLED;

#else
     printf("\nPlease enter name of CDI image (return to exit): ");
     gets(string);
     if (strlen(string) == 0) return CANCELLED;
#endif

     return 0; // OK
}


//////////////////////////////////////////////////////////////////////////////


unsigned char asksavefilename(char *string)
{

#ifdef _WIN32
HWND hWnd = NULL;
OPENFILENAME ofn;
char szFile[MAX_PATH];

     memset(&ofn, 0L, sizeof(ofn));
     strcpy(szFile, "image.nrg");

#ifdef __BORLANDC__
     ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
#else
     ofn.lStructSize = sizeof(ofn);
#endif
     ofn.hwndOwner = hWnd;
     ofn.lpstrFile = szFile;
     ofn.nMaxFile = sizeof(szFile);
     ofn.lpstrFilter = "Nero DAO image (*.nrg)\0*.nrg\0Nero TAO image (*.nrg)\0*.nrg\0All files (*.*)\0*.*\0\0";
     ofn.nFilterIndex = 1;
     ofn.lpstrDefExt = "nrg";
     ofn.lpstrTitle = "Enter destination NRG image...";
     ofn.lpstrFileTitle = NULL;
     ofn.nMaxFileTitle = 0;
     ofn.lpstrInitialDir = NULL;
     ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;

     if (GetSaveFileName(&ofn))
        strcpy (string, szFile);
     else
        return CANCELLED;

#else
     printf("\nPlease enter name of NRG image (return to exit): ");
     gets(string);
     if (strlen(string) == 0) return CANCELLED;
#endif

     return ofn.nFilterIndex;  // 1=DAO, 2=TAO
}


//////////////////////////////////////////////////////////////////////////////


unsigned long lba_to_msf(unsigned long lba)
{
unsigned long m, s, f;
unsigned long current_msf;

       m = (lba / 60) / 75;
       s = (lba - m*60*75)  / 75;
       f = (lba - m*60*75 - s*75);

       current_msf = (bcd_conv(f) | bcd_conv(s)<<8 | bcd_conv(m)<<16);

return(current_msf);
}


//////////////////////////////////////////////////////////////////////////////


unsigned long bcd_conv(unsigned long value)
{
       unsigned long a,b;
	
       a = (value/10)*16;
       b = value%10;
	
       return (a + b);
}


//////////////////////////////////////////////////////////////////////////////


void mem_write(void *data, long size, int *index, char *buffer)
{
      memcpy (&(buffer[*index]), data, size);
      *index += size;
}

void mem_write_as_big(void *data, long size, int *index, char *buffer)
{
int i;
char *src;

src = data;

#ifdef BIG_ENDIAN
      for (i=0; i < size; i++)
          buffer[*index+i] = src[i];
#else
      for (i=0; i < size; i++)
          buffer[*index+i] = src[size-(i+1)];
#endif

      *index += size;
}

void mem_write_int_big(long value, long size, int *index, char *buffer)
{
 switch(size)
       {
       case 4:
              buffer[*index+0] = value >> 24;
              buffer[*index+1] = value >> 16;
              buffer[*index+2] = value >> 8;
              buffer[*index+3] = value;
              break;
       case 2:
              buffer[*index+0] = value >> 8;
              buffer[*index+1] = value;
              break;
       case 1:
              buffer[*index+0] = value;
       }

      *index += size;
}


//////////////////////////////////////////////////////////////////////////////


void NRG_write_cues_hdr (char *fcues, int *fcues_i, image_s *image, unsigned long track_type)
{

unsigned long value;

       mem_write(CUES_S, 4, fcues_i, fcues);
       mem_write_int_big(((image->tracks + 1) * 16), 4, fcues_i, fcues);
       if (track_type != 0)
          value = 0x41000000;
       else
          value = 0x01000000;
       mem_write_int_big(value, 4, fcues_i, fcues);
       mem_write_int_big(0L, 4, fcues_i, fcues);
}


void NRG_write_daoi_hdr (char *fdaoi, int *fdaoi_i, image_s *image)
{

unsigned long value;

       mem_write(DAOI_S, 4, fdaoi_i, fdaoi);
       mem_write_int_big((22 + (30*image->tracks)), 4, fdaoi_i, fdaoi);
       mem_write_int_big((22 + (30*image->tracks)), 4, fdaoi_i, fdaoi);
       mem_write_int_big(0L, 4, fdaoi_i, fdaoi);
       mem_write_int_big(0L, 4, fdaoi_i, fdaoi);
       mem_write_int_big(0L, 4, fdaoi_i, fdaoi);
       mem_write_int_big(0L, 2, fdaoi_i, fdaoi);
       mem_write_int_big(0x20, 1, fdaoi_i, fdaoi);
       mem_write_int_big(0L, 1, fdaoi_i, fdaoi);
       mem_write_int_big(0x01, 1, fdaoi_i, fdaoi);
       mem_write_int_big(image->tracks, 1, fdaoi_i, fdaoi);
}


void NRG_write_cues_track (char *fcues, int *fcues_i, track_s *track)
{

unsigned long value;
unsigned long current_msf;

       if (track->mode != 0)
          value = 0x41000000;
       else
          value = 0x01000000;
       value |= (bcd_conv(track->global_current_track)<<16) & 0x00ff0000;
       mem_write_int_big(value, 4, fcues_i, fcues);
       current_msf = lba_to_msf(track->start_lba);
       mem_write_int_big(current_msf, 4, fcues_i, fcues);
       value |= 0x0100;
       mem_write_int_big(value, 4, fcues_i, fcues);
       if (track->length < 0)
          current_msf = lba_to_msf(track->start_lba); // pregap = 0
       else
          current_msf = lba_to_msf(track->start_lba + track->pregap_length);
       mem_write_int_big(current_msf, 4, fcues_i, fcues);
}

void NRG_write_daoi_track (char *fdaoi, int *fdaoi_i, track_s *track, long nrg_offset)
{

unsigned long value;

       mem_write_int_big(0L, 4, fdaoi_i, fdaoi);
       mem_write_int_big(0L, 4, fdaoi_i, fdaoi);
       mem_write_int_big(0L, 2, fdaoi_i, fdaoi);
       mem_write_int_big(track->sector_size, 4, fdaoi_i, fdaoi);
       if (track->mode == 0)
          value = 0x07000001;
       else
          value = 0x03000001;
       mem_write_int_big(value, 4, fdaoi_i, fdaoi);
       mem_write_int_big(nrg_offset, 4, fdaoi_i, fdaoi);
       if (track->length < 0)
          value = nrg_offset; // pregap = 0
       else
          value = nrg_offset + (track->pregap_length*track->sector_size);
       mem_write_int_big(value, 4, fdaoi_i, fdaoi);
       value = nrg_offset + ((track->pregap_length + track->length)*track->sector_size);
       mem_write_int_big(value, 4, fdaoi_i, fdaoi);
}

void NRG_write_cues_tail (char *fcues, int *fcues_i, track_s *track)
{

unsigned long value;
unsigned long current_msf;

       if (track->mode != 0)
          value = 0x41000000;
       else
          value = 0x01000000;
       value |= 0xAA0100;
       mem_write_int_big(value, 4, fcues_i, fcues);
       current_msf = lba_to_msf(track->start_lba + track->length + track->pregap_length);
       mem_write_int_big(current_msf, 4, fcues_i, fcues);
}

void NRG_write_sinf (char *fdaoi, int *fdaoi_i, image_s *image)
{

       mem_write(SINF_S, 4, fdaoi_i, fdaoi);
       mem_write_int_big(0x04, 4, fdaoi_i, fdaoi);
       mem_write_int_big(image->tracks, 4, fdaoi_i, fdaoi);
}

void NRG_write_etnf_hdr (char *fdaoi, int *fdaoi_i, image_s *image)
{

       mem_write(ETNF_S, 4, fdaoi_i, fdaoi);
       mem_write_int_big((image->tracks * 20), 4, fdaoi_i, fdaoi);
}

void NRG_write_etnf_track (char *fdaoi, int *fdaoi_i, track_s *track, opts_s *opts, long nrg_offset)
{

unsigned long value;
unsigned long sector_size;

       if (opts->do_convert)    // conversion
          sector_size = 2336;
       else
          sector_size = track->sector_size;

       mem_write_int_big(nrg_offset, 4, fdaoi_i, fdaoi);
       if (opts->do_cut)                        // igual que savetrack
          value = track->length - 2;            // arreglar
       else
          value = track->length;                // no puede ser negativo!
       mem_write_int_big((value*sector_size), 4, fdaoi_i, fdaoi);
       if (track->mode != 0)                    // arreglar
          value = 0x03;
       else
          value = 0x07;
       mem_write_int_big(value, 4, fdaoi_i, fdaoi);
       mem_write_int_big(track->start_lba, 4, fdaoi_i, fdaoi);
       mem_write_int_big(0L, 4, fdaoi_i, fdaoi);
}


