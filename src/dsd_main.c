/*
 * Copyright (C) 2010 DSD Author
 * GPG Key ID: 0x3F1D7FD0 (74EF 430D F7F2 0A48 FCE6  F630 FAA2 635D 3F1D 7FD0)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#define _MAIN

#include "dsd.h"
#include "p25p1_const.h"
#include "x2tdma_const.h"
#include "dstar_const.h"
#include "nxdn_const.h"
#include "dmr_const.h"
#include "dpmr_const.h"
#include "provoice_const.h"
#include "git_ver.h"
#include "p25p1_heuristics.h"
#include "pa_devs.h"


int comp (const void *a, const void *b)
{
  if (*((const int *) a) == *((const int *) b))
    return 0;
  else if (*((const int *) a) < *((const int *) b))
    return -1;
  else
    return 1;
}

void noCarrier (dsd_opts * opts, dsd_state * state)
{
  state->dibit_buf_p = state->dibit_buf + 200;
  memset (state->dibit_buf, 0, sizeof (int) * 200);

  if (opts->mbe_out_f != NULL)
  {
    closeMbeOutFile (opts, state);
  }
  state->jitter = -1;
  state->lastsynctype = -1;
  state->carrier = 0;
  state->max = 15000;
  state->min = -15000;
  state->center = 0;
  state->err_str[0] = 0;
  sprintf(state->fsubtype, "              ");
  sprintf(state->ftype, "             ");
  state->color_code = -1;
  state->color_code_ok = 0;
  state->errs = 0;
  state->errs2 = 0;
  memset(state->ambe_ciphered, 0, sizeof(state->ambe_ciphered));
  memset(state->ambe_deciphered, 0, sizeof(state->ambe_deciphered));
  state->lasttg = 0;
  state->lastsrc = 0;
  state->lastp25type = 0;
  state->repeat = 0;
  state->nac = 0;
  state->numtdulc = 0;
  sprintf(state->slot1light, " slot1 ");
  sprintf(state->slot2light, " slot2 ");
  state->firstframe = 0;
  if (opts->audio_gain == (float) 0)
  {
    state->aout_gain = 25;
  }
  memset (state->aout_max_buf, 0, sizeof (float) * 200);
  state->aout_max_buf_p = state->aout_max_buf;
  state->aout_max_buf_idx = 0;
  sprintf(state->algid, "________");
  sprintf(state->keyid, "________________");
  mbe_initMbeParms (state->cur_mp, state->prev_mp, state->prev_mp_enhanced);

  /* Next part of the dPMR frame is unknown now */
  opts->dPMR_next_part_of_superframe = 0;

  /* The dPMR source and destination ID are now invalid */
  state->dPMRVoiceFS2Frame.CalledIDOk  = 0;
  state->dPMRVoiceFS2Frame.CallingIDOk = 0;
  memset(state->dPMRVoiceFS2Frame.CalledID, 0, 8);
  memset(state->dPMRVoiceFS2Frame.CallingID, 0, 8);

  /* Re-init the SACCH structure */
  memset(state->NxdnSacchRawPart, 0, sizeof(state->NxdnSacchRawPart));

  /* Re-init the FACCH1 structure */
  memset(state->NxdnFacch1RawPart, 0, sizeof(state->NxdnFacch1RawPart));

  /* Re-init the FACCH2 structure */
  memset(&state->NxdnFacch2RawPart, 0, sizeof(state->NxdnFacch2RawPart));

  // TODO : Is it really necessary ???
  memset(&state->NxdnElementsContent, 0, sizeof(state->NxdnElementsContent));

  /* Re-init the LICH structure content */
  memset(&state->NxdnLich, 0, sizeof(state->NxdnLich));

  /* Re-init DMR data structure */
  memset(&state->TS1SuperFrame.Data, 0, sizeof(DMRDataPDU_t));
  memset(&state->TS2SuperFrame.Data, 0, sizeof(DMRDataPDU_t));

} /* End noCarrier() */


void initOpts (dsd_opts * opts)
{
  opts->onesymbol = 10;
  opts->mbe_in_file[0] = 0;
  opts->mbe_in_f = NULL;
  opts->errorbars = 1;
  opts->datascope = 0;
  opts->symboltiming = 0;
  opts->verbose = 2;
  opts->p25enc = 0;
  opts->p25lc = 0;
  opts->p25status = 0;
  opts->p25tg = 0;
  opts->scoperate = 15;
  sprintf(opts->audio_in_dev, "/dev/audio");
  opts->audio_in_fd = -1;
#ifdef USE_PORTAUDIO
  opts->audio_in_pa_stream = NULL;
#endif
  sprintf(opts->audio_out_dev, "/dev/audio");
  opts->audio_out_fd = -1;
#ifdef USE_PORTAUDIO
  opts->audio_out_pa_stream = NULL;
#endif
  opts->split = 0;
  opts->playoffset = 0;
  opts->mbe_out_dir[0] = 0;
  opts->mbe_out_file[0] = 0;
  opts->mbe_out_path[0] = 0;
  opts->mbe_out_f = NULL;
  opts->audio_gain = 0;
  opts->audio_out = 1;
  opts->output_channel = 0;
  opts->output_num_channels = 0;
  opts->mc_out_buf = NULL;
  opts->mc_out_buf_size = 0;
  opts->wav_out_file[0] = 0;
  opts->wav_out_f = NULL;
  //opts->wav_out_fd = -1;
  opts->serial_baud = 115200;
  sprintf(opts->serial_dev, "/dev/ttyUSB0");
  opts->resume = 0;
  opts->frame_dstar = 0;
  opts->frame_x2tdma = 1;
  opts->frame_p25p1 = 1;
  opts->frame_nxdn48 = 0;
  opts->frame_nxdn96 = 1;
  opts->frame_dmr = 1;
  opts->frame_provoice = 0;
  opts->frame_dpmr = 0;
  opts->mod_c4fm = 1;
  opts->mod_qpsk = 1;
  opts->mod_gfsk = 1;
  opts->uvquality = 3;
  opts->inverted_x2tdma = 1;    // most transmitter + scanner + sound card combinations show inverted signals for this
  opts->inverted_dmr = 0;       // most transmitter + scanner + sound card combinations show non-inverted signals for this
  opts->inverted_dpmr = 0;      // most transmitter + scanner + sound card combinations show non-inverted signals for this
  opts->mod_threshold = 26;
  opts->ssize = 36;
  opts->msize = 15;
  opts->playfiles = 0;
  opts->delay = 0;
  opts->use_cosine_filter = 1;
  opts->unmute_encrypted_p25 = 0;

  opts->dPMR_curr_frame_is_encrypted = 0;
  opts->dPMR_next_part_of_superframe = 0;

  opts->EncryptionMode = MODE_UNENCRYPTED;


  opts->SlotToOnlyDecode = 0;

  memset(opts->UseSpecificDmr49BitsAmbeKeyStream, 0, sizeof(opts->UseSpecificDmr49BitsAmbeKeyStream));
  opts->UseSpecificDmr49BitsAmbeKeyStreamUsed = 0;
  memset(opts->UseSpecificDmrAmbeSuperFrameKeyStream, 0, sizeof(opts->UseSpecificDmrAmbeSuperFrameKeyStream));
  opts->UseSpecificDmrAmbeSuperFrameKeyStreamUsed = 0;

  memset(opts->UseSpecificNxdn49BitsAmbeKeyStream, 0, sizeof(opts->UseSpecificNxdn49BitsAmbeKeyStream));
  opts->UseSpecificNxdn49BitsAmbeKeyStreamUsed = 0;
  memset(opts->UseSpecificNxdnAmbeSuperFrameKeyStream, 0, sizeof(opts->UseSpecificNxdnAmbeSuperFrameKeyStream));
  opts->UseSpecificNxdnAmbeSuperFrameKeyStreamUsed = 0;

  memset(opts->UseSpecificdPMR49BitsAmbeKeyStream, 0, sizeof(opts->UseSpecificdPMR49BitsAmbeKeyStream));
  opts->UseSpecificdPMR49BitsAmbeKeyStreamUsed = 0;
  memset(opts->UseSpecificdPMRAmbeSuperFrameKeyStream, 0, sizeof(opts->UseSpecificdPMRAmbeSuperFrameKeyStream));
  opts->UseSpecificdPMRAmbeSuperFrameKeyStreamUsed = 0;
} /* End initOpts() */

