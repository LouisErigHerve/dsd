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

#include "dsd.h"

/* Extract 1 symbol */
int getSymbol (dsd_opts * opts, dsd_state * state, int have_sync)
{
  short sample;
  int i;
  int sum, symbol, count;
  ssize_t result;
  int index = 1;

  UNUSED_VARIABLE(index);

  sum = 0;
  count = 0;

  /* There is up to 20 samples, so make up to 20 loop */
  for (i = 0; i < state->samplesPerSymbol; i++)
  {
    // timing control
    if ((i == 0) && (have_sync == 0))
    {
      if (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_NXDN48)
      {
        //if ((state->jitter >= 7) && (state->jitter <= 10))
        if ((state->jitter >= ((SAMPLE_PER_SYMBOL_NXDN48 / 2) - 3)) && (state->jitter <= (SAMPLE_PER_SYMBOL_NXDN48 / 2)))
        {
          i--;
        }
        else if ((state->jitter >= ((SAMPLE_PER_SYMBOL_NXDN48 / 2) + 1)) && (state->jitter <= ((SAMPLE_PER_SYMBOL_NXDN48 / 2) + 4)))
        {
          i++;
        }
      }
      else if (state->rf_mod == QPSK_MODE) /* QPSK Mode */
      {
        if ((state->jitter >= 0) && (state->jitter < state->symbolCenter))
        {
          i++;          // fall back
        }
        else if ((state->jitter > state->symbolCenter) && (state->jitter < state->samplesPerSymbol))
        {
          i--;          // catch up
        }
      }
      else if (state->rf_mod == GFSK_MODE) /* GFSK Mode */
      {
        if ((state->jitter >= state->symbolCenter - 1) && (state->jitter <= state->symbolCenter))
        {
          i--;
        }
        else if ((state->jitter >= state->symbolCenter + 1) && (state->jitter <= state->symbolCenter + 2))
        {
          i++;
        }
        else
        {
          /* Nothing to do */
        }
      }
      else if (state->rf_mod == C4FM_MODE) /* C4FM Mode */
      {
        if ((state->jitter > 0) && (state->jitter <= state->symbolCenter))
        {
          i--;          // catch up
        }
        else if ((state->jitter > state->symbolCenter) && (state->jitter < state->samplesPerSymbol))
        {
          i++;          // fall back
        }
      }
      state->jitter = -1;
    }

    // Read the new sample from the input
    if(opts->audio_in_type == 0)
    {
      sample = 0;
      result = read (opts->audio_in_fd, &sample, 2);

      if(result == -1)
      {
        cleanupAndExit (opts, state);
      }
    }
    else if (opts->audio_in_type == 1)
    {
      sample = 0;
      result = sf_read_short(opts->audio_in_file, &sample, 1);

      if(result == 0)
      {
        cleanupAndExit (opts, state);
      }
    }
    else
    {
#ifdef USE_PORTAUDIO
      sample = 0;
      PaError err = Pa_ReadStream( opts->audio_in_pa_stream, &sample, 1 );

      if( err != paNoError )
      {
        fprintf( stderr, "An error occured while using the portaudio input stream\n" );
        fprintf( stderr, "Error number: %d\n", err );
        fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
      }
#endif
    }

#ifdef TRACE_DSD
    state->debug_sample_index++;
#endif

    // fprintf(stderr, "res: %zd\n, offset: %lld", result, sf_seek(opts->audio_in_file, 0, SEEK_CUR));
    if (opts->use_cosine_filter)
    {
      /* Frame sync type
       *  0 = +P25p1
       *  1 = -P25p1
       *  2 = +X2-TDMA (non inverted signal data frame)
       *  3 = -X2-TDMA (inverted signal voice frame)
       *  4 = +X2-TDMA (non inverted signal voice frame)
       *  5 = -X2-TDMA (inverted signal data frame)
       *  6 = +D-STAR
       *  7 = -D-STAR
       *  8 = +NXDN (non inverted voice frame)
       *  9 = -NXDN (inverted voice frame)
       * 10 = +DMR (non inverted signal data frame)
       * 11 = -DMR (inverted signal voice frame)
       * 12 = +DMR (non inverted signal voice frame)
       * 13 = -DMR (inverted signal data frame)
       * 14 = +ProVoice
       * 15 = -ProVoice
       * 16 = +NXDN (non inverted data frame)
       * 17 = -NXDN (inverted data frame)
       * 18 = +D-STAR_HD
       * 19 = -D-STAR_HD
       * 20 = +dPMR Frame Sync 1
       * 21 = +dPMR Frame Sync 2
       * 22 = +dPMR Frame Sync 3
       * 23 = +dPMR Frame Sync 4
       * 24 = -dPMR Frame Sync 1
       * 25 = -dPMR Frame Sync 2
       * 26 = -dPMR Frame Sync 3
       * 27 = -dPMR Frame Sync 4
       * 28 = +NXDN (sync only)
       * 29 = -NXDN (sync only)
       * 30 = +YSF
       * 31 = -YSF
       * 32 = +DMR MS Voice
       * 33 = +DMR MS Data
       * 34 = +DMR RC Data
       * 35 = +P25 P2
       * 36 = -P25 P2
       * 37 = +EDACS
       * 38 = -EDACS
       * 39 = -DMR MS Voice (inverted)
       * 40 = -DMR MS Data (inverted)
       * 41 = -DMR RC Data (inverted)
       */
      if ((state->lastsynctype >= 10 && state->lastsynctype <= 13) ||
          (state->lastsynctype >= 32 && state->lastsynctype <= 34) ||
          (state->lastsynctype >= 39 && state->lastsynctype <= 41))
      {
        sample = dmr_filter(sample);
      }
      else if (state->lastsynctype == 8  || state->lastsynctype == 9  ||
               state->lastsynctype == 16 || state->lastsynctype == 17 ||
               state->lastsynctype == 20 || state->lastsynctype == 21 ||
               state->lastsynctype == 22 || state->lastsynctype == 23 ||
               state->lastsynctype == 24 || state->lastsynctype == 25 ||
               state->lastsynctype == 26 || state->lastsynctype == 27 ||
               state->lastsynctype == 28 || state->lastsynctype == 29  )
      {
        if(state->samplesPerSymbol == SAMPLE_PER_SYMBOL_NXDN48)
        {
          /* NXDN 4800 baud and dPMR share the same filter */
          sample = nxdn_filter(sample);
        }
        else /* The 12.5KHz NXDN filter is the same as the DMR filter */
        {
          sample = dmr_filter(sample);
        }
      }
    } /* End if (opts->use_cosine_filter) */

    if ((sample > state->max) && (have_sync == 1) && (state->rf_mod == C4FM_MODE))
    {
      sample = state->max;
    }
    else if ((sample < state->min) && (have_sync == 1) && (state->rf_mod == C4FM_MODE))
    {
      sample = state->min;
    }

    if (sample > state->center)
    {
      if (state->lastsample < state->center)
      {
        state->numflips += 1;
      }
      if (sample > (state->maxref * 1.25))
      {
        if (state->lastsample < (state->maxref * 1.25))
        {
          state->numflips += 1;
        }
        if ((state->jitter < 0) && (state->rf_mod == QPSK_MODE))
        {               // first spike out of place
          state->jitter = i;
        }
        if ((opts->symboltiming == 1) && (have_sync == 0) && (state->lastsynctype != -1))
        {
          fprintf(stderr, "O");
        }
      }
      else
      {
        if ((opts->symboltiming == 1) && (have_sync == 0) && (state->lastsynctype != -1))
        {
          fprintf(stderr, "+");
        }
        if ((state->jitter < 0) && (state->lastsample < state->center) && (state->rf_mod != QPSK_MODE))
        {               // first transition edge
          state->jitter = i;
        }
      }
    }
    else
    {                       // sample < 0
      if (state->lastsample > state->center)
      {
        state->numflips += 1;
      }
      if (sample < (state->minref * 1.25))
      {
        if (state->lastsample > (state->minref * 1.25))
        {
          state->numflips += 1;
        }
        if ((state->jitter < 0) && (state->rf_mod == QPSK_MODE))
        {               // first spike out of place
          state->jitter = i;
        }
        if ((opts->symboltiming == 1) && (have_sync == 0) && (state->lastsynctype != -1))
        {
          fprintf(stderr, "X");
        }
      }
      else
      {
        if ((opts->symboltiming == 1) && (have_sync == 0) && (state->lastsynctype != -1))
        {
          fprintf(stderr, "-");
        }
        if ((state->jitter < 0) && (state->lastsample > state->center) && (state->rf_mod != QPSK_MODE))
        {               // first transition edge
          state->jitter = i;
        }
      }
    }
    if (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_NXDN48) /* NXDN48 and dPMR */
    {
      //if ((i >= 9) && (i <= 11))  // Modif 2020-04-17
      if((i >= (SAMPLE_PER_SYMBOL_NXDN48 / 2) - 1) && (i <= (SAMPLE_PER_SYMBOL_NXDN48 / 2) + 1))
      {
        sum += sample;
        count++;
      }
    }
    if (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_NXDN96)
    {
      if (i == 2)
      {
        sum += sample;
        count++;
      }
    }
    else /* (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_DMR) || (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_NXDN48) */
    {
      if (state->rf_mod == C4FM_MODE)
      {
        // rf_mode = 0: C4FM modulation

        if ((i >= state->symbolCenter - 1) && (i <= state->symbolCenter + 2))
        {
          sum += sample;
          count++;
        }

#ifdef TRACE_DSD
        if (i == state->symbolCenter - 1) {
          state->debug_sample_left_edge = state->debug_sample_index - 1;
        }
        if (i == state->symbolCenter + 2) {
          state->debug_sample_right_edge = state->debug_sample_index - 1;
        }
#endif
      }
      else
      {
        /* Index +/- 10% */
        index = state->symbolCenter / 10;

        // rf_mod = 1: QPSK modulation
        // rf_mod = 2: GFSK modulation
        // Note: this has been changed to use an additional symbol to the left
        // On the p25_raw_unencrypted.flac it is evident that the timing
        // comes one sample too late.
        // This change makes a significant improvement in the BER, at least for
        // this file.
        //if ((i == state->symbolCenter) || (i == state->symbolCenter + 1))
        if ((i == state->symbolCenter - 1) || (i == state->symbolCenter + 1))
        //if ((i >= state->symbolCenter - index) || (i <= state->symbolCenter + index))
        //if ((i >= state->symbolCenter - 1) || (i <= state->symbolCenter + 1))    // TODO : Modif 2019-01-04
        {
          sum += sample;
          count++;
        }

#ifdef TRACE_DSD
        //if (i == state->symbolCenter) {
        if (i == state->symbolCenter - 1) {
          state->debug_sample_left_edge = state->debug_sample_index - 1;
        }
        if (i == state->symbolCenter + 1) {
          state->debug_sample_right_edge = state->debug_sample_index - 1;
        }
#endif
      }
    }

    state->lastsample = sample;
  }   // for

  symbol = (sum / count);

  if ((opts->symboltiming == 1) && (have_sync == 0) && (state->lastsynctype != -1))
  {
    if (state->jitter >= 0)
    {
      fprintf(stderr, " %i\n", state->jitter);
    }
    else
    {
      fprintf(stderr, "\n");
    }
  }

#ifdef TRACE_DSD
  if (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_DMR) {
    float left, right;
    if (state->debug_label_file == NULL) {
      state->debug_label_file = fopen ("pp_label.txt", "w");
    }
    left = state->debug_sample_left_edge / SAMPLE_RATE_IN;
    right = state->debug_sample_right_edge / SAMPLE_RATE_IN;
    if (state->debug_prefix != '\0') {
      if (state->debug_prefix == 'I') {
        fprintf(state->debug_label_file, "%f\t%f\t%c%c %i\n", left, right, state->debug_prefix, state->debug_prefix_2, symbol);
      } else {
        fprintf(state->debug_label_file, "%f\t%f\t%c %i\n", left, right, state->debug_prefix, symbol);
      }
    } else {
      fprintf(state->debug_label_file, "%f\t%f\t%i\n", left, right, symbol);
    }
  }
#endif


  state->symbolcnt++;
  return (symbol);
}

