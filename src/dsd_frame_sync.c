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


#define LBUF_SIZE   24


void printFrameSync (dsd_opts * opts, dsd_state * state, char *frametype, int offset, char *modulation)
{
  if (opts->verbose > 0)
  {
    /* Get the current time */
    getCurrentTime(opts, state);

    /* Print the current time */
    fprintf(stderr, "[%04d-%02d-%02d %02d:%02d:%02d] ", state->TimeYear, state->TimeMonth,
           state->TimeDay, state->TimeHour, state->TimeMinute, state->TimeSecond);
    fprintf(stderr, "Sync: %s ", frametype);
  }
  if (opts->verbose > 2)
  {
    fprintf(stderr, "o: %4i ", offset);
  }
  if (opts->verbose > 1)
  {
    fprintf(stderr, "mod: %s ", modulation);
  }
  if (opts->verbose > 2)
  {
    fprintf(stderr, "g: %f ", state->aout_gain);
  }
}

int getFrameSync (dsd_opts * opts, dsd_state * state)
{
  /* detects frame sync and returns frame type
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
   */


  int i, j, t, o, dibit, sync, symbol, synctest_pos, lastt;
  char synctest[25];
  char synctest12[13];
  char synctest18[19];
  char synctest32[33];
  char modulation[8];
  char *synctest_p;
  char synctest_buf[10240];
  int lmin, lmax, lidx;
  int lbuf1[LBUF_SIZE], lbuf2[LBUF_SIZE];
  int lsum;
  char spectrum[64];
  char NXDN_LICH_Parity_Is_Correct = 0;

  for (i = 0; i < LBUF_SIZE; i++)
  {
    lbuf1[i] = 0;
    lbuf2[i] = 0;
  }

  // detect frame sync
  t = 0;
  synctest[24] = 0;
  synctest12[12] = 0;
  synctest18[18] = 0;
  synctest32[32] = 0;
  synctest_pos = 0;
  synctest_p = synctest_buf + 40;  // Normally + 10 => Modified to prevent segmentation fault
  sync = 0;
  lmin = 0;
  lmax = 0;
  lidx = 0;
  lastt = 0;
  state->numflips = 0;
  if ((opts->symboltiming == 1) && (state->carrier == 1))
  {
    fprintf(stderr, "\nSymbol Timing:\n");
  }
  while (sync == 0)
  {
    t++;
    symbol = getSymbol (opts, state, 0);

    lbuf1[lidx] = symbol;
    state->sbuf[state->sidx] = symbol;
    if (lidx == (LBUF_SIZE - 1))
    {
      lidx = 0;
    }
    else
    {
      lidx++;
    }
    if (state->sidx == (opts->ssize - 1))
    {
      state->sidx = 0;
    }
    else
    {
      state->sidx++;
    }

    if (lastt == 23)
    {
      lastt = 0;
      if (state->numflips > opts->mod_threshold)
      {
        if (opts->mod_qpsk == 1)
        {
          state->rf_mod = QPSK_MODE;
        }
      }
      else if (state->numflips > 18)
      {
        if (opts->mod_gfsk == 1)
        {
          state->rf_mod = GFSK_MODE;
        }
      }
      else
      {
        if (opts->mod_c4fm == 1)
        {
          state->rf_mod = C4FM_MODE;
        }
      }
      state->numflips = 0;
    }
    else
    {
      lastt++;
    }

    if (state->dibit_buf_p > state->dibit_buf + 900000)
    {
      state->dibit_buf_p = state->dibit_buf + 200;
    }

    //determine dibit state
    if (symbol > 0)
    {
      *state->dibit_buf_p = 1;
      state->dibit_buf_p++;
      dibit = 49;               // '1'
    }
    else
    {
      *state->dibit_buf_p = 3;
      state->dibit_buf_p++;
      dibit = 51;               // '3'
    }

    *synctest_p = dibit;
    if (t >= 12)
    {
      for (i = 0; i < LBUF_SIZE; i++)
      {
        lbuf2[i] = lbuf1[i];
      }
      qsort (lbuf2, LBUF_SIZE, sizeof (int), comp);

      // min/max calculation
      lmin = (lbuf2[2] + lbuf2[3] + lbuf2[4]) / 3;
      lmax = (lbuf2[21] + lbuf2[20] + lbuf2[19]) / 3;

      if (state->rf_mod == QPSK_MODE)
      {
        state->minbuf[state->midx] = lmin;
        state->maxbuf[state->midx] = lmax;
        if (state->midx == (opts->msize - 1))
        {
          state->midx = 0;
        }
        else
        {
          state->midx++;
        }
        lsum = 0;
        for (i = 0; i < opts->msize; i++)
        {
          lsum += state->minbuf[i];
        }
        state->min = lsum / opts->msize;
        lsum = 0;
        for (i = 0; i < opts->msize; i++)
        {
          lsum += state->maxbuf[i];
        }
        state->max = lsum / opts->msize;
        state->center = ((state->max) + (state->min)) / 2;
        state->maxref = (int)((state->max) * 0.80F);
        state->minref = (int)((state->min) * 0.80F);
      }
      else
      {
        state->maxref = state->max;
        state->minref = state->min;
      }

      if (state->rf_mod == C4FM_MODE)
      {
        sprintf(modulation, "C4FM");
      }
      else if (state->rf_mod == QPSK_MODE)
      {
        sprintf(modulation, "QPSK");
      }
      else if (state->rf_mod == GFSK_MODE)
      {
        sprintf(modulation, "GFSK");
      }

      if (opts->datascope == 1)
      {
        if (lidx == 0)
        {
          for (i = 0; i < 64; i++)
          {
            spectrum[i] = 0;
          }
          for (i = 0; i < 24; i++)
          {
            o = (lbuf2[i] + 32768) / 1024;
            spectrum[o]++;
          }
          if (state->symbolcnt > (4800 / opts->scoperate))
          {
            state->symbolcnt = 0;
            fprintf(stderr, "\n");
            fprintf(stderr, "Demod mode:     %s                Nac:                     %4X\n", modulation, state->nac);
            fprintf(stderr, "Frame Type:    %s        Talkgroup:            %7i\n", state->ftype, state->lasttg);
            fprintf(stderr, "Frame Subtype: %s       Source:          %12i\n", state->fsubtype, state->lastsrc);
            fprintf(stderr, "TDMA activity:  %s %s     Voice errors: %s\n", state->slot1light, state->slot2light, state->err_str);
            fprintf(stderr, "+----------------------------------------------------------------+\n");
            for (i = 0; i < 10; i++)
            {
              fprintf(stderr, "|");
              for (j = 0; j < 64; j++)
              {
                if (i == 0)
                {
                  if ((j == ((state->min) + 32768) / 1024) || (j == ((state->max) + 32768) / 1024))
                  {
                    fprintf(stderr, "#");
                  }
                  else if (j == (state->center + 32768) / 1024)
                  {
                    fprintf(stderr, "!");
                  }
                  else
                  {
                    if (j == 32)
                    {
                      fprintf(stderr, "|");
                    }
                    else
                    {
                      fprintf(stderr, " ");
                    }
                  }
                }
                else
                {
                  if (spectrum[j] > 9 - i)
                  {
                    fprintf(stderr, "*");
                  }
                  else
                  {
                    if (j == 32)
                    {
                      fprintf(stderr, "|");
                    }
                    else
                    {
                      fprintf(stderr, " ");
                    }
                  }
                }
              }
              fprintf(stderr, "|\n");
            }
            fprintf(stderr, "+----------------------------------------------------------------+\n");
          }
        }
      } /* end if (opts->datascope == 1) */

      strncpy (synctest, (synctest_p - 23), 24);
      if (opts->frame_p25p1 == 1)
      {
        if (strcmp (synctest, P25P1_SYNC) == 0)
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          sprintf(state->ftype, " P25 Phase 1 ");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, " +P25p1    ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = 0;
          return (0);
        }
        if (strcmp (synctest, INV_P25P1_SYNC) == 0)
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          sprintf(state->ftype, " P25 Phase 1 ");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, " -P25p1    ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = 1;
          return (1);
        }
      }
      if (opts->frame_x2tdma == 1)
      {
        if ((strcmp (synctest, X2TDMA_BS_DATA_SYNC) == 0) || (strcmp (synctest, X2TDMA_MS_DATA_SYNC) == 0))
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + (lmax)) / 2;
          state->min = ((state->min) + (lmin)) / 2;
          if (opts->inverted_x2tdma == 0)
          {
            // data frame
            sprintf(state->ftype, " X2-TDMA     ");
            if (opts->errorbars == 1)
            {
              printFrameSync (opts, state, " +X2-TDMA  ", synctest_pos + 1, modulation);
            }
            state->lastsynctype = 2;
            return (2);
          }
          else
          {
            // inverted voice frame
            sprintf(state->ftype, " X2-TDMA     ");
            if (opts->errorbars == 1)
            {
              printFrameSync (opts, state, " -X2-TDMA  ", synctest_pos + 1, modulation);
            }
            if (state->lastsynctype != 3)
            {
              state->firstframe = 1;
            }
            state->lastsynctype = 3;
            return (3);
          }
        }
        if ((strcmp (synctest, X2TDMA_BS_VOICE_SYNC) == 0) || (strcmp (synctest, X2TDMA_MS_VOICE_SYNC) == 0))
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          if (opts->inverted_x2tdma == 0)
          {
            // voice frame
            sprintf(state->ftype, " X2-TDMA     ");
            if (opts->errorbars == 1)
            {
              printFrameSync (opts, state, " +X2-TDMA  ", synctest_pos + 1, modulation);
            }
            if (state->lastsynctype != 4)
            {
              state->firstframe = 1;
            }
            state->lastsynctype = 4;
            return (4);
          }
          else
          {
            // inverted data frame
            sprintf(state->ftype, " X2-TDMA     ");
            if (opts->errorbars == 1)
            {
              printFrameSync (opts, state, " -X2-TDMA  ", synctest_pos + 1, modulation);
            }
            state->lastsynctype = 5;
            return (5);
          }
        }
      } /* End if (opts->frame_x2tdma == 1) */
      if (opts->frame_dmr == 1)
      {
        if ((strcmp (synctest, DMR_MS_DATA_SYNC) == 0) || (strcmp (synctest, DMR_BS_DATA_SYNC) == 0))
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + (lmax)) / 2;
          state->min = ((state->min) + (lmin)) / 2;
          state->directmode = 0;
          if (opts->inverted_dmr == 0)
          {
            // data frame
            sprintf(state->ftype, " DMR         ");
            if (opts->errorbars == 1)
            {
              printFrameSync (opts, state, " +DMR      ", synctest_pos + 1, modulation);
            }
            state->lastsynctype = 10;
            return (10);
          }
          else
          {
            // inverted voice frame
            sprintf(state->ftype, " DMR         ");
            if (opts->errorbars == 1)
            {
              printFrameSync (opts, state, " -DMR      ", synctest_pos + 1, modulation);
            }
            if (state->lastsynctype != 11)
            {
              state->firstframe = 1;
            }
            state->lastsynctype = 11;
            return (11);
          }
        }
        if(strcmp (synctest, DMR_DIRECT_MODE_TS1_DATA_SYNC) == 0)
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + (lmax)) / 2;
          state->min = ((state->min) + (lmin)) / 2;
          state->currentslot = 0;
          state->directmode = 1;  /* Direct mode */
          if (opts->inverted_dmr == 0)
          {
            // data frame
            sprintf(state->ftype, " DMR         ");
            if (opts->errorbars == 1)
            {
              printFrameSync (opts, state, " +DMR      ", synctest_pos + 1, modulation);
            }
            state->lastsynctype = 10;
            return (10);
          }
          else
          {
            // inverted voice frame
            sprintf(state->ftype, " DMR         ");
            if (opts->errorbars == 1)
            {
              printFrameSync (opts, state, " -DMR      ", synctest_pos + 1, modulation);
            }
            if (state->lastsynctype != 11)
            {
              state->firstframe = 1;
            }
            state->lastsynctype = 11;
            return (11);
          }
        } /* End if(strcmp (synctest, DMR_DIRECT_MODE_TS1_DATA_SYNC) == 0) */
        if(strcmp (synctest, DMR_DIRECT_MODE_TS2_DATA_SYNC) == 0)
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + (lmax)) / 2;
          state->min = ((state->min) + (lmin)) / 2;
          state->currentslot = 1;
          state->directmode = 1;  /* Direct mode */
          if (opts->inverted_dmr == 0)
          {
            // data frame
            sprintf(state->ftype, " DMR         ");
            if (opts->errorbars == 1)
            {
              printFrameSync (opts, state, " +DMR      ", synctest_pos + 1, modulation);
            }
            state->lastsynctype = 10;
            return (10);
          }
          else
          {
            // inverted voice frame
            sprintf(state->ftype, " DMR         ");
            if (opts->errorbars == 1)
            {
              printFrameSync (opts, state, " -DMR      ", synctest_pos + 1, modulation);
            }
            if (state->lastsynctype != 11)
            {
              state->firstframe = 1;
            }
            state->lastsynctype = 11;
            return (11);
          }
        } /* End if(strcmp (synctest, DMR_DIRECT_MODE_TS2_DATA_SYNC) == 0) */
        if((strcmp (synctest, DMR_MS_VOICE_SYNC) == 0) || (strcmp (synctest, DMR_BS_VOICE_SYNC) == 0))
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          state->directmode = 0;
          if (opts->inverted_dmr == 0)
          {
            // voice frame
            sprintf(state->ftype, " DMR         ");
            if (opts->errorbars == 1)
            {
              printFrameSync (opts, state, " +DMR      ", synctest_pos + 1, modulation);
            }
            if (state->lastsynctype != 12)
            {
              state->firstframe = 1;
            }
            state->lastsynctype = 12;
            return (12);
          }
          else
          {
            // inverted data frame
            sprintf(state->ftype, " DMR         ");
            if (opts->errorbars == 1)
            {
              printFrameSync (opts, state, " -DMR      ", synctest_pos + 1, modulation);
            }
            state->lastsynctype = 13;
            return (13);
          }
        }
        if(strcmp (synctest, DMR_DIRECT_MODE_TS1_VOICE_SYNC) == 0)
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          state->currentslot = 0;
          state->directmode = 1;  /* Direct mode */
          if (opts->inverted_dmr == 0)
          {
            // voice frame
            sprintf(state->ftype, " DMR         ");
            if (opts->errorbars == 1)
            {
              printFrameSync (opts, state, " +DMR      ", synctest_pos + 1, modulation);
            }
            if (state->lastsynctype != 12)
            {
              state->firstframe = 1;
            }
            state->lastsynctype = 12;
            return (12);
          }
          else
          {
            // inverted data frame
            sprintf(state->ftype, " DMR         ");
            if (opts->errorbars == 1)
            {
              printFrameSync (opts, state, " -DMR      ", synctest_pos + 1, modulation);
            }
            state->lastsynctype = 13;
            return (13);
          }
        } /* End if(strcmp (synctest, DMR_DIRECT_MODE_TS1_VOICE_SYNC) == 0) */
        if(strcmp (synctest, DMR_DIRECT_MODE_TS2_VOICE_SYNC) == 0)
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          state->currentslot = 1;
          state->directmode = 1;  /* Direct mode */
          if (opts->inverted_dmr == 0)
          {
            // voice frame
            sprintf(state->ftype, " DMR         ");
            if (opts->errorbars == 1)
            {
              printFrameSync (opts, state, " +DMR      ", synctest_pos + 1, modulation);
            }
            if (state->lastsynctype != 12)
            {
              state->firstframe = 1;
            }
            state->lastsynctype = 12;
            return (12);
          }
          else
          {
            // inverted data frame
            sprintf(state->ftype, " DMR         ");
            if (opts->errorbars == 1)
            {
              printFrameSync (opts, state, " -DMR      ", synctest_pos + 1, modulation);
            }
            state->lastsynctype = 13;
            return (13);
          }
        } /* End if(strcmp (synctest, DMR_DIRECT_MODE_TS2_VOICE_SYNC) == 0) */
      } /* End if (opts->frame_dmr == 1) */
      if(opts->frame_provoice == 1)
      {
        strncpy (synctest32, (synctest_p - 31), 32);
        if ((strcmp (synctest32, PROVOICE_SYNC) == 0) || (strcmp (synctest32, PROVOICE_EA_SYNC) == 0))
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          sprintf(state->ftype, " ProVoice    ");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, " -ProVoice ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = 14;
          return (14);
        }
        else if ((strcmp (synctest32, INV_PROVOICE_SYNC) == 0) || (strcmp (synctest32, INV_PROVOICE_EA_SYNC) == 0))
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          sprintf(state->ftype, " ProVoice    ");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, " -ProVoice ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = 15;
          return (15);
        }
      } /* End if(opts->frame_provoice == 1) */
      if ((opts->frame_nxdn96 == 1) || (opts->frame_nxdn48 == 1))
      {
        strncpy (synctest18, (synctest_p - 17), 18);

        if (strncmperr (synctest18, NXDN_SYNC, 10, 1) == 0)
        {
          /* Save previous LICH */
          state->NxdnLich.PreviousCompleteLich = state->NxdnLich.CompleteLich;

          /* Process LICH */
          NXDN_LICH_Parity_Is_Correct = (char)NXDN_decode_LICH((uint8_t*)&synctest18[10],
                                                               (uint8_t*)&state->NxdnLich.RFChannelType,
                                                               (uint8_t*)&state->NxdnLich.FunctionnalChannelType,
                                                               (uint8_t*)&state->NxdnLich.Option,
                                                               (uint8_t*)&state->NxdnLich.Direction,
                                                               (uint8_t*)&state->NxdnLich.CompleteLich,
                                                               0);

//          fprintf(stderr, "LITCH Parity=%d %s ; RF Channel Type=%u%u ; Functional Channel Type=%u%u ; Option=%u%u ; Direction=%u %s\n",
//                 NXDN_LICH_Parity_Is_Correct,
//                 (NXDN_LICH_Parity_Is_Correct ? "(OK) " : "(ERR)"),
//                 (state->NxdnLich.RFChannelType >> 1) & 0x01,
//                 state->NxdnLich.RFChannelType & 0x01,
//                 (state->NxdnLich.FunctionnalChannelType >> 1) & 0x01,
//                 state->NxdnLich.FunctionnalChannelType & 0x01,
//                 (state->NxdnLich.Option >> 1) & 0x01,
//                 state->NxdnLich.Option & 0x01,
//                 state->NxdnLich.Direction,
//                 (state->NxdnLich.Direction ? "(Outbound)" : "(Inbound) "));

          if(NXDN_LICH_Parity_Is_Correct)// || (state->NxdnLich.PreviousCompleteLich == state->NxdnLich.CompleteLich))
          {
            if ((state->lastsynctype == 8) || (state->lastsynctype == 16) || (state->lastsynctype == 28))
            {
              state->carrier = 1;
              state->offset = synctest_pos;
              state->max = ((state->max) + lmax) / 2;
              state->min = ((state->min) + lmin) / 2;
              if (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_NXDN48)
              {
                sprintf(state->ftype, " NXDN48      ");
                if (opts->errorbars == 1)
                {
                  printFrameSync (opts, state, " +NXDN48   ", synctest_pos + 1, modulation);
                }
              }
              else /* (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_NXDN96) */
              {
                sprintf(state->ftype, " NXDN96      ");
                if (opts->errorbars == 1)
                {
                  printFrameSync (opts, state, " +NXDN96   ", synctest_pos + 1, modulation);
                }
              }
              state->lastsynctype = 28;
              return (28);
            }
            else
            {
              state->lastsynctype = 28;
            }
          } /* End if(NXDN_LICH_Parity_Is_Correct || (state->NxdnLich.PreviousCompleteLich == state->NxdnLich.CompleteLich)) */
        } /* End if (strncmperr (synctest18, NXDN_SYNC, 10, 1) == 0) */
        else if (strncmperr (synctest18, INV_NXDN_SYNC, 10, 1) == 0)
        {
          /* Save previous LICH */
          state->NxdnLich.PreviousCompleteLich = state->NxdnLich.CompleteLich;

          /* Process LICH */
          NXDN_LICH_Parity_Is_Correct = (char)NXDN_decode_LICH((uint8_t*)&synctest18[10],
                                                               (uint8_t*)&state->NxdnLich.RFChannelType,
                                                               (uint8_t*)&state->NxdnLich.FunctionnalChannelType,
                                                               (uint8_t*)&state->NxdnLich.Option,
                                                               (uint8_t*)&state->NxdnLich.Direction,
                                                               (uint8_t*)&state->NxdnLich.CompleteLich,
                                                               1);

          if(NXDN_LICH_Parity_Is_Correct)// || (state->NxdnLich.PreviousCompleteLich == state->NxdnLich.CompleteLich))
          {
            if ((state->lastsynctype == 9) || (state->lastsynctype == 17) || (state->lastsynctype == 29))
            {
              state->carrier = 1;
              state->offset = synctest_pos;
              state->max = ((state->max) + lmax) / 2;
              state->min = ((state->min) + lmin) / 2;
              if (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_NXDN48)
              {
                sprintf(state->ftype, " NXDN48      ");
                if (opts->errorbars == 1)
                {
                  printFrameSync (opts, state, " -NXDN48   ", synctest_pos + 1, modulation);
                }
              }
              else /* (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_NXDN96) */
              {
                sprintf(state->ftype, " NXDN96      ");
                if (opts->errorbars == 1)
                {
                  printFrameSync (opts, state, " -NXDN96   ", synctest_pos + 1, modulation);
                }
              }
              state->lastsynctype = 29;
              return (29);
            }
            else
            {
              state->lastsynctype = 29;
            }
          } /* End if(NXDN_LICH_Parity_Is_Correct || (state->NxdnLich.PreviousCompleteLich == state->NxdnLich.CompleteLich)) */
        } /* End else if (strncmperr (synctest18, INV_NXDN_SYNC, 10, 1) == 0) */
        else
        {
          /* Nothing to do */
        }

//        if ((strncmperr (synctest18, NXDN_BS_VOICE_SYNC, 18, 1) == 0) || (strncmperr (synctest18, NXDN_MS_VOICE_SYNC, 18, 1) == 0))
//        {
//          if ((state->lastsynctype == 8) || (state->lastsynctype == 16))
//          {
//            state->carrier = 1;
//            state->offset = synctest_pos;
//            state->max = ((state->max) + lmax) / 2;
//            state->min = ((state->min) + lmin) / 2;
//            if (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_NXDN48)
//            {
//              sprintf(state->ftype, " NXDN48      ");
//              if (opts->errorbars == 1)
//              {
//                printFrameSync (opts, state, " +NXDN48   ", synctest_pos + 1, modulation);
//              }
//            }
//            else /* (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_NXDN96) */
//            {
//              sprintf(state->ftype, " NXDN96      ");
//              if (opts->errorbars == 1)
//              {
//                printFrameSync (opts, state, " +NXDN96   ", synctest_pos + 1, modulation);
//              }
//            }
//            state->lastsynctype = 8;
//            return (8);
//          }
//          else
//          {
//            state->lastsynctype = 8;
//          }
//        }
//        else if ((strncmperr (synctest18, INV_NXDN_BS_VOICE_SYNC, 18, 1) == 0) || (strncmperr (synctest18, INV_NXDN_MS_VOICE_SYNC, 18, 1) == 0))
//        {
//          if ((state->lastsynctype == 9) || (state->lastsynctype == 17))
//          {
//            state->carrier = 1;
//            state->offset = synctest_pos;
//            state->max = ((state->max) + lmax) / 2;
//            state->min = ((state->min) + lmin) / 2;
//            if (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_NXDN48)
//            {
//              sprintf(state->ftype, " NXDN48      ");
//              if (opts->errorbars == 1)
//              {
//                printFrameSync (opts, state, " -NXDN48   ", synctest_pos + 1, modulation);
//              }
//            }
//            else /* (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_NXDN96) */
//            {
//              sprintf(state->ftype, " NXDN96      ");
//              if (opts->errorbars == 1)
//              {
//                printFrameSync (opts, state, " -NXDN96   ", synctest_pos + 1, modulation);
//              }
//            }
//            state->lastsynctype = 9;
//            return (9);
//          }
//          else
//          {
//            state->lastsynctype = 9;
//          }
//        }
//        else if ((strncmperr (synctest18, NXDN_BS_DATA_SYNC, 18, 1) == 0) || (strncmperr (synctest18, NXDN_MS_DATA_SYNC, 18, 1) == 0))
//        {
//          if ((state->lastsynctype == 8) || (state->lastsynctype == 16))
//          {
//            state->carrier = 1;
//            state->offset = synctest_pos;
//            state->max = ((state->max) + lmax) / 2;
//            state->min = ((state->min) + lmin) / 2;
//            if (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_NXDN48)
//            {
//              sprintf(state->ftype, " NXDN48      ");
//              if (opts->errorbars == 1)
//              {
//                printFrameSync (opts, state, " +NXDN48   ", synctest_pos + 1, modulation);
//              }
//            }
//            else /* (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_NXDN96) */
//            {
//              sprintf(state->ftype, " NXDN96      ");
//              if (opts->errorbars == 1)
//              {
//                printFrameSync (opts, state, " +NXDN96   ", synctest_pos + 1, modulation);
//              }
//            }
//            state->lastsynctype = 16;
//            return (16);
//          }
//          else
//          {
//            state->lastsynctype = 16;
//          }
//        }
//        else if ((strncmperr (synctest18, INV_NXDN_BS_DATA_SYNC, 18, 1) == 0) || (strncmperr (synctest18, INV_NXDN_MS_DATA_SYNC, 18, 1) == 0))
//        {
//          if ((state->lastsynctype == 9) || (state->lastsynctype == 17))
//          {
//            state->carrier = 1;
//            state->offset = synctest_pos;
//            state->max = ((state->max) + lmax) / 2;
//            state->min = ((state->min) + lmin) / 2;
//            sprintf(state->ftype, " NXDN        ");
//            if (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_NXDN48)
//            {
//              sprintf(state->ftype, " NXDN48      ");
//              if (opts->errorbars == 1)
//              {
//                printFrameSync (opts, state, " -NXDN48   ", synctest_pos + 1, modulation);
//              }
//            }
//            else /* (state->samplesPerSymbol == SAMPLE_PER_SYMBOL_NXDN96) */
//            {
//              sprintf(state->ftype, " NXDN96      ");
//              if (opts->errorbars == 1)
//              {
//                printFrameSync (opts, state, " -NXDN96   ", synctest_pos + 1, modulation);
//              }
//            }
//            state->lastsynctype = 17;
//            return (17);
//          }
//          else
//          {
//            state->lastsynctype = 17;
//          }
//        }
//        else
//        {
//          /* Nothing to do */
//        }
      } /* End if ((opts->frame_nxdn96 == 1) || (opts->frame_nxdn48 == 1)) */
      if (opts->frame_dstar == 1)
      {
        if (strcmp (synctest, DSTAR_SYNC) == 0)
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          sprintf(state->ftype, " D-STAR      ");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, " +D-STAR   ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = 6;
          return (6);
        }
        if (strcmp (synctest, INV_DSTAR_SYNC) == 0)
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          sprintf(state->ftype, " D-STAR      ");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, " -D-STAR   ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = 7;
          return (7);
        }
        if (strcmp (synctest, DSTAR_HD) == 0)
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          sprintf(state->ftype, " D-STAR_HD   ");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, " +D-STAR_HD   ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = 18;
          return (18);
        }
        if (strcmp (synctest, INV_DSTAR_HD) == 0)
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          sprintf(state->ftype, " D-STAR_HD   ");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, " -D-STAR_HD   ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = 19;
          return (19);
        }
      } /* End if (opts->frame_dstar == 1) */

      if ((t == 24) && (state->lastsynctype != -1))
      {
        if ((state->lastsynctype == 0) && ((state->lastp25type == 1) || (state->lastp25type == 2)))
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + (lmax)) / 2;
          state->min = ((state->min) + (lmin)) / 2;
          sprintf(state->ftype, "(P25 Phase 1)");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, "(+P25p1)   ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = -1;
          return (0);
        }
        else if ((state->lastsynctype == 1) && ((state->lastp25type == 1) || (state->lastp25type == 2)))
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          sprintf(state->ftype, "(P25 Phase 1)");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, "(-P25p1)   ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = -1;
          return (1);
        }
        else if ((state->lastsynctype == 3) && ((strcmp (synctest, X2TDMA_BS_VOICE_SYNC) != 0) || (strcmp (synctest, X2TDMA_MS_VOICE_SYNC) != 0)))
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          sprintf(state->ftype, "(X2-TDMA)    ");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, "(-X2-TDMA) ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = -1;
          return (3);
        }
        else if ((state->lastsynctype == 4) && ((strcmp (synctest, X2TDMA_BS_DATA_SYNC) != 0) || (strcmp (synctest, X2TDMA_MS_DATA_SYNC) != 0)))
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          sprintf(state->ftype, "(X2-TDMA)    ");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, "(+X2-TDMA) ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = -1;
          return (4);
        }
        else if ((state->lastsynctype == 11) && ((strcmp (synctest, DMR_BS_VOICE_SYNC) != 0) || (strcmp (synctest, DMR_MS_VOICE_SYNC) != 0)))
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          state->directmode = 0;
          sprintf(state->ftype, "(DMR)        ");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, "(-DMR)     ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = -1;
          return (11);
        }
        else if ((state->lastsynctype == 11) && (strcmp (synctest, DMR_DIRECT_MODE_TS1_VOICE_SYNC) == 0))
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          state->currentslot = 0;
          state->directmode = 1;  /* Direct mode */
          sprintf(state->ftype, "(DMR)        ");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, "(-DMR)     ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = -1;
          return (11);
        }
        else if ((state->lastsynctype == 11) && (strcmp (synctest, DMR_DIRECT_MODE_TS2_VOICE_SYNC) == 0))
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          state->currentslot = 1;
          state->directmode = 1;  /* Direct mode */
          sprintf(state->ftype, "(DMR)        ");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, "(-DMR)     ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = -1;
          return (11);
        }
        else if ((state->lastsynctype == 12) && ((strcmp (synctest, DMR_BS_DATA_SYNC) != 0) || (strcmp (synctest, DMR_MS_DATA_SYNC) != 0)))
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          sprintf(state->ftype, "(DMR)        ");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, "(+DMR)     ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = -1;
          return (12);
        }
        else if((state->lastsynctype == 12) && (strcmp (synctest, DMR_DIRECT_MODE_TS1_DATA_SYNC) == 0))
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          state->currentslot = 0;
          state->directmode = 1;  /* Direct mode */
          sprintf(state->ftype, "(DMR)        ");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, "(+DMR)     ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = -1;
          return (12);
        }
        else if((state->lastsynctype == 12) && (strcmp (synctest, DMR_DIRECT_MODE_TS2_DATA_SYNC) == 0))
        {
          state->carrier = 1;
          state->offset = synctest_pos;
          state->max = ((state->max) + lmax) / 2;
          state->min = ((state->min) + lmin) / 2;
          state->currentslot = 1;
          state->directmode = 1;  /* Direct mode */
          sprintf(state->ftype, "(DMR)        ");
          if (opts->errorbars == 1)
          {
            printFrameSync (opts, state, "(+DMR)     ", synctest_pos + 1, modulation);
          }
          state->lastsynctype = -1;
          return (12);
        }
      } /* End if ((t == 24) && (state->lastsynctype != -1)) */

      if(opts->frame_dpmr == 1)
      {
        strncpy(synctest,   (synctest_p - 23), 24);
        strncpy(synctest12, (synctest_p - 11), 12);

        if(strcmp(synctest, DPMR_FRAME_SYNC_1) == 0)
        {
          if (opts->inverted_dpmr == 0)
          {
            //fprintf(stderr, "DPMR_FRAME_SYNC_1\n"); // TODO : To be removed

            if ((state->lastsynctype == 20) || (state->lastsynctype == 21) ||
                (state->lastsynctype == 22) || (state->lastsynctype == 23))
            {
              state->carrier = 1;
              state->offset = synctest_pos;
              state->max = ((state->max) + lmax) / 2;
              state->min = ((state->min) + lmin) / 2;
              sprintf(state->ftype, " dPMR        ");
              if (opts->errorbars == 1)
              {
                printFrameSync (opts, state, " +dPMR     ", synctest_pos + 1, modulation);
              }

              /* The next part of the superframe will normally be the first part */
              //opts->dPMR_next_part_of_superframe = 1;

              state->lastsynctype = 20;
              return (20);
            }
            else
            {
              state->lastsynctype = 20;
            }
          } /* End if (opts->inverted_dpmr == 0) */
        }
        else if(strcmp(synctest12, DPMR_FRAME_SYNC_2) == 0)
        {
          if (opts->inverted_dpmr == 0)
          {
            //fprintf(stderr, "DPMR_FRAME_SYNC_2\n"); // TODO : To be removed

            // TODO : Modif 2019-01-04
            //if ((state->lastsynctype == 20) || (state->lastsynctype == 21) ||
            //    (state->lastsynctype == 22) || (state->lastsynctype == 23))
            //{
              state->carrier = 1;
              state->offset = synctest_pos;
              state->max = ((state->max) + lmax) / 2;
              state->min = ((state->min) + lmin) / 2;

              sprintf(state->ftype, " dPMR        ");
              if (opts->errorbars == 1)
              {
                printFrameSync (opts, state, " +dPMR     ", synctest_pos + 1, modulation);
              }

              state->lastsynctype = 21;
              return (21);
            // TODO : Modif 2019-01-04
            //}
            //else
            //{
            //  state->lastsynctype = 21;
            //}
          }
        }
        else if(strcmp(synctest12, DPMR_FRAME_SYNC_3) == 0)
        {
          if (opts->inverted_dpmr == 0)
          {
            //fprintf(stderr, "DPMR_FRAME_SYNC_3\n"); // TODO : To be removed

            if ((state->lastsynctype == 20) || (state->lastsynctype == 21) ||
                (state->lastsynctype == 22) || (state->lastsynctype == 23))
            {
              state->carrier = 1;
              state->offset = synctest_pos;
              state->max = ((state->max) + lmax) / 2;
              state->min = ((state->min) + lmin) / 2;
              sprintf(state->ftype, " dPMR        ");
              if (opts->errorbars == 1)
              {
                printFrameSync (opts, state, " +dPMR     ", synctest_pos + 1, modulation);
              }

              /* The next part of the superframe will normally be the first part */
              //opts->dPMR_next_part_of_superframe = 1;

              state->lastsynctype = 22;
              return (22);
            }
            else
            {
              state->lastsynctype = 22;
            }
          }
        }
        if(strcmp(synctest, DPMR_FRAME_SYNC_4) == 0)
        {
          if (opts->inverted_dpmr == 0)
          {
            //fprintf(stderr, "DPMR_FRAME_SYNC_4\n"); // TODO : To be removed

            if ((state->lastsynctype == 20) || (state->lastsynctype == 21) ||
                (state->lastsynctype == 22) || (state->lastsynctype == 23))
            {
              state->carrier = 1;
              state->offset = synctest_pos;
              state->max = ((state->max) + lmax) / 2;
              state->min = ((state->min) + lmin) / 2;
              sprintf(state->ftype, " dPMR        ");
              if (opts->errorbars == 1)
              {
                printFrameSync (opts, state, " +dPMR     ", synctest_pos + 1, modulation);
              }

              /* The next part of the superframe will normally be the first part */
              //opts->dPMR_next_part_of_superframe = 1;

              state->lastsynctype = 23;
              return (23);
            }
            else
            {
              state->lastsynctype = 23;
            }
          }
        }
        else if(strcmp(synctest, INV_DPMR_FRAME_SYNC_1) == 0)
        {
          if (opts->inverted_dpmr)
          {
            //fprintf(stderr, "INV_DPMR_FRAME_SYNC_1\n"); // TODO : To be removed
            if ((state->lastsynctype == 24) || (state->lastsynctype == 25) ||
                (state->lastsynctype == 26) || (state->lastsynctype == 27))
            {
              state->carrier = 1;
              state->offset = synctest_pos;
              state->max = ((state->max) + lmax) / 2;
              state->min = ((state->min) + lmin) / 2;
              sprintf(state->ftype, " dPMR        ");
              if (opts->errorbars == 1)
              {
                printFrameSync (opts, state, " -dPMR     ", synctest_pos + 1, modulation);
              }
              state->lastsynctype = 24;
              return (24);
            }
            else
            {
              state->lastsynctype = 24;
            }
          }
        }
        else if(strcmp(synctest12, INV_DPMR_FRAME_SYNC_2) == 0)
        {
          if (opts->inverted_dpmr)
          {
            //fprintf(stderr, "DPMR_FRAME_SYNC_2\n"); // TODO : To be removed

            if ((state->lastsynctype == 24) || (state->lastsynctype == 25) ||
                (state->lastsynctype == 26) || (state->lastsynctype == 27))
            {
              state->carrier = 1;
              state->offset = synctest_pos;
              state->max = ((state->max) + lmax) / 2;
              state->min = ((state->min) + lmin) / 2;
              sprintf(state->ftype, " dPMR        ");
              if (opts->errorbars == 1)
              {
                printFrameSync (opts, state, " -dPMR     ", synctest_pos + 1, modulation);
              }
              state->lastsynctype = 25;
              return (25);
            }
            else
            {
              state->lastsynctype = 25;
            }
          }
        }
        else if(strcmp(synctest12, INV_DPMR_FRAME_SYNC_3) == 0)
        {
          if (opts->inverted_dpmr)
          {
            //fprintf(stderr, "INV_DPMR_FRAME_SYNC_3\n"); // TODO : To be removed

            if ((state->lastsynctype == 24) || (state->lastsynctype == 25) ||
                (state->lastsynctype == 26) || (state->lastsynctype == 27))
            {
              state->carrier = 1;
              state->offset = synctest_pos;
              state->max = ((state->max) + lmax) / 2;
              state->min = ((state->min) + lmin) / 2;
              sprintf(state->ftype, " dPMR        ");
              if (opts->errorbars == 1)
              {
                printFrameSync (opts, state, " -dPMR     ", synctest_pos + 1, modulation);
              }
              state->lastsynctype = 26;
              return (26);
            }
            else
            {
              state->lastsynctype = 26;
            }
          }
        }
        if(strcmp(synctest, INV_DPMR_FRAME_SYNC_4) == 0)
        {
          if (opts->inverted_dpmr)
          {
            //fprintf(stderr, "INV_DPMR_FRAME_SYNC_4\n"); // TODO : To be removed

            if ((state->lastsynctype == 24) || (state->lastsynctype == 25) ||
                (state->lastsynctype == 26) || (state->lastsynctype == 27))
            {
              state->carrier = 1;
              state->offset = synctest_pos;
              state->max = ((state->max) + lmax) / 2;
              state->min = ((state->min) + lmin) / 2;
              sprintf(state->ftype, " dPMR        ");
              if (opts->errorbars == 1)
              {
                printFrameSync (opts, state, " -dPMR     ", synctest_pos + 1, modulation);
              }
              state->lastsynctype = 27;
              return (27);
            }
            else
            {
              state->lastsynctype = 27;
            }
          }
        }
        else
        {
          /* No dPMR frame sync detected */
        }
      } /* End if(opts->frame_dpmr == 1) */
    } /* End if (t >= 12) */

    if (exitflag == 1)
    {
      cleanupAndExit (opts, state);
    }

    if (synctest_pos < 10200)
    {
      synctest_pos++;
      synctest_p++;
    }
    else
    {
      // buffer reset
      synctest_pos = 0;
      synctest_p = synctest_buf + 40;  // Normally + 10 => Modified to prevent segmentation fault
      noCarrier (opts, state);
    }

    if (state->lastsynctype != 1)
    {
      if (synctest_pos >= 1800)
      {
        if ((opts->errorbars == 1) && (opts->verbose > 1) && (state->carrier == 1))
        {
          fprintf(stderr, "Sync: no sync\n");
        }
        noCarrier (opts, state);
        return (-1);
      }
    }
  } /* End while (sync == 0) */

  return (-1);
}