void initState (dsd_state * state)
{
  int i, j;

  state->dibit_buf    = malloc (sizeof (int) * 1000000);
  state->dibit_buf_p  = state->dibit_buf + 200;
  memset (state->dibit_buf, 0, sizeof (int) * 200);
  state->repeat = 0;
  state->audio_out_buf = malloc (sizeof (short) * 1000000);
  memset (state->audio_out_buf, 0, 100 * sizeof (short));
  state->audio_out_buf_p = state->audio_out_buf + 100;
  state->audio_out_float_buf = malloc (sizeof (float) * 1000000);
  memset (state->audio_out_float_buf, 0, 100 * sizeof (float));
  state->audio_out_float_buf_p = state->audio_out_float_buf + 100;
  state->audio_out_idx = 0;
  state->audio_out_idx2 = 0;
  state->audio_out_temp_buf_p = state->audio_out_temp_buf;
  //state->wav_out_bytes = 0;
  state->center = 0;
  state->jitter = -1;
  state->synctype = -1;
  state->max = 15000;
  state->min = -15000;
  state->lmid = 0;
  state->umid = 0;
  state->minref = -12000;
  state->maxref = 12000;
  state->lastsample = 0;
  for (i = 0; i < 128; i++)
  {
    state->sbuf[i] = 0;
  }
  state->sidx = 0;
  for (i = 0; i < 1024; i++)
  {
    state->maxbuf[i] = 15000;
  }
  for (i = 0; i < 1024; i++)
  {
    state->minbuf[i] = -15000;
  }
  state->midx = 0;
  state->err_str[0] = 0;
  sprintf(state->fsubtype, "              ");
  sprintf(state->ftype, "             ");
  state->symbolcnt = 0;
  state->rf_mod = C4FM_MODE;
  state->numflips = 0;
  state->lastsynctype = -1;
  state->lastp25type = 0;
  state->offset = 0;
  state->carrier = 0;
  for (i = 0; i < 25; i++)
  {
    for (j = 0; j < 16; j++)
    {
      state->tg[i][j] = 48;
    }
  }
  state->tgcount = 0;
  state->lasttg = 0;
  state->lastsrc = 0;
  state->nac = 0;
  state->errs = 0;
  state->errs2 = 0;
  state->mbe_file_type = -1;
  state->optind = 0;
  state->numtdulc = 0;
  state->firstframe = 0;
  sprintf(state->slot1light, " slot0 ");
  sprintf(state->slot2light, " slot1 ");
  state->aout_gain = 25;
  memset (state->aout_max_buf, 0, sizeof (float) * 200);
  state->aout_max_buf_p = state->aout_max_buf;
  state->aout_max_buf_idx = 0;
  state->samplesPerSymbol = SAMPLE_PER_SYMBOL_DMR; /* DMR by default */
  state->symbolCenter = (SAMPLE_PER_SYMBOL_DMR / 2) - 1;  /* Should be equal to 4 at 48000 kHz */
  sprintf(state->algid, "________");
  sprintf(state->keyid, "________________");
  state->currentslot = 0;
  state->directmode = 0;
  state->cur_mp = malloc (sizeof (mbe_parms));
  state->prev_mp = malloc (sizeof (mbe_parms));
  state->prev_mp_enhanced = malloc (sizeof (mbe_parms));
  mbe_initMbeParms (state->cur_mp, state->prev_mp, state->prev_mp_enhanced);
  state->p25kid = 0;

  state->debug_audio_errors = 0;
  state->debug_header_errors = 0;
  state->debug_header_critical_errors = 0;

#ifdef TRACE_DSD
  state->debug_sample_index = 0;
  state->debug_label_file = NULL;
  state->debug_label_dibit_file = NULL;
  state->debug_label_imbe_file = NULL;
#endif

  initialize_p25_heuristics(&state->p25_heuristics);

  state->color_code = -1;
  state->color_code_ok = 0;

  /* Init the time */
  state->TimeYear   = 0;
  state->TimeMonth  = 0;
  state->TimeDay    = 0;
  state->TimeHour   = 0;
  state->TimeMinute = 0;
  state->TimeSecond = 0;

  memset(&state->TS1SuperFrame, 0, sizeof(TimeSlotVoiceSuperFrame_t));
  memset(&state->TS2SuperFrame, 0, sizeof(TimeSlotVoiceSuperFrame_t));
  memset(&state->dPMRVoiceFS2Frame, 0, sizeof(dPMRVoiceFS2Frame_t));

  state->CapacityPlusFlag = 0;

  state->printDMRRawVoiceFrameHex    = 0;
  state->printDMRRawVoiceFrameBin    = 0;
  state->printDMRRawDataFrameHex     = 0;
  state->printDMRRawDataFrameBin     = 0;
  state->printDMRAmbeVoiceSampleHex  = 0;
  state->printDMRAmbeVoiceSampleBin  = 0;

  state->printdPMRRawVoiceFrameHex   = 0;
  state->printdPMRRawVoiceFrameBin   = 0;
  state->printdPMRRawDataFrameHex    = 0;
  state->printdPMRRawDataFrameBin    = 0;
  state->printdPMRAmbeVoiceSampleHex = 0;
  state->printdPMRAmbeVoiceSampleBin = 0;

  state->printNXDNAmbeVoiceSampleHex = 0;

  state->special_display_format_enable = 0;
  state->display_raw_data = 0;

  memset(state->NxdnSacchRawPart, 0, sizeof(state->NxdnSacchRawPart));
  memset(state->NxdnFacch1RawPart, 0, sizeof(state->NxdnFacch1RawPart));
  memset(&state->NxdnFacch2RawPart, 0, sizeof(state->NxdnFacch2RawPart));
  memset(&state->NxdnElementsContent, 0, sizeof(state->NxdnElementsContent));
  memset(&state->NxdnLich, 0, sizeof(state->NxdnLich));

} /* End initState() */

void usage(void)
{
  fprintf(stderr, "\n");
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  dsd [options]            Live scanner mode\n");
  fprintf(stderr, "  dsd [options] -r <files> Read/Play saved mbe data from file(s)\n");
  fprintf(stderr, "  dsd -h                   Show help\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Display Options:\n");
  fprintf(stderr, "  -e            Show Frame Info and errorbars (default)\n");
  fprintf(stderr, "  -pe           Show P25 encryption sync bits\n");
  fprintf(stderr, "  -pl           Show P25 link control bits\n");
  fprintf(stderr, "  -ps           Show P25 status bits and low speed data\n");
  fprintf(stderr, "  -pt           Show P25 talkgroup info\n");
  fprintf(stderr, "  -q            Don't show Frame Info/errorbars\n");
  fprintf(stderr, "  -s            Datascope (disables other display options)\n");
  fprintf(stderr, "  -t            Show symbol timing during sync\n");
  fprintf(stderr, "  -v <num>      Frame information Verbosity\n");
  fprintf(stderr, "  -z <num>      Frame rate for datascope\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Input/Output options:\n");
  fprintf(stderr, "  -i <device>   Audio input device (default is /dev/audio, - for piped stdin)\n");
  fprintf(stderr, "  -o <device>   Audio output device (default is /dev/audio, - for piped stdout)\n");
  fprintf(stderr, "  -d <dir>      Create mbe data files, use this directory\n");
  fprintf(stderr, "  -r <files>    Read/Play saved mbe data from file(s)\n");
  fprintf(stderr, "  -g <num>      Audio output gain (default = 0 = auto, disable = -1)\n");
  fprintf(stderr, "  -n            Do not send synthesized speech to audio output device\n");
  fprintf(stderr, "  -w <file>     Output synthesized speech to a .wav file\n");
  fprintf(stderr, "  -c <num>      Output audio on channel <num> of a multi-channel device\n");
  fprintf(stderr, "                 (1-indexed, e.g., -c 3 for channel 3)\n");
  fprintf(stderr, "                 Device channel count is auto-detected from PortAudio\n");
  fprintf(stderr, "  -a            Display port audio devices\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Scanner control options:\n");
  fprintf(stderr, "  -B <num>      Serial port baud rate (default=115200)\n");
  fprintf(stderr, "  -C <device>   Serial port for scanner control (default=/dev/ttyUSB0)\n");
  fprintf(stderr, "  -R <num>      Resume scan after <num> TDULC frames or any PDU or TSDU\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Decoder options:\n");
  fprintf(stderr, "  -fa           Auto-detect frame type (default)\n");
  fprintf(stderr, "  -f1           Decode only P25 Phase 1\n");
  fprintf(stderr, "  -fd           Decode only D-STAR\n");
  fprintf(stderr, "  -fi           Decode only NXDN48* (6.25 kHz) / IDAS*\n");
  fprintf(stderr, "  -fn           Decode only NXDN96 (12.5 kHz)\n");
  fprintf(stderr, "  -fp           Decode only ProVoice*\n");
  fprintf(stderr, "  -fr           Decode only DMR/MOTOTRBO\n");
  fprintf(stderr, "  -fx           Decode only X2-TDMA\n");
  fprintf(stderr, "  -fm           Decode only dPMR*\n");
  fprintf(stderr, "  -l            Disable DMR/MOTOTRBO and NXDN input filtering\n");
  fprintf(stderr, "  -ma           Auto-select modulation optimizations (default)\n");
  fprintf(stderr, "  -mc           Use only C4FM modulation optimizations\n");
  fprintf(stderr, "  -mg           Use only GFSK modulation optimizations\n");
  fprintf(stderr, "  -mq           Use only QPSK modulation optimizations\n");
  fprintf(stderr, "  -pu           Unmute Encrypted P25\n");
  fprintf(stderr, "  -u <num>      Unvoiced speech quality (default=3)\n");
  fprintf(stderr, "  -xx           Expect non-inverted X2-TDMA signal\n");
  fprintf(stderr, "  -xr           Expect inverted DMR/MOTOTRBO signal\n");
  fprintf(stderr, "  -xd           Expect inverted ICOM dPMR signal\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  * denotes frame types that cannot be auto-detected.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Advanced decoder options:\n");
  fprintf(stderr, "  -A <num>      QPSK modulation auto detection threshold (default=26)\n");
  fprintf(stderr, "  -S <num>      Symbol buffer size for QPSK decision point tracking\n");
  fprintf(stderr, "                 (default=36)\n");
  fprintf(stderr, "  -M <num>      Min/Max buffer size for QPSK decision point tracking\n");
  fprintf(stderr, "                 (default=15)\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Audio synthesize options:\n");
  fprintf(stderr, "  -1            Synthesize audio for first DMR timeslot\n");
  fprintf(stderr, "  -2            Synthesize audio for second DMR timeslot\n");
  fprintf(stderr, "\n");
#ifdef BUILD_DSD_WITH_FRAME_CONTENT_DISPLAY
  fprintf(stderr, "Others options:\n");
  fprintf(stderr, "  -DMRPrintVoiceFrameHex        Print all DMR voice frame in hexadecimal format\n");
  fprintf(stderr, "  -DMRPrintVoiceFrameBin        Print all DMR voice frame in binary format\n");
  fprintf(stderr, "  -DMRPrintDataFrameHex         Print all DMR data frame in hexadecimal format\n");
  fprintf(stderr, "  -DMRPrintDataFrameHex         Print all DMR data frame in binary format\n");
  fprintf(stderr, "  -DMRPrintAmbeVoiceSampleHex   Print all DMR AMBE voice sample in hexadecimal format\n");
  fprintf(stderr, "  -DMRPrintAmbeVoiceSampleBin   Print all DMR AMBE voice sample in binary format\n");
  fprintf(stderr, "\n");
#endif /* BUILD_DSD_WITH_FRAME_CONTENT_DISPLAY */

  fprintf(stderr, "  -DMRForceAmbe49BitsKeyStreamHex=<value> Force AMBE voice decoding with a specific key stream\n");
  fprintf(stderr, "                                  <value> = a 7 bytes hexadecimal value\n");
  fprintf(stderr, "                                  Examples :\n");
  fprintf(stderr, "                                  <value> = 1F001F001F0000 to apply BP Key 1\n");
  fprintf(stderr, "                                  <value> = E300E300E30001 to apply BP Key 2\n");
  fprintf(stderr, "                                  <value> = FC00FC00FC0001 to apply BP Key 3\n");
  fprintf(stderr, "                                  <value> = 25032503250300 to apply BP Key 4\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -DMRForceAmbe49BitsKeyStreamBin=<value> Force AMBE voice decoding with a specific key stream\n");
  fprintf(stderr, "                                  <value> = a 49 bits length ASCII binary value string\n");
  fprintf(stderr, "                                  Examples :\n");
  fprintf(stderr, "                                  <value> = 0001111100000000000111110000000000011111000000000 to apply BP Key 1\n");
  fprintf(stderr, "                                  <value> = 1110001100000000111000110000000011100011000000001 to apply BP Key 2\n");
  fprintf(stderr, "                                  <value> = 1111110000000000111111000000000011111100000000001 to apply BP Key 3\n");
  fprintf(stderr, "                                  <value> = 0010010100000011001001010000001100100101000000110 to apply BP Key 4\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -DMRForceAmbeSuperFrameKeyStreamHex=<value> Force AMBE voice decoding with a specific key stream\n");
  fprintf(stderr, "                                  <value> = a 111 bytes hexadecimal string\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -DMRForceAmbeSuperFrameKeyStreamBin=<value> Force AMBE voice decoding with a specific key stream\n");
  fprintf(stderr, "                                  <value> = a 882 bits length (49 bits * 18 DMR frames) ASCII binary value\n");
  fprintf(stderr, "\n");

#ifdef BUILD_DSD_WITH_FRAME_CONTENT_DISPLAY
  fprintf(stderr, "  -DPMRPrintVoiceFrameHex       Print all dPMR voice frame in hexadecimal format\n");
  fprintf(stderr, "  -DPMRPrintVoiceFrameBin       Print all dPMR voice frame in binary format\n");
  fprintf(stderr, "  -DPMRPrintDataFrameHex        Print all dPMR data frame in hexadecimal format\n");
  fprintf(stderr, "  -DPMRPrintDataFrameHex        Print all dPMR data frame in binary format\n");
  fprintf(stderr, "  -DPMRPrintAmbeVoiceSampleHex  Print all dPMR AMBE voice sample in hexadecimal format\n");
  fprintf(stderr, "  -DPMRPrintAmbeVoiceSampleBin  Print all dPMR AMBE voice sample in binary format\n");
  fprintf(stderr, "\n");
#endif /* BUILD_DSD_WITH_FRAME_CONTENT_DISPLAY */
  fprintf(stderr, "  -DPMRForceAmbe49BitsKeyStreamHex=<value> Force AMBE voice decoding with a specific key stream\n");
  fprintf(stderr, "                                  <value> = a 7 bytes hexadecimal value\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -DPMRForceAmbe49BitsKeyStreamBin=<value> Force AMBE voice decoding with a specific key stream\n");
  fprintf(stderr, "                                  <value> = a 49 bits length ASCII binary value string\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -DPMRForceAmbeSuperFrameKeyStreamHex=<value> Force AMBE voice decoding with a specific key stream\n");
  fprintf(stderr, "                                  <value> = a 98 bytes hexadecimal string\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -DPMRForceAmbeSuperFrameKeyStreamBin=<value> Force AMBE voice decoding with a specific key stream\n");
  fprintf(stderr, "                                  <value> = a 784 bits length (49 bits * 16 voice frames) ASCII binary value\n");
#ifdef BUILD_DSD_WITH_FRAME_CONTENT_DISPLAY
  fprintf(stderr, "  -NXDNPrintAmbeVoiceSampleHex  Print all dPMR AMBE voice sample in hexadecimal format\n");
  fprintf(stderr, "\n");
#endif /* BUILD_DSD_WITH_FRAME_CONTENT_DISPLAY */
  fprintf(stderr, "  -NxdnForceAmbe49BitsKeyStreamHex=<value> Force AMBE voice decoding with a specific key stream\n");
  fprintf(stderr, "                                <value> = a 7 bytes hexadecimal value\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -NxdnForceAmbe49BitsKeyStreamBin=<value> Force AMBE voice decoding with a specific key stream\n");
  fprintf(stderr, "                                <value> = a 49 bits length ASCII binary value string\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -NxdnForceAmbeSuperFrameKeyStreamHex=<value> Force AMBE voice decoding with a specific key stream\n");
  fprintf(stderr, "                                <value> = a 98 bytes hexadecimal string\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -NxdnForceAmbeSuperFrameKeyStreamBin=<value> Force AMBE voice decoding with a specific key stream\n");
  fprintf(stderr, "                                <value> = a 784 bits length (49 bits * 16 voice frames) ASCII binary value\n");
  fprintf(stderr, "\n");
#ifdef BUILD_DSD_WITH_FRAME_CONTENT_DISPLAY
  fprintf(stderr, "  -DisplaySpecialFormat         Display AMBE frame in a special format\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -DisplayRawData               Display AMBE frame in a raw format (used to find DMR keys)\n");
  fprintf(stderr, "\n");
#endif /* BUILD_DSD_WITH_FRAME_CONTENT_DISPLAY */
  exit (0);
} /* End usage() */

void liveScanner (dsd_opts * opts, dsd_state * state)
{
#ifdef USE_PORTAUDIO
  if(opts->audio_in_type == 2)
  {
    PaError err = Pa_StartStream( opts->audio_in_pa_stream );
    if( err != paNoError )
    {
      fprintf( stderr, "An error occured while starting the portaudio input stream\n" );
      fprintf( stderr, "Error number: %d\n", err );
      fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
      return;
    }
  }
#endif
  while (1)
  {
    noCarrier (opts, state);
    state->synctype = getFrameSync (opts, state);
    // recalibrate center/umid/lmid
    state->center = ((state->max) + (state->min)) / 2;
    state->umid = (((state->max) - state->center) * 5 / 8) + state->center;
    state->lmid = (((state->min) - state->center) * 5 / 8) + state->center;
    while (state->synctype != -1)
    {
      processFrame (opts, state);

#ifdef TRACE_DSD
      state->debug_prefix = 'S';
#endif

      state->synctype = getFrameSync (opts, state);

#ifdef TRACE_DSD
      state->debug_prefix = '\0';
#endif

      // recalibrate center/umid/lmid
      state->center = ((state->max) + (state->min)) / 2;
      state->umid = (((state->max) - state->center) * 5 / 8) + state->center;
      state->lmid = (((state->min) - state->center) * 5 / 8) + state->center;
    }
  }
}

void freeAllocatedMemory(dsd_opts * opts, dsd_state * state)
{
  /* Free allocated memory */
  free(state->prev_mp_enhanced);
  free(state->prev_mp);
  free(state->cur_mp);
  free(state->audio_out_float_buf);
  free(state->audio_out_buf);
  free(state->dibit_buf);
  free(opts->mc_out_buf);
  opts->mc_out_buf = NULL;
  opts->mc_out_buf_size = 0;
}

void cleanupAndExit (dsd_opts * opts, dsd_state * state)
{
  noCarrier (opts, state);

  if (opts->wav_out_f != NULL)
  {
    closeWavOutFile (opts, state);
  }

#ifdef USE_PORTAUDIO
  if((opts->audio_in_type == 2) || (opts->audio_out_type == 2))
  {
    fprintf(stderr, "Terminating portaudio.\n");
    PaError err = paNoError;
    if(opts->audio_in_pa_stream != NULL)
    {
      err = Pa_CloseStream( opts->audio_in_pa_stream );
      if( err != paNoError )
      {
        fprintf( stderr, "An error occured while closing the portaudio input stream\n" );
        fprintf( stderr, "Error number: %d\n", err );
        fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
      }
    }
    if(opts->audio_out_pa_stream != NULL)
    {
      err = Pa_IsStreamActive( opts->audio_out_pa_stream );
      if(err == 1)
        err = Pa_StopStream( opts->audio_out_pa_stream );
      if( err != paNoError )
      {
        fprintf( stderr, "An error occured while closing the portaudio output stream\n" );
        fprintf( stderr, "Error number: %d\n", err );
        fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
      }
      err = Pa_CloseStream( opts->audio_out_pa_stream );
      if( err != paNoError )
      {
        fprintf( stderr, "An error occured while closing the portaudio output stream\n" );
        fprintf( stderr, "Error number: %d\n", err );
        fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
      }
    }
    err = Pa_Terminate();
    if( err != paNoError )
    {
      fprintf( stderr, "An error occured while terminating portaudio\n" );
      fprintf( stderr, "Error number: %d\n", err );
      fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    }
  }
#endif

  fprintf(stderr, "\n");
  fprintf(stderr, "Total audio errors: %i\n", state->debug_audio_errors);
  fprintf(stderr, "Total header errors: %i\n", state->debug_header_errors);
  fprintf(stderr, "Total irrecoverable header errors: %i\n", state->debug_header_critical_errors);

  //debug_print_heuristics(&(state->p25_heuristics));

  fprintf(stderr, "\n");
  fprintf(stderr, "+P25 BER estimate: %.2f%%\n", get_P25_BER_estimate(&state->p25_heuristics));
  fprintf(stderr, "-P25 BER estimate: %.2f%%\n", get_P25_BER_estimate(&state->inv_p25_heuristics));
  fprintf(stderr, "\n");

#ifdef TRACE_DSD
  if (state->debug_label_file != NULL) {
    fclose(state->debug_label_file);
    state->debug_label_file = NULL;
  }
  if(state->debug_label_dibit_file != NULL) {
    fclose(state->debug_label_dibit_file);
    state->debug_label_dibit_file = NULL;
  }
  if(state->debug_label_imbe_file != NULL) {
    fclose(state->debug_label_imbe_file);
    state->debug_label_imbe_file = NULL;
  }
#endif

  /* Free allocated memory */
  freeAllocatedMemory(opts, state);

  fprintf(stderr, "Exiting.\n");
  exit (0);
}

void sigfun (int sig)
{
  UNUSED_VARIABLE(sig);
  exitflag = 1;
  signal (SIGINT, SIG_DFL);
}

int main (int argc, char **argv)
{
  int c;
  extern char *optarg;
  extern int optind, opterr, optopt;
  dsd_opts opts;
  dsd_state state;
  char versionstr[25];


  int dmr_specific_keystream_valid  = 0;
  int nxdn_specific_keystream_valid = 0;
  int dpmr_specific_keystream_valid = 0;

  int NbDigit = 0;
  char TempBuffer[1024] = {0};
  char TempBuffer2[1024] = {0};
  int i, j = 0;
  int Length;
  int Result;
  FILE * RecordingFile = NULL;

  UNUSED_VARIABLE(j);
  UNUSED_VARIABLE(RecordingFile);

  mbe_printVersion (versionstr);

  fprintf(stderr, "*******************************************************************************\n");
  fprintf(stderr, "*******************************************************************************\n");
  fprintf(stderr, "*******************************************************************************\n");
  fprintf(stderr, "****  Digital Speech Decoder 1.8.4-dev (build:%s)\n", GIT_TAG);
  fprintf(stderr, "****  mbelib version %s\n", versionstr);
  fprintf(stderr, "*******************************************************************************\n");
  fprintf(stderr, "*******************************************************************************\n");
  fprintf(stderr, "*******************************************************************************\n");


  /*************************/
  /* Initialization of DSD */
  /*************************/

  initOpts (&opts);
  initState (&state);

  /* Init DMR Link Control (LC) management library */
  DmrLinkControlInitLib();

  /* Init DMR data management library */
  DmrDataContentInitLib();

  /* Init all Golay and Hamming check functions */
  InitAllFecFunction();

  exitflag = 0;
  signal (SIGINT, sigfun);

  while ((c = getopt (argc, argv, "haep:qstv:z:i:o:d:g:nw:c:B:C:R:f:m:u:x:A:S:M:rlD:N:12")) != -1)
  {
    opterr = 0;
    switch (c)
    {
      case 'h':
        usage ();
        exit (0);
      case 'a':
        printPortAudioDevices();
        exit(0);
      case 'e':
        opts.errorbars = 1;
        opts.datascope = 0;
        break;
      case 'p':
        if (optarg[0] == 'e')
        {
          opts.p25enc = 1;
        }
        else if (optarg[0] == 'l')
        {
          opts.p25lc = 1;
        }
        else if (optarg[0] == 's')
        {
          opts.p25status = 1;
        }
        else if (optarg[0] == 't')
        {
          opts.p25tg = 1;
        }
        else if (optarg[0] == 'u')
        {
          opts.unmute_encrypted_p25 = 1;
        }
        break;
      case 'q':
        opts.errorbars = 0;
        opts.verbose = 0;
        break;
      case 's':
        opts.errorbars = 0;
        opts.p25enc = 0;
        opts.p25lc = 0;
        opts.p25status = 0;
        opts.p25tg = 0;
        opts.datascope = 1;
        opts.symboltiming = 0;
        break;
      case 't':
        opts.symboltiming = 1;
        opts.errorbars = 1;
        opts.datascope = 0;
        break;
      case 'v':
        sscanf (optarg, "%d", &opts.verbose);
        break;
      case 'z':
        sscanf (optarg, "%d", &opts.scoperate);
        opts.errorbars = 0;
        opts.p25enc = 0;
        opts.p25lc = 0;
        opts.p25status = 0;
        opts.p25tg = 0;
        opts.datascope = 1;
        opts.symboltiming = 0;
        fprintf(stderr, "Setting datascope frame rate to %i frame per second.\n", opts.scoperate);
        break;
      case 'i':
        strncpy(opts.audio_in_dev, optarg, 1023);
        opts.audio_in_dev[1023] = '\0';
        break;
      case 'o':
        strncpy(opts.audio_out_dev, optarg, 1023);
        opts.audio_out_dev[1023] = '\0';
        break;
      case 'd':
        strncpy(opts.mbe_out_dir, optarg, 1023);
        opts.mbe_out_dir[1023] = '\0';
        fprintf(stderr, "Writing mbe data files to directory %s\n", opts.mbe_out_dir);
        break;
      case 'g':
        sscanf (optarg, "%f", &opts.audio_gain);
        if (opts.audio_gain < (float) 0 )
        {
          fprintf(stderr, "Disabling audio out gain setting\n");
        }
        else if (opts.audio_gain == (float) 0)
        {
          opts.audio_gain = (float) 0;
          fprintf(stderr, "Enabling audio out auto-gain\n");
        }
        else
        {
          fprintf(stderr, "Setting audio out gain to %f\n", opts.audio_gain);
          state.aout_gain = opts.audio_gain;
        }
        break;
      case 'n':
        opts.audio_out = 0;
        fprintf(stderr, "Disabling audio output to soundcard.\n");
        break;
      case 'w':
        strncpy(opts.wav_out_file, optarg, 1023);
        opts.wav_out_file[1023] = '\0';
        fprintf(stderr, "Writing audio to file %s\n", opts.wav_out_file);
        openWavOutFile (&opts, &state);
        break;
      case 'c':
        sscanf (optarg, "%d", &opts.output_channel);
        if (opts.output_channel < 1)
        {
          fprintf(stderr, "Error: output channel must be >= 1\n");
          exit(1);
        }
        fprintf(stderr, "Output audio on channel %d of multi-channel device\n", opts.output_channel);
        break;
      case 'B':
        sscanf (optarg, "%d", &opts.serial_baud);
        break;
      case 'C':
        strncpy(opts.serial_dev, optarg, 1023);
        opts.serial_dev[1023] = '\0';
        break;
      case 'R':
        sscanf (optarg, "%d", &opts.resume);
        fprintf(stderr, "Enabling scan resume after %i TDULC frames\n", opts.resume);
        break;
      case 'f':
        if (optarg[0] == 'a')
        {
          opts.frame_dstar = 1;
          opts.frame_x2tdma = 1;
          opts.frame_p25p1 = 1;
          opts.frame_nxdn48 = 0;
          opts.frame_nxdn96 = 1;
          opts.frame_dmr = 1;
          opts.frame_provoice = 0;
          opts.frame_dpmr = 0;
        }
        else if (optarg[0] == 'd')
        {
          opts.frame_dstar = 1;
          opts.frame_x2tdma = 0;
          opts.frame_p25p1 = 0;
          opts.frame_nxdn48 = 0;
          opts.frame_nxdn96 = 0;
          opts.frame_dmr = 0;
          opts.frame_provoice = 0;
          opts.frame_dpmr = 0;
          fprintf(stderr, "Decoding only D-STAR frames.\n");
        }
        else if (optarg[0] == 'x')
        {
          opts.frame_dstar = 0;
          opts.frame_x2tdma = 1;
          opts.frame_p25p1 = 0;
          opts.frame_nxdn48 = 0;
          opts.frame_nxdn96 = 0;
          opts.frame_dmr = 0;
          opts.frame_provoice = 0;
          opts.frame_dpmr = 0;
          fprintf(stderr, "Decoding only X2-TDMA frames.\n");
        }
        else if (optarg[0] == 'p')
        {
          opts.frame_dstar = 0;
          opts.frame_x2tdma = 0;
          opts.frame_p25p1 = 0;
          opts.frame_nxdn48 = 0;
          opts.frame_nxdn96 = 0;
          opts.frame_dmr = 0;
          opts.frame_provoice = 1;
          opts.frame_dpmr = 0;
          state.samplesPerSymbol = SAMPLE_PER_SYMBOL_NXDN96;
          state.symbolCenter = SAMPLE_PER_SYMBOL_NXDN96 / 2;
          opts.mod_c4fm = 0;
          opts.mod_qpsk = 0;
          opts.mod_gfsk = 1;
          state.rf_mod = GFSK_MODE;
          fprintf(stderr, "Setting symbol rate to 9600 / second\n");
          fprintf(stderr, "Enabling only GFSK modulation optimizations.\n");
          fprintf(stderr, "Decoding only ProVoice frames.\n");
        }
        else if (optarg[0] == '1')
        {
          opts.frame_dstar = 0;
          opts.frame_x2tdma = 0;
          opts.frame_p25p1 = 1;
          opts.frame_nxdn48 = 0;
          opts.frame_nxdn96 = 0;
          opts.frame_dmr = 0;
          opts.frame_provoice = 0;
          opts.frame_dpmr = 0;
          fprintf(stderr, "Decoding only P25 Phase 1 frames.\n");
        }
        else if (optarg[0] == 'i')
        {
          opts.frame_dstar = 0;
          opts.frame_x2tdma = 0;
          opts.frame_p25p1 = 0;
          opts.frame_nxdn48 = 1;
          opts.frame_nxdn96 = 0;
          opts.frame_dmr = 0;
          opts.frame_provoice = 0;
          opts.frame_dpmr = 0;
          state.samplesPerSymbol = SAMPLE_PER_SYMBOL_NXDN48;
          state.symbolCenter = 10;
          opts.mod_c4fm = 0;
          opts.mod_qpsk = 0;
          opts.mod_gfsk = 1;
          state.rf_mod = GFSK_MODE;
          fprintf(stderr, "Setting symbol rate to 2400 / second\n");
          fprintf(stderr, "Enabling only GFSK modulation optimizations.\n");
          fprintf(stderr, "Decoding only NXDN 4800 baud frames.\n");
        }
        else if (optarg[0] == 'n')
        {
          opts.frame_dstar = 0;
          opts.frame_x2tdma = 0;
          opts.frame_p25p1 = 0;
          opts.frame_nxdn48 = 0;
          opts.frame_nxdn96 = 1;
          opts.frame_dmr = 0;
          opts.frame_provoice = 0;
          opts.frame_dpmr = 0;
          opts.mod_c4fm = 0;
          opts.mod_qpsk = 0;
          opts.mod_gfsk = 1;
          state.rf_mod = GFSK_MODE;
          fprintf(stderr, "Enabling only GFSK modulation optimizations.\n");
          fprintf(stderr, "Decoding only NXDN 9600 baud frames.\n");
        }
        else if (optarg[0] == 'r')
        {
          opts.frame_dstar = 0;
          opts.frame_x2tdma = 0;
          opts.frame_p25p1 = 0;
          opts.frame_nxdn48 = 0;
          opts.frame_nxdn96 = 0;
          opts.frame_dmr = 1;
          opts.frame_provoice = 0;
          opts.frame_dpmr = 0;
          opts.mod_c4fm = 0;
          opts.mod_qpsk = 0;
          opts.mod_gfsk = 1;
          state.rf_mod = GFSK_MODE;
          fprintf(stderr, "Enabling only GFSK modulation optimizations.\n");
          fprintf(stderr, "Decoding only DMR/MOTOTRBO frames.\n");
        }
        else if (optarg[0] == 'm')
        {
          opts.frame_dstar = 0;
          opts.frame_x2tdma = 0;
          opts.frame_p25p1 = 0;
          opts.frame_nxdn48 = 0;
          opts.frame_nxdn96 = 0;
          opts.frame_dmr = 0;
          opts.frame_provoice = 0;
          opts.frame_dpmr = 1;
          state.samplesPerSymbol = SAMPLE_PER_SYMBOL_DPMR;
          state.symbolCenter = 10;
          opts.mod_c4fm = 0;
          opts.mod_qpsk = 0;
          opts.mod_gfsk = 1;
          state.rf_mod = GFSK_MODE;
          fprintf(stderr, "Setting symbol rate to 2400 / second\n");
          fprintf(stderr, "Enabling only GFSK modulation optimizations.\n");
          fprintf(stderr, "Decoding only dPMR (4800 baud frames).\n");
        }
        break;
      case 'm':
        if (optarg[0] == 'a')
        {
          opts.mod_c4fm = 1;
          opts.mod_qpsk = 1;
          opts.mod_gfsk = 1;
          state.rf_mod = C4FM_MODE;
        }
        else if (optarg[0] == 'c')
        {
          opts.mod_c4fm = 1;
          opts.mod_qpsk = 0;
          opts.mod_gfsk = 0;
          state.rf_mod = C4FM_MODE;
          fprintf(stderr, "Enabling only C4FM modulation optimizations.\n");
        }
        else if (optarg[0] == 'g')
        {
          opts.mod_c4fm = 0;
          opts.mod_qpsk = 0;
          opts.mod_gfsk = 1;
          state.rf_mod = GFSK_MODE;
          fprintf(stderr, "Enabling only GFSK modulation optimizations.\n");
        }
        else if (optarg[0] == 'q')
        {
          opts.mod_c4fm = 0;
          opts.mod_qpsk = 1;
          opts.mod_gfsk = 0;
          state.rf_mod = QPSK_MODE;
          fprintf(stderr, "Enabling only QPSK modulation optimizations.\n");
        }
        break;
      case 'u':
        sscanf (optarg, "%i", &opts.uvquality);
        if (opts.uvquality < 1)
        {
          opts.uvquality = 1;
        }
        else if (opts.uvquality > 64)
        {
          opts.uvquality = 64;
        }
        fprintf(stderr, "Setting unvoice speech quality to %i waves per band.\n", opts.uvquality);
        break;
      case 'x':
        if (optarg[0] == 'x')
        {
          opts.inverted_x2tdma = 0;
          fprintf(stderr, "Expecting non-inverted X2-TDMA signals.\n");
        }
        else if (optarg[0] == 'r')
        {
          opts.inverted_dmr = 1;
          fprintf(stderr, "Expecting inverted DMR/MOTOTRBO signals.\n");
        }
        else if (optarg[0] == 'd')
        {
          opts.inverted_dpmr = 1;
          fprintf(stderr, "Expecting inverted ICOM dPMR signals.\n");
        }
        break;
      case 'A':
        sscanf (optarg, "%i", &opts.mod_threshold);
        fprintf(stderr, "Setting C4FM/QPSK auto detection threshold to %i\n", opts.mod_threshold);
        break;
      case 'S':
        sscanf (optarg, "%i", &opts.ssize);
        if (opts.ssize > 128)
        {
          opts.ssize = 128;
        }
        else if (opts.ssize < 1)
        {
          opts.ssize = 1;
        }
        fprintf(stderr, "Setting QPSK symbol buffer to %i\n", opts.ssize);
        break;
      case 'M':
        sscanf (optarg, "%i", &opts.msize);
        if (opts.msize > 1024)
        {
          opts.msize = 1024;
        }
        else if (opts.msize < 1)
        {
          opts.msize = 1;
        }
        fprintf(stderr, "Setting QPSK Min/Max buffer to %i\n", opts.msize);
        break;
      case 'r':
        opts.playfiles = 1;
        opts.errorbars = 0;
        opts.datascope = 0;
        state.optind = optind;
        break;
      case 'l':
        opts.use_cosine_filter = 0;
        break;
      case 'D':
      {
#ifdef BUILD_DSD_WITH_FRAME_CONTENT_DISPLAY
        if(strcmp("MRPrintVoiceFrameHex",      optarg) == 0) state.printDMRRawVoiceFrameHex      = 1;
        if(strcmp("MRPrintVoiceFrameBin",      optarg) == 0) state.printDMRRawVoiceFrameBin      = 1;
        if(strcmp("MRPrintDataFrameHex",       optarg) == 0) state.printDMRRawDataFrameHex       = 1;
        if(strcmp("MRPrintDataFrameBin",       optarg) == 0) state.printDMRRawDataFrameBin       = 1;
        if(strcmp("MRPrintAmbeVoiceSampleHex", optarg) == 0) state.printDMRAmbeVoiceSampleHex    = 1;
        if(strcmp("MRPrintAmbeVoiceSampleBin", optarg) == 0) state.printDMRAmbeVoiceSampleBin    = 1;
        if(strcmp("isplaySpecialFormat",       optarg) == 0)
        {
          state.special_display_format_enable = 1;
          fprintf(stderr, "DMR AMBE frame will be displayed in \"special mode\"\n");
        }
        if(strcmp("isplayRawData",             optarg) == 0)
        {
          state.display_raw_data = 1;
          fprintf(stderr, "DMR AMBE frame will be displayed as RAW data\n");
        }
#endif /* BUILD_DSD_WITH_FRAME_CONTENT_DISPLAY */


        if(strncmp("MRForceAmbe49BitsKeyStreamHex=", optarg, 30) == 0)
        {
          dmr_specific_keystream_valid = 1;
          memset(TempBuffer, 0, sizeof(TempBuffer));
          memset(TempBuffer2, 0, sizeof(TempBuffer2));

          /* Get the specific Key in ASCII format */
          sscanf(optarg + 30, "%s", TempBuffer);
          Length = strlen(TempBuffer);

          /* An AMBE sample = 49 bit = approx 7 bytes = 14 ASCII characters */
          if(Length != 14)
          {
            /* Wrong length => Specific key is invalid */
            dmr_specific_keystream_valid = 0;
          }

          if(dmr_specific_keystream_valid)
          {
            /* A 49 bit specific key has 7 bytes length,
             * convert the ASCII format into hex format */
            for(i = 0; i < 7; i++)
            {
              Result = (int)ConvertAsciiToByte((uint8_t)TempBuffer[i * 2], (uint8_t)TempBuffer[(i * 2) + 1], (uint8_t*)&TempBuffer2[i]);

              /* Check the ASII to byte conversion status */
              if(Result != 0)
              {
                /* ASCII to byte conversion status error => Specific key stream is invalid */
                dmr_specific_keystream_valid = 0;

                /* Exit the "for" loop" */
                break;
              }
            } /* End for(i = 0; i > 7; i++) */
          } /* End if(dmr_specific_keystream_valid) */

          if(dmr_specific_keystream_valid)
          {
            Convert7BytesInto49BitSample(TempBuffer2, opts.UseSpecificDmr49BitsAmbeKeyStream);
            fprintf(stderr, "Force decoding AMBE audio sample with the following specific Key stream = %s (HEX) = ", TempBuffer);
            for(i = 0; i < 49; i++)
            {
              fprintf(stderr, "%d", opts.UseSpecificDmr49BitsAmbeKeyStream[i]);
            }
            fprintf(stderr, " (BIN)\n");

            /* Lock the current DMR Key Stream */
            opts.UseSpecificDmr49BitsAmbeKeyStreamUsed = 1;
          } /* End if(dmr_specific_keystream_valid) */
        } /* End if(strncmp("MRForceAmbe49BitsKeyStreamHex=", optarg, 30) == 0) */


        if(strncmp("MRForceAmbe49BitsKeyStreamBin=", optarg, 30) == 0)
        {
          dmr_specific_keystream_valid = 1;
          memset(TempBuffer, 0, sizeof(TempBuffer));
          memset(TempBuffer2, 0, sizeof(TempBuffer2));

          /* Get the specific Key in ASCII format */
          sscanf(optarg + 30, "%s", TempBuffer);
          Length = strlen(TempBuffer);

          /* An AMBE sample = 49 bit */
          if(Length != 49)
          {
            /* Wrong length => Specific key is invalid */
            dmr_specific_keystream_valid = 0;

            fprintf(stderr, "The specific key stream length must be 49 bits long instead of %d", Length);
          }

          if(dmr_specific_keystream_valid)
          {
            for(i = 0; i < 49; i++)
            {
              if((TempBuffer[i] == '0') || (TempBuffer[i] == '1'))
              {
                opts.UseSpecificDmr49BitsAmbeKeyStream[i] = (TempBuffer[i] - '0') & 1;
              }
              else
              {
                /* Conversion status error => Specific key stream is invalid */
                dmr_specific_keystream_valid = 0;

                fprintf(stderr, "DMRForceAmbe49BitsKeyStreamBin - Invalid byte detected : 0x%02X\n", TempBuffer[i]);

                /* Exit the "for" loop" */
                break;
              }
            }
          } /* End if(dmr_specific_keystream_valid) */

          if(dmr_specific_keystream_valid)
          {
            Convert49BitSampleInto7Bytes(opts.UseSpecificDmr49BitsAmbeKeyStream, TempBuffer2);
            fprintf(stderr, "Force decoding AMBE audio sample with the following specific Key stream = ");

            for(i = 0; i < 7; i++)
            {
              fprintf(stderr, "%02X", TempBuffer2[i] & 0xFF);
            }
            fprintf(stderr, " (HEX) = ");
            for(i = 0; i < 49; i++)
            {
              fprintf(stderr, "%d", opts.UseSpecificDmr49BitsAmbeKeyStream[i]);
            }
            fprintf(stderr, " (BIN)\n");

            /* Lock the current DMR Key Stream */
            opts.UseSpecificDmr49BitsAmbeKeyStreamUsed = 1;
          } /* End if(dmr_specific_keystream_valid) */
        } /* End if(strncmp("MRForceAmbe49BitsKeyStreamBin=", optarg, 30) == 0) */


        if(strncmp("MRForceAmbeSuperFrameKeyStreamHex=", optarg, 34) == 0)
        {
          dmr_specific_keystream_valid = 1;
          memset(TempBuffer, 0, sizeof(TempBuffer));
          memset(TempBuffer2, 0, sizeof(TempBuffer2));

          /* Get the specific Key in ASCII format */
          sscanf(optarg + 34, "%s", TempBuffer);
          Length = strlen(TempBuffer);

          /* An entire AMBE DMR superframe = 49 bits x 18 frames = 882 bits = 110.25 bytes = approx 111 bytes = 222 ASCII characters */
          if(Length != 222)
          {
            /* Wrong length => Specific key is invalid */
            dmr_specific_keystream_valid = 0;

            fprintf(stderr, "The specific key stream length must be 222 ASCII HEX characters long instead of %d", Length);
          }

          if(dmr_specific_keystream_valid)
          {
            /* Convert the ASCII format into hex format */
            for(i = 0; i < 111; i++)
            {
              Result = (int)ConvertAsciiToByte((uint8_t)TempBuffer[i * 2], (uint8_t)TempBuffer[(i * 2) + 1], (uint8_t*)&TempBuffer2[i]);

              /* Check the ASII to byte conversion status */
              if(Result != 0)
              {
                /* ASCII to byte conversion status error => Spacific key stream is invalid */
                dmr_specific_keystream_valid = 0;

                //fprintf(stderr, "TempBuffer[i * 2]       = TempBuffer[%d * 2]       = TempBuffer[%d] = 0x%02X\n", i, i * 2, TempBuffer[i * 2]);
                //fprintf(stderr, "TempBuffer[(i * 2) + 1] = TempBuffer[(%d * 2) + 1] = TempBuffer[%d] = 0x%02X\n", i, (i * 2) + 1, TempBuffer[(i * 2) + 1]);

                fprintf(stderr, "DMRForceAmbeSuperFrameKeyStreamHex - Invalid byte detected at index %d : 0x%02X\n", i, TempBuffer2[i]);

                /* Exit the "for" loop" */
                break;
              }

              /* First 880 bits are complete byte */
              if(i < 110)
              {
                opts.UseSpecificDmrAmbeSuperFrameKeyStream[(i * 8) + 0] = (TempBuffer2[i] >> 7) & 1;
                opts.UseSpecificDmrAmbeSuperFrameKeyStream[(i * 8) + 1] = (TempBuffer2[i] >> 6) & 1;
                opts.UseSpecificDmrAmbeSuperFrameKeyStream[(i * 8) + 2] = (TempBuffer2[i] >> 5) & 1;
                opts.UseSpecificDmrAmbeSuperFrameKeyStream[(i * 8) + 3] = (TempBuffer2[i] >> 4) & 1;
                opts.UseSpecificDmrAmbeSuperFrameKeyStream[(i * 8) + 4] = (TempBuffer2[i] >> 3) & 1;
                opts.UseSpecificDmrAmbeSuperFrameKeyStream[(i * 8) + 5] = (TempBuffer2[i] >> 2) & 1;
                opts.UseSpecificDmrAmbeSuperFrameKeyStream[(i * 8) + 6] = (TempBuffer2[i] >> 1) & 1;
                opts.UseSpecificDmrAmbeSuperFrameKeyStream[(i * 8) + 7] = (TempBuffer2[i] >> 0) & 1;
              }
              else /* Last 2 bits are incomplete byte */
              {
                opts.UseSpecificDmrAmbeSuperFrameKeyStream[(i * 8) + 0] = (TempBuffer2[i] >> 7) & 1;
                opts.UseSpecificDmrAmbeSuperFrameKeyStream[(i * 8) + 1] = (TempBuffer2[i] >> 6) & 1;
              }

            } /* End for(i = 0; i < 111; i++) */
          } /* End if(dmr_specific_keystream_valid) */

          if(dmr_specific_keystream_valid)
          {
            for(i = 0; i < 111; i++)
            {
              if(i < (int)sizeof(opts.UseSpecificDmrAmbeSuperFrameKeyStream))
              {
                TempBuffer2[i] = (char)ConvertBitIntoBytes((uint8_t *)&opts.UseSpecificDmrAmbeSuperFrameKeyStream[i * 8], 8) & 0xFF;
              }
              else
              {
                TempBuffer2[i] = 0;
              }
            }

            fprintf(stderr, "Force decoding AMBE audio sample with the following specific Key stream = ");

            for(i = 0; i < 111; i++)
            {
              fprintf(stderr, "%02X", TempBuffer2[i] & 0xFF);
            }
            fprintf(stderr, " (HEX) = ");
            for(i = 0; i < (49 * 18); i++)
            {
              fprintf(stderr, "%d", opts.UseSpecificDmrAmbeSuperFrameKeyStream[i]);
            }
            fprintf(stderr, " (BIN)\n");

            /* Lock the current DMR Key Stream */
            opts.UseSpecificDmrAmbeSuperFrameKeyStreamUsed = 1;
          } /* End if(dmr_specific_keystream_valid) */
        } /* End if(strncmp("MRForceAmbe49BitsKeyStreamHex=", optarg, 30) == 0) */


        if(strncmp("MRForceAmbeSuperFrameKeyStreamBin=", optarg, 34) == 0)
        {
          dmr_specific_keystream_valid = 1;
          memset(TempBuffer, 0, sizeof(TempBuffer));
          memset(TempBuffer2, 0, sizeof(TempBuffer2));

          /* Get the specific Key in ASCII format */
          sscanf(optarg + 34, "%s", TempBuffer);
          Length = strlen(TempBuffer);

          /* An entire AMBE DMR superframe = 49 bits x 18 frames = 882 bits */
          if(Length != (49 * 18))
          {
            /* Wrong length => Specific key is invalid */
            dmr_specific_keystream_valid = 0;

            fprintf(stderr, "The specific key stream length must be 882 bits long instead of %d", Length);
          }

          if(dmr_specific_keystream_valid)
          {
            for(i = 0; i < (49 * 18); i++)
            {
              if((TempBuffer[i] == '0') || (TempBuffer[i] == '1'))
              {
                opts.UseSpecificDmrAmbeSuperFrameKeyStream[i] = (TempBuffer[i] - '0') & 1;
              }
              else
              {
                /* Conversion status error => Specific key stream is invalid */
                dmr_specific_keystream_valid = 0;

                fprintf(stderr, "DMRForceAmbeSuperFrameKeyStreamBin - Invalid byte detected : 0x%02X\n", TempBuffer[i]);

                /* Exit the "for" loop" */
                break;
              }
            }
          } /* End if(dmr_specific_keystream_valid) */

          if(dmr_specific_keystream_valid)
          {
            for(i = 0; i < 111; i++)
            {
              if(i < (int)sizeof(opts.UseSpecificDmrAmbeSuperFrameKeyStream))
              {
                TempBuffer2[i] = (char)ConvertBitIntoBytes((uint8_t *)&opts.UseSpecificDmrAmbeSuperFrameKeyStream[i * 8], 8) & 0xFF;
              }
              else
              {
                TempBuffer2[i] = 0;
              }
            }

            fprintf(stderr, "Force decoding AMBE audio sample with the following specific Key stream = ");

            for(i = 0; i < 111; i++)
            {
              fprintf(stderr, "%02X", TempBuffer2[i] & 0xFF);
            }
            fprintf(stderr, " (HEX) = ");
            for(i = 0; i < (49 * 18); i++)
            {
              fprintf(stderr, "%d", opts.UseSpecificDmrAmbeSuperFrameKeyStream[i]);
            }
            fprintf(stderr, " (BIN)\n");

            /* Lock the current DMR Key Stream */
            opts.UseSpecificDmrAmbeSuperFrameKeyStreamUsed = 1;
          } /* End if(dmr_specific_keystream_valid) */
        } /* End if(strncmp("MRForceAmbe49BitsKeyStreamBin=", optarg, 30) == 0) */


#ifdef BUILD_DSD_WITH_FRAME_CONTENT_DISPLAY
        if(strcmp("PMRPrintVoiceFrameHex",      optarg) == 0) state.printdPMRRawVoiceFrameHex    = 1;
        if(strcmp("PMRPrintVoiceFrameBin",      optarg) == 0) state.printdPMRRawVoiceFrameBin    = 1;
        if(strcmp("PMRPrintDataFrameHex",       optarg) == 0) state.printdPMRRawDataFrameHex     = 1;
        if(strcmp("PMRPrintDataFrameBin",       optarg) == 0) state.printdPMRRawDataFrameBin     = 1;
        if(strcmp("PMRPrintAmbeVoiceSampleHex", optarg) == 0) state.printdPMRAmbeVoiceSampleHex  = 1;
        if(strcmp("PMRPrintAmbeVoiceSampleBin", optarg) == 0) state.printdPMRAmbeVoiceSampleBin  = 1;
#endif /* BUILD_DSD_WITH_FRAME_CONTENT_DISPLAY */


        if(strncmp("PMRForceAmbe49BitsKeyStreamHex=", optarg, 31) == 0)
        {
          dpmr_specific_keystream_valid = 1;
          memset(TempBuffer, 0, sizeof(TempBuffer));
          memset(TempBuffer2, 0, sizeof(TempBuffer2));

          /* Get the specific Key in ASCII format */
          sscanf(optarg + 31, "%s", TempBuffer);
          Length = strlen(TempBuffer);

          /* An AMBE sample = 49 bit = approx 7 bytes = 14 ASCII characters */
          if(Length != 14)
          {
            /* Wrong length => Specific key is invalid */
            dpmr_specific_keystream_valid = 0;
          }

          if(dpmr_specific_keystream_valid)
          {
            /* A 49 bit specific key has 7 bytes length,
             * convert the ASCII format into hex format */
            for(i = 0; i < 7; i++)
            {
              Result = (int)ConvertAsciiToByte((uint8_t)TempBuffer[i * 2], (uint8_t)TempBuffer[(i * 2) + 1], (uint8_t*)&TempBuffer2[i]);

              /* Check the ASII to byte conversion status */
              if(Result != 0)
              {
                /* ASCII to byte conversion status error => Specific key stream is invalid */
                dpmr_specific_keystream_valid = 0;

                /* Exit the "for" loop" */
                break;
              }
            } /* End for(i = 0; i > 7; i++) */
          } /* End if(dpmr_specific_keystream_valid) */

          if(dpmr_specific_keystream_valid)
          {
            Convert7BytesInto49BitSample(TempBuffer2, opts.UseSpecificdPMR49BitsAmbeKeyStream);
            fprintf(stderr, "Force decoding AMBE audio sample with the following specific Key stream = %s (HEX) = ", TempBuffer);
            for(i = 0; i < 49; i++)
            {
              fprintf(stderr, "%d", opts.UseSpecificdPMR49BitsAmbeKeyStream[i]);
            }
            fprintf(stderr, " (BIN)\n");

            /* Lock the current dPMR Key Stream */
            opts.UseSpecificdPMR49BitsAmbeKeyStreamUsed = 1;
          } /* End if(dpmr_specific_keystream_valid) */
        } /* End if(strncmp("PMRForceAmbe49BitsKeyStreamHex=", optarg, 31) == 0) */


        if(strncmp("XDNForceAmbe49BitsKeyStreamBin=", optarg, 31) == 0)
        {
          dpmr_specific_keystream_valid = 1;
          memset(TempBuffer, 0, sizeof(TempBuffer));
          memset(TempBuffer2, 0, sizeof(TempBuffer2));

          /* Get the specific Key in ASCII format */
          sscanf(optarg + 31, "%s", TempBuffer);
          Length = strlen(TempBuffer);

          /* An AMBE sample = 49 bit */
          if(Length != 49)
          {
            /* Wrong length => Specific key is invalid */
            dpmr_specific_keystream_valid = 0;

            fprintf(stderr, "The specific key stream length must be 49 bits long instead of %d", Length);
          }

          if(dpmr_specific_keystream_valid)
          {
            for(i = 0; i < 49; i++)
            {
              if((TempBuffer[i] == '0') || (TempBuffer[i] == '1'))
              {
                opts.UseSpecificdPMR49BitsAmbeKeyStream[i] = (TempBuffer[i] - '0') & 1;
              }
              else
              {
                /* Conversion status error => Specific key stream is invalid */
                dpmr_specific_keystream_valid = 0;

                fprintf(stderr, "dPMRForceAmbe49BitsKeyStreamBin - Invalid byte detected : 0x%02X\n", TempBuffer[i]);

                /* Exit the "for" loop" */
                break;
              }
            }
          } /* End if(dpmr_specific_keystream_valid) */

          if(dpmr_specific_keystream_valid)
          {
            Convert49BitSampleInto7Bytes(opts.UseSpecificdPMR49BitsAmbeKeyStream, TempBuffer2);
            fprintf(stderr, "Force decoding AMBE audio sample with the following specific Key stream = ");

            for(i = 0; i < 7; i++)
            {
              fprintf(stderr, "%02X", TempBuffer2[i] & 0xFF);
            }
            fprintf(stderr, " (HEX) = ");
            for(i = 0; i < 49; i++)
            {
              fprintf(stderr, "%d", opts.UseSpecificdPMR49BitsAmbeKeyStream[i]);
            }
            fprintf(stderr, " (BIN)\n");

            /* Lock the current dPMR Key Stream */
            opts.UseSpecificdPMR49BitsAmbeKeyStreamUsed = 1;
          } /* End if(dpmr_specific_keystream_valid) */
        } /* End if(strncmp("PMRForceAmbe49BitsKeyStreamBin=", optarg, 31) == 0) */


        if(strncmp("PMRForceAmbeSuperFrameKeyStreamHex=", optarg, 35) == 0)
        {
          dpmr_specific_keystream_valid = 1;
          memset(TempBuffer, 0, sizeof(TempBuffer));
          memset(TempBuffer2, 0, sizeof(TempBuffer2));

          /* Get the specific Key in ASCII format */
          sscanf(optarg + 35, "%s", TempBuffer);
          Length = strlen(TempBuffer);

          /* An entire AMBE dPMR voice superframe = 49 bits x 16 frames = 784 bits = 98 bytes = 196 ASCII characters */
          if(Length != 196)
          {
            /* Wrong length => Specific key is invalid */
            dpmr_specific_keystream_valid = 0;

            fprintf(stderr, "The specific key stream length must be 196 ASCII HEX characters long instead of %d", Length);
          }

          if(dpmr_specific_keystream_valid)
          {
            /* Convert the ASCII format into hex format */
            for(i = 0; i < 98; i++)
            {
              Result = (int)ConvertAsciiToByte((uint8_t)TempBuffer[i * 2], (uint8_t)TempBuffer[(i * 2) + 1], (uint8_t*)&TempBuffer2[i]);

              /* Check the ASII to byte conversion status */
              if(Result != 0)
              {
                /* ASCII to byte conversion status error => Spacific key stream is invalid */
                dpmr_specific_keystream_valid = 0;

                fprintf(stderr, "dPMRForceAmbeSuperFrameKeyStreamHex - Invalid byte detected at index %d : 0x%02X\n", i, TempBuffer2[i]);

                /* Exit the "for" loop" */
                break;
              }

              opts.UseSpecificdPMRAmbeSuperFrameKeyStream[(i * 8) + 0] = (TempBuffer2[i] >> 7) & 1;
              opts.UseSpecificdPMRAmbeSuperFrameKeyStream[(i * 8) + 1] = (TempBuffer2[i] >> 6) & 1;
              opts.UseSpecificdPMRAmbeSuperFrameKeyStream[(i * 8) + 2] = (TempBuffer2[i] >> 5) & 1;
              opts.UseSpecificdPMRAmbeSuperFrameKeyStream[(i * 8) + 3] = (TempBuffer2[i] >> 4) & 1;
              opts.UseSpecificdPMRAmbeSuperFrameKeyStream[(i * 8) + 4] = (TempBuffer2[i] >> 3) & 1;
              opts.UseSpecificdPMRAmbeSuperFrameKeyStream[(i * 8) + 5] = (TempBuffer2[i] >> 2) & 1;
              opts.UseSpecificdPMRAmbeSuperFrameKeyStream[(i * 8) + 6] = (TempBuffer2[i] >> 1) & 1;
              opts.UseSpecificdPMRAmbeSuperFrameKeyStream[(i * 8) + 7] = (TempBuffer2[i] >> 0) & 1;

            } /* End for(i = 0; i < 98; i++) */
          } /* End if(dpmr_specific_keystream_valid) */

          if(dpmr_specific_keystream_valid)
          {
            for(i = 0; i < 98; i++)
            {
              if(i < (int)sizeof(opts.UseSpecificdPMRAmbeSuperFrameKeyStream))
              {
                TempBuffer2[i] = (char)ConvertBitIntoBytes((uint8_t *)&opts.UseSpecificdPMRAmbeSuperFrameKeyStream[i * 8], 8) & 0xFF;
              }
              else
              {
                TempBuffer2[i] = 0;
              }
            }

            fprintf(stderr, "Force decoding AMBE audio sample with the following specific Key stream = ");

            for(i = 0; i < 98; i++)
            {
              fprintf(stderr, "%02X", TempBuffer2[i] & 0xFF);
            }
            fprintf(stderr, " (HEX) = ");
            for(i = 0; i < (49 * 16); i++)
            {
              fprintf(stderr, "%d", opts.UseSpecificdPMRAmbeSuperFrameKeyStream[i]);
            }
            fprintf(stderr, " (BIN)\n");

            /* Lock the current dPMR Key Stream */
            opts.UseSpecificdPMRAmbeSuperFrameKeyStreamUsed = 1;
          } /* End if(dpmr_specific_keystream_valid) */
        } /* End if(strncmp("PMRForceAmbe49BitsKeyStreamHex=", optarg, 35) == 0) */


        if(strncmp("PMRForceAmbeSuperFrameKeyStreamBin=", optarg, 35) == 0)
        {
          dpmr_specific_keystream_valid = 1;
          memset(TempBuffer, 0, sizeof(TempBuffer));
          memset(TempBuffer2, 0, sizeof(TempBuffer2));

          /* Get the specific Key in ASCII format */
          sscanf(optarg + 35, "%s", TempBuffer);
          Length = strlen(TempBuffer);

          /* An entire AMBE dPMR superframe = 49 bits x 16 frames = 784 bits */
          if(Length != (49 * 16))
          {
            /* Wrong length => Specific key is invalid */
            dpmr_specific_keystream_valid = 0;

            fprintf(stderr, "The specific key stream length must be 784 bits long instead of %d", Length);
          }

          if(dpmr_specific_keystream_valid)
          {
            for(i = 0; i < (49 * 16); i++)
            {
              if((TempBuffer[i] == '0') || (TempBuffer[i] == '1'))
              {
                opts.UseSpecificdPMRAmbeSuperFrameKeyStream[i] = (TempBuffer[i] - '0') & 1;
              }
              else
              {
                /* Conversion status error => Specific key stream is invalid */
                dpmr_specific_keystream_valid = 0;

                fprintf(stderr, "DPMRForceAmbeSuperFrameKeyStreamBin - Invalid byte detected : 0x%02X\n", TempBuffer[i]);

                /* Exit the "for" loop" */
                break;
              }
            }
          } /* End if(dpmr_specific_keystream_valid) */

          if(dpmr_specific_keystream_valid)
          {
            for(i = 0; i < 98; i++)
            {
              if(i < (int)sizeof(opts.UseSpecificdPMRAmbeSuperFrameKeyStream))
              {
                TempBuffer2[i] = (char)ConvertBitIntoBytes((uint8_t *)&opts.UseSpecificdPMRAmbeSuperFrameKeyStream[i * 8], 8) & 0xFF;
              }
              else
              {
                TempBuffer2[i] = 0;
              }
            }

            fprintf(stderr, "Force decoding AMBE audio sample with the following specific Key stream = ");

            for(i = 0; i < 98; i++)
            {
              fprintf(stderr, "%02X", TempBuffer2[i] & 0xFF);
            }
            fprintf(stderr, " (HEX) = ");
            for(i = 0; i < (49 * 16); i++)
            {
              fprintf(stderr, "%d", opts.UseSpecificdPMRAmbeSuperFrameKeyStream[i]);
            }
            fprintf(stderr, " (BIN)\n");

            /* Lock the current dPMR Key Stream */
            opts.UseSpecificdPMRAmbeSuperFrameKeyStreamUsed = 1;
          } /* End if(dpmr_specific_keystream_valid) */
        } /* End if(strncmp("PMRForceAmbe49BitsKeyStreamBin=", optarg, 35) == 0) */

        break;
      } /* End case 'D' */

      case 'N':
      {
#ifdef BUILD_DSD_WITH_FRAME_CONTENT_DISPLAY
        if(strcmp("XDNPrintAmbeVoiceSampleHex", optarg) == 0) state.printNXDNAmbeVoiceSampleHex  = 1;
#endif /* BUILD_DSD_WITH_FRAME_CONTENT_DISPLAY */

        if(strncmp("xdnForceAmbe49BitsKeyStreamHex=", optarg, 31) == 0)
        {
          nxdn_specific_keystream_valid = 1;
          memset(TempBuffer, 0, sizeof(TempBuffer));
          memset(TempBuffer2, 0, sizeof(TempBuffer2));

          /* Get the specific Key in ASCII format */
          sscanf(optarg + 31, "%s", TempBuffer);
          Length = strlen(TempBuffer);

          /* An AMBE sample = 49 bit = approx 7 bytes = 14 ASCII characters */
          if(Length != 14)
          {
            /* Wrong length => Specific key is invalid */
            nxdn_specific_keystream_valid = 0;
          }

          if(nxdn_specific_keystream_valid)
          {
            /* A 49 bit specific key has 7 bytes length,
             * convert the ASCII format into hex format */
            for(i = 0; i < 7; i++)
            {
              Result = (int)ConvertAsciiToByte((uint8_t)TempBuffer[i * 2], (uint8_t)TempBuffer[(i * 2) + 1], (uint8_t*)&TempBuffer2[i]);

              /* Check the ASII to byte conversion status */
              if(Result != 0)
              {
                /* ASCII to byte conversion status error => Specific key stream is invalid */
                nxdn_specific_keystream_valid = 0;

                /* Exit the "for" loop" */
                break;
              }
            } /* End for(i = 0; i > 7; i++) */
          } /* End if(nxdn_specific_keystream_valid) */

          if(nxdn_specific_keystream_valid)
          {
            Convert7BytesInto49BitSample(TempBuffer2, opts.UseSpecificNxdn49BitsAmbeKeyStream);
            fprintf(stderr, "Force decoding AMBE audio sample with the following specific Key stream = %s (HEX) = ", TempBuffer);
            for(i = 0; i < 49; i++)
            {
              fprintf(stderr, "%d", opts.UseSpecificNxdn49BitsAmbeKeyStream[i]);
            }
            fprintf(stderr, " (BIN)\n");

            /* Lock the current NXDN Key Stream */
            opts.UseSpecificNxdn49BitsAmbeKeyStreamUsed = 1;
          } /* End if(nxdn_specific_keystream_valid) */
        } /* End if(strncmp("xdnForceAmbe49BitsKeyStreamHex=", optarg, 31) == 0) */


        if(strncmp("XDNForceAmbe49BitsKeyStreamBin=", optarg, 31) == 0)
        {
          nxdn_specific_keystream_valid = 1;
          memset(TempBuffer, 0, sizeof(TempBuffer));
          memset(TempBuffer2, 0, sizeof(TempBuffer2));

          /* Get the specific Key in ASCII format */
          sscanf(optarg + 31, "%s", TempBuffer);
          Length = strlen(TempBuffer);

          /* An AMBE sample = 49 bit */
          if(Length != 49)
          {
            /* Wrong length => Specific key is invalid */
            nxdn_specific_keystream_valid = 0;

            fprintf(stderr, "The specific key stream length must be 49 bits long instead of %d", Length);
          }

          if(nxdn_specific_keystream_valid)
          {
            for(i = 0; i < 49; i++)
            {
              if((TempBuffer[i] == '0') || (TempBuffer[i] == '1'))
              {
                opts.UseSpecificNxdn49BitsAmbeKeyStream[i] = (TempBuffer[i] - '0') & 1;
              }
              else
              {
                /* Conversion status error => Specific key stream is invalid */
                nxdn_specific_keystream_valid = 0;

                fprintf(stderr, "NxdnForceAmbe49BitsKeyStreamBin - Invalid byte detected : 0x%02X\n", TempBuffer[i]);

                /* Exit the "for" loop" */
                break;
              }
            }
          } /* End if(nxdn_specific_keystream_valid) */

          if(nxdn_specific_keystream_valid)
          {
            Convert49BitSampleInto7Bytes(opts.UseSpecificNxdn49BitsAmbeKeyStream, TempBuffer2);
            fprintf(stderr, "Force decoding AMBE audio sample with the following specific Key stream = ");

            for(i = 0; i < 7; i++)
            {
              fprintf(stderr, "%02X", TempBuffer2[i] & 0xFF);
            }
            fprintf(stderr, " (HEX) = ");
            for(i = 0; i < 49; i++)
            {
              fprintf(stderr, "%d", opts.UseSpecificNxdn49BitsAmbeKeyStream[i]);
            }
            fprintf(stderr, " (BIN)\n");

            /* Lock the current NXDN Key Stream */
            opts.UseSpecificNxdn49BitsAmbeKeyStreamUsed = 1;
          } /* End if(nxdn_specific_keystream_valid) */
        } /* End if(strncmp("xdnForceAmbe49BitsKeyStreamBin=", optarg, 31) == 0) */

        if(strncmp("xdnForceAmbeSuperFrameKeyStreamHex=", optarg, 35) == 0)
        {
          nxdn_specific_keystream_valid = 1;
          memset(TempBuffer, 0, sizeof(TempBuffer));
          memset(TempBuffer2, 0, sizeof(TempBuffer2));

          /* Get the specific Key in ASCII format */
          sscanf(optarg + 35, "%s", TempBuffer);
          Length = strlen(TempBuffer);

          /* An entire AMBE NXDN voice superframe = 49 bits x 16 frames = 784 bits = 98 bytes = 196 ASCII characters */
          if(Length != 196)
          {
            /* Wrong length => Specific key is invalid */
            nxdn_specific_keystream_valid = 0;

            fprintf(stderr, "The specific key stream length must be 196 ASCII HEX characters long instead of %d", Length);
          }

          if(nxdn_specific_keystream_valid)
          {
            /* Convert the ASCII format into hex format */
            for(i = 0; i < 98; i++)
            {
              Result = (int)ConvertAsciiToByte((uint8_t)TempBuffer[i * 2], (uint8_t)TempBuffer[(i * 2) + 1], (uint8_t*)&TempBuffer2[i]);

              /* Check the ASII to byte conversion status */
              if(Result != 0)
              {
                /* ASCII to byte conversion status error => Spacific key stream is invalid */
                nxdn_specific_keystream_valid = 0;

                fprintf(stderr, "NxdnForceAmbeSuperFrameKeyStreamHex - Invalid byte detected at index %d : 0x%02X\n", i, TempBuffer2[i]);

                /* Exit the "for" loop" */
                break;
              }

              opts.UseSpecificNxdnAmbeSuperFrameKeyStream[(i * 8) + 0] = (TempBuffer2[i] >> 7) & 1;
              opts.UseSpecificNxdnAmbeSuperFrameKeyStream[(i * 8) + 1] = (TempBuffer2[i] >> 6) & 1;
              opts.UseSpecificNxdnAmbeSuperFrameKeyStream[(i * 8) + 2] = (TempBuffer2[i] >> 5) & 1;
              opts.UseSpecificNxdnAmbeSuperFrameKeyStream[(i * 8) + 3] = (TempBuffer2[i] >> 4) & 1;
              opts.UseSpecificNxdnAmbeSuperFrameKeyStream[(i * 8) + 4] = (TempBuffer2[i] >> 3) & 1;
              opts.UseSpecificNxdnAmbeSuperFrameKeyStream[(i * 8) + 5] = (TempBuffer2[i] >> 2) & 1;
              opts.UseSpecificNxdnAmbeSuperFrameKeyStream[(i * 8) + 6] = (TempBuffer2[i] >> 1) & 1;
              opts.UseSpecificNxdnAmbeSuperFrameKeyStream[(i * 8) + 7] = (TempBuffer2[i] >> 0) & 1;

            } /* End for(i = 0; i < 98; i++) */
          } /* End if(nxdn_specific_keystream_valid) */

          if(nxdn_specific_keystream_valid)
          {
            for(i = 0; i < 98; i++)
            {
              if(i < (int)sizeof(opts.UseSpecificNxdnAmbeSuperFrameKeyStream))
              {
                TempBuffer2[i] = (char)ConvertBitIntoBytes((uint8_t *)&opts.UseSpecificNxdnAmbeSuperFrameKeyStream[i * 8], 8) & 0xFF;
              }
              else
              {
                TempBuffer2[i] = 0;
              }
            }

            fprintf(stderr, "Force decoding AMBE audio sample with the following specific Key stream = ");

            for(i = 0; i < 98; i++)
            {
              fprintf(stderr, "%02X", TempBuffer2[i] & 0xFF);
            }
            fprintf(stderr, " (HEX) = ");
            for(i = 0; i < (49 * 16); i++)
            {
              fprintf(stderr, "%d", opts.UseSpecificNxdnAmbeSuperFrameKeyStream[i]);
            }
            fprintf(stderr, " (BIN)\n");

            /* Lock the current NXDN Key Stream */
            opts.UseSpecificNxdnAmbeSuperFrameKeyStreamUsed = 1;
          } /* End if(nxdn_specific_keystream_valid) */
        } /* End if(strncmp("xdnForceAmbe49BitsKeyStreamHex=", optarg, 35) == 0) */


        if(strncmp("xdnForceAmbeSuperFrameKeyStreamBin=", optarg, 35) == 0)
        {
          nxdn_specific_keystream_valid = 1;
          memset(TempBuffer, 0, sizeof(TempBuffer));
          memset(TempBuffer2, 0, sizeof(TempBuffer2));

          /* Get the specific Key in ASCII format */
          sscanf(optarg + 35, "%s", TempBuffer);
          Length = strlen(TempBuffer);

          /* An entire AMBE NXDN superframe = 49 bits x 16 frames = 784 bits */
          if(Length != (49 * 16))
          {
            /* Wrong length => Specific key is invalid */
            nxdn_specific_keystream_valid = 0;

            fprintf(stderr, "The specific key stream length must be 784 bits long instead of %d", Length);
          }

          if(nxdn_specific_keystream_valid)
          {
            for(i = 0; i < (49 * 16); i++)
            {
              if((TempBuffer[i] == '0') || (TempBuffer[i] == '1'))
              {
                opts.UseSpecificNxdnAmbeSuperFrameKeyStream[i] = (TempBuffer[i] - '0') & 1;
              }
              else
              {
                /* Conversion status error => Specific key stream is invalid */
                nxdn_specific_keystream_valid = 0;

                fprintf(stderr, "NxdnForceAmbeSuperFrameKeyStreamBin - Invalid byte detected : 0x%02X\n", TempBuffer[i]);

                /* Exit the "for" loop" */
                break;
              }
            }
          } /* End if(nxdn_specific_keystream_valid) */

          if(nxdn_specific_keystream_valid)
          {
            for(i = 0; i < 98; i++)
            {
              if(i < (int)sizeof(opts.UseSpecificNxdnAmbeSuperFrameKeyStream))
              {
                TempBuffer2[i] = (char)ConvertBitIntoBytes((uint8_t *)&opts.UseSpecificNxdnAmbeSuperFrameKeyStream[i * 8], 8) & 0xFF;
              }
              else
              {
                TempBuffer2[i] = 0;
              }
            }

            fprintf(stderr, "Force decoding AMBE audio sample with the following specific Key stream = ");

            for(i = 0; i < 98; i++)
            {
              fprintf(stderr, "%02X", TempBuffer2[i] & 0xFF);
            }
            fprintf(stderr, " (HEX) = ");
            for(i = 0; i < (49 * 16); i++)
            {
              fprintf(stderr, "%d", opts.UseSpecificNxdnAmbeSuperFrameKeyStream[i]);
            }
            fprintf(stderr, " (BIN)\n");

            /* Lock the current NXDN Key Stream */
            opts.UseSpecificNxdnAmbeSuperFrameKeyStreamUsed = 1;
          } /* End if(nxdn_specific_keystream_valid) */
        } /* End if(strncmp("xdnForceAmbe49BitsKeyStreamBin=", optarg, 35) == 0) */

        break;
      } /* End case 'N' */

      case '1':
      {
        opts.SlotToOnlyDecode = 1;
        fprintf(stderr, "Synthesize audio for first DMR timeslot\n");
        break;
      }
      case '2':
      {
        opts.SlotToOnlyDecode = 2;
        fprintf(stderr, "Synthesize audio for second DMR timeslot\n");
        break;
      }
      default:
        usage ();
        exit (0);
    }
  } /* End while ((c = getopt (argc, argv, "haep:qstv:z:i:o:d:g:nw:B:C:R:f:m:u:x:A:S:M:rlD:N:")) != -1) */


  if (opts.resume > 0)
  {
    openSerial (&opts, &state);
  }


#ifdef USE_PORTAUDIO
  if((strncmp(opts.audio_in_dev, "pa:", 3) == 0)
      || (strncmp(opts.audio_out_dev, "pa:", 3) == 0))
  {
    fprintf(stderr, "Initializing portaudio.\n");
    PaError err = Pa_Initialize();
    if( err != paNoError )
    {
      fprintf( stderr, "An error occured while initializing portaudio\n" );
      fprintf( stderr, "Error number: %d\n", err );
      fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
      exit(err);
    }
  }
#endif

  if (opts.playfiles == 1)
  {
    opts.split = 1;
    opts.playoffset = 0;
    opts.delay = 0;
    if (strlen(opts.wav_out_file) > 0)
    {
      openWavOutFile (&opts, &state);
    }
    else
    {
      openAudioOutDevice (&opts, SAMPLE_RATE_OUT);
    }
  }
  else if ((strcmp (opts.audio_in_dev, opts.audio_out_dev) != 0)  || (strcmp (opts.audio_in_dev,"-") == 0))
  {
    opts.split = 1;
    opts.playoffset = 0;
    opts.delay = 0;
    if (strlen(opts.wav_out_file) > 0)
    {
      openWavOutFile (&opts, &state);
    }
    else
    {
      openAudioOutDevice (&opts, SAMPLE_RATE_OUT);
    }
    openAudioInDevice (&opts);
  }
  else
  {
    opts.split = 0;
    opts.playoffset = 25;     // 38
    opts.delay = 0;
    openAudioInDevice (&opts);
    opts.audio_out_fd = opts.audio_in_fd;
  }

  if (opts.playfiles == 1)
  {
    playMbeFiles (&opts, &state, argc, argv);
  }
  else
  {
    liveScanner (&opts, &state);
  }
  cleanupAndExit (&opts, &state);
  return (0);
}
