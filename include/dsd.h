#ifndef DSD_H
#define DSD_H

#ifdef __cplusplus
extern "C" {
#endif


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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#define __USE_XOPEN
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef SOLARIS
#include <sys/audioio.h>
#endif
#if defined(BSD) && !defined(__APPLE__)
#include <sys/soundcard.h>
#endif
#include <math.h>
#include <mbelib.h>
#include <sndfile.h>
#include <stdbool.h>

#include "p25p1_heuristics.h"


/* Define -------------------------------------------------------------------*/
#define UNUSED_VARIABLE(x) (x = x)

#define SAMPLE_RATE_IN 48000
#define SAMPLE_RATE_OUT 8000

/* Could only be 2 or 4 */
#define NB_OF_DPMR_VOICE_FRAME_TO_DECODE 2


#define BUILD_DSD_WITH_FRAME_CONTENT_DISPLAY


#ifdef USE_PORTAUDIO
#include <portaudio.h>
#define PA_FRAMES_PER_BUFFER 64
//Buffer needs to be large enough to prevent input buffer overruns while DSD is doing other struff (like outputting voice)
//else you get skipped samples which result in incomplete/erronous decodes and a mountain of error messages.
#define PA_LATENCY_IN 0.500
//Buffer needs to be large enough to prevent output buffer underruns while DSD is doing other stuff (like decoding input)
//else you get choppy audio and in 'extreme' cases errors.
//Buffer also needs to be as small as possible so we don't have a lot of audio delay.
#define PA_LATENCY_OUT 0.100
#endif



/*
 * global variables
 */
extern int exitflag;


/*
 * Enum
 */
enum
{
  MODE_UNENCRYPTED,
  MODE_BASIC_PRIVACY,
  MODE_ENHANCED_PRIVACY_ARC4,
  MODE_ENHANCED_PRIVACY_AES128,
  MODE_ENHANCED_PRIVACY_AES256,
  MODE_HYTERA_BASIC_40_BIT,
  MODE_HYTERA_BASIC_128_BIT,
  MODE_HYTERA_BASIC_256_BIT,
  MODE_HYTERA_FULL_ENCRYPT
};


/*
 * typedef
 */

/* Read Solomon (12,9) constant */
#define RS_12_9_DATASIZE        9
#define RS_12_9_CHECKSUMSIZE    3

/* Read Solomon (12,9) struct */
typedef struct
{
  uint8_t data[RS_12_9_DATASIZE+RS_12_9_CHECKSUMSIZE];
} rs_12_9_codeword_t;

// Maximum degree of various polynomials.
#define RS_12_9_POLY_MAXDEG (RS_12_9_CHECKSUMSIZE*2)

typedef struct
{
  uint8_t data[RS_12_9_POLY_MAXDEG];
} rs_12_9_poly_t;

#define RS_12_9_CORRECT_ERRORS_RESULT_NO_ERRORS_FOUND           0
#define RS_12_9_CORRECT_ERRORS_RESULT_ERRORS_CORRECTED          1
#define RS_12_9_CORRECT_ERRORS_RESULT_ERRORS_CANT_BE_CORRECTED  2
typedef uint8_t rs_12_9_correct_errors_result_t;

typedef struct
{
  uint8_t bytes[3];
} rs_12_9_checksum_t;


typedef struct
{
  char VoiceFrame[3][72];  /* 3 x 72 bit of voice per time slot frame    */
  char Sync[48];           /* 48 bit of data or SYNC per time slot frame */
}TimeSlotRawVoiceFrame_t;


typedef struct
{
  char DeInterleavedVoiceSample[3][4][24];  /* 3 x (4 x 24) deinterleaved bit of voice per time slot frame */
}TimeSlotDeinterleavedVoiceFrame_t;


typedef struct
{
  int  errs1[3];  /* 3 x errors #1 computed when demodulate the AMBE voice bit of the frame */
  int  errs2[3];  /* 3 x errors #2 computed when demodulate the AMBE voice bit of the frame */
  char AmbeBit[3][49];  /* 3 x 49 bit of AMBE voice of the frame */
}TimeSlotAmbeVoiceFrame_t;


typedef struct
{
  unsigned int  ProtectFlag;
  unsigned int  Reserved;
  unsigned int  LastBlock;
  unsigned int  FullLinkControlOpcode;
  unsigned int  FullLinkCSBKOpcode;
  unsigned int  FeatureSetID;
  unsigned int  ServiceOptions;
  unsigned int  GroupAddress;      /* Destination ID or TG (in Air Interface format) */
  unsigned int  GroupAddressCSBK[6]; /* Destination ID or TG applicable for CSBK Cap+ frames */
  unsigned int  SourceAddress;     /* Source ID (in Air Interface format) */
  unsigned int  GPSReserved;       /* 4 bits GPS reserved field */
  unsigned int  GPSPositionError;  /* 3 bits GPS position error */
  unsigned int  GPSLongitude;
  unsigned int  GPSLatitude;
  unsigned int  TalkerAliasHeaderDataFormat;
  unsigned int  TalkerAliasHeaderDataLength;
  unsigned char TalkerAliasHeaderData[49];
  unsigned char TalkerAliasBlock1Data[56];
  unsigned char TalkerAliasBlock2Data[56];
  unsigned char TalkerAliasBlock3Data[56];
  unsigned char FullLinkData[96];  /* 72 bits of LC data + 5 bits CRC or 96 bits of LC data + 16 bits CRC */
  unsigned int  RestChannel;
  unsigned int  ActiveChannel1;
  unsigned int  ActiveChannel2;
  unsigned int  NeighborsSites[6];
  unsigned int  CurrentSite;
  unsigned int  DataValidity;      /* 0 = All Full LC data are incorrect ; 1 = Full LC data are correct (CRC checked OK) */
}FullLinkControlPDU_t;


typedef struct
{
  /* Data header data */
  char  DataHeaderBit[96];
  int   GroupDataCall;                  /* 1 = Group call ; 0 = Individual call */
  int   ResponseRequested;
  int   AppendedBlocks;
  int   DataPacketFormat;
  int   PadOctetCount;
  int   SAPIdentifier;
  int   DestinationLogicalLinkID;
  int   SourceLogicalLinkID;
  int   FullMessageFlag;
  int   BlocksToFollow;
  int   ReSyncronizeFlag;
  int   SendSequenceNumber;
  int   FragmentSequenceNumber;

  /* Specific to data header block for response packet */
  int   Class;
  int   Type;
  int   Status;

  /* Specific to proprietary header */
  int   MFID;
  char  ProprietaryData[8];

  /* Specific to status/precoded short data header or raw short data packet header */
  int   SourcePort;
  int   DestinationPort;
  int   StatusPrecoded;                   /* Available only on status/precoded short data header */
  int   SelecriveAutomaticRepeatReQuest;  /* Available only on raw short data packet header */
  int   BitPadding;

  /* Specific to defined short data packet header */
  int   DefinedData;

  /* Specific to Unified Data Transport (UDT) data header */
  int   UDTFormat;
  int   PadNibble;
  int   SupplementaryFlag;
  int   ProtectFlag;
  int   UDTOpcode;

  /* Rate 1/2 data */
  int   Rate12NbOfReceivedBlock;
  char  Rate12DataBit[12192];                      /* May carry up to 127 blocks of 96 bits (total 1524 bytes) of rate 1/2 data */
  short Rate12DataWordBigEndian[762];              /* May carry up to 127 blocks of 96 bits (total 762 word) of rate 1/2 data (stored into 16 bit big endian format) */
  char  Rate12DataBitBigEndianUnconfirmed[12192];  /* May carry up to 127 blocks of 96 bits (total 1500 bytes) of unconfirmed rate 1/2 data (stored as 16 bit big endian format) */
  char  Rate12DataBitBigEndianConfirmed[10160];    /* May carry up to 127 blocks of 80 bits (total 1250 bytes) of confirmed rate 1/2 data (stored as 16 bit big endian format) */

  /* Rate 3/4 data */
  int   Rate34NbOfReceivedBlock;
  char  Rate34DataBit[18288];                      /* May carry up to 127 blocks of 144 bits (total 2286 bytes) of rate 3/4 data */
  short Rate34DataWordBigEndian[1143];             /* May carry up to 127 blocks of 144 bits (total 1143 word) of rate 3/4 data (stored into 16 bit big endian format) */
  char  Rate34DataBitBigEndianUnconfirmed[18288];  /* May carry up to 127 blocks of 144 bits (total 2286 bytes) of unconfirmed rate 3/4 data (stored as 16 bit big endian format) */
  char  Rate34DataBitBigEndianConfirmed[16256];    /* May carry up to 127 blocks of 128 bits (total 2032 bytes) of confirmed rate 3/4 data (stored as 16 bit big endian format) */

  /* Rate 1 data */
  int   Rate1NbOfReceivedBlock;
  char  Rate1DataBit[24384];                      /* May carry up to 127 blocks of 192 bits (total 3048 bytes) of rate 1 data */
  short Rate1DataWordBigEndian[1524];             /* May carry up to 127 blocks of 192 bits (total 1524 word) of rate 1 data (stored into 16 bit big endian format) */
  char  Rate1DataBitBigEndianUnconfirmed[24384];  /* May carry up to 127 blocks of 192 bits (total 3048 bytes) of unconfirmed rate 1 data (stored as 16 bit big endian format) */
  char  Rate1DataBitBigEndianConfirmed[22352];    /* May carry up to 127 blocks of 192 bits (total 2794 bytes) of confirmed rate 1 data (stored as 16 bit big endian format) */

} DMRDataPDU_t;


typedef struct
{
  TimeSlotRawVoiceFrame_t TimeSlotRawVoiceFrame[6]; /* A voice superframe contains 6 timeslot voice frame */
  TimeSlotDeinterleavedVoiceFrame_t TimeSlotDeinterleavedVoiceFrame[6];
  TimeSlotAmbeVoiceFrame_t TimeSlotAmbeVoiceFrame[6];
  FullLinkControlPDU_t FullLC;
  DMRDataPDU_t Data;
} TimeSlotVoiceSuperFrame_t;


typedef struct
{
  unsigned char RawVoiceBit[NB_OF_DPMR_VOICE_FRAME_TO_DECODE * 4][72];
  unsigned int  errs1[NB_OF_DPMR_VOICE_FRAME_TO_DECODE * 4];  /* 8 x errors #1 computed when demodulate the AMBE voice bit of the frame */
  unsigned int  errs2[NB_OF_DPMR_VOICE_FRAME_TO_DECODE * 4];  /* 8 x errors #2 computed when demodulate the AMBE voice bit of the frame */
  unsigned char AmbeBit[NB_OF_DPMR_VOICE_FRAME_TO_DECODE * 4][49];  /* 8 x 49 bit of AMBE voice of the frame */
  unsigned char CCHData[NB_OF_DPMR_VOICE_FRAME_TO_DECODE][48];
  unsigned int  CCHDataHammingOk[NB_OF_DPMR_VOICE_FRAME_TO_DECODE];
  unsigned char CCHDataCRC[NB_OF_DPMR_VOICE_FRAME_TO_DECODE];
  unsigned int  CCHDataCrcOk[NB_OF_DPMR_VOICE_FRAME_TO_DECODE];
  unsigned char CalledID[8];
  unsigned int  CalledIDOk;
  unsigned char CallingID[8];
  unsigned int  CallingIDOk;
  unsigned int  FrameNumbering[NB_OF_DPMR_VOICE_FRAME_TO_DECODE];
  unsigned int  CommunicationMode[NB_OF_DPMR_VOICE_FRAME_TO_DECODE];
  unsigned int  Version[NB_OF_DPMR_VOICE_FRAME_TO_DECODE];
  unsigned int  CommsFormat[NB_OF_DPMR_VOICE_FRAME_TO_DECODE];
  unsigned int  EmergencyPriority[NB_OF_DPMR_VOICE_FRAME_TO_DECODE];
  unsigned int  Reserved[NB_OF_DPMR_VOICE_FRAME_TO_DECODE];
  unsigned char SlowData[NB_OF_DPMR_VOICE_FRAME_TO_DECODE];
  unsigned int  ColorCode[NB_OF_DPMR_VOICE_FRAME_TO_DECODE / 2];
} dPMRVoiceFS2Frame_t;


typedef struct __attribute__((packed, aligned(1)))
{
  char  FileTypeBlockID[4];  /* "RIFF" constant (0x52,0x49,0x46,0x46) */
  int   FileSize;            /* Wave file size in bytes minus 8 bytes */
  char  FileFormatID[4];     /* Format "WAVE" (0x57,0x41,0x56,0x45) */

  /* Block describing the audio format */
  char  FormatBlockID[4];    /* "fmt" constant 0x66,0x6D, 0x74,0x20) */
  int   BlockSize;           /* Number of byte of block : here 16 bytes (0x10) */
  short AudioFormat;         /* 1 = PCM */
  short NbCanaux;            /* Mono = 1 ; Stereo = 2 ; may be up to 6 */
  int   Frequency;           /* 11025, 22050, 44100, 48000 or 96000 */
  int   BytePerSec;          /* Frequency * BytePerBlock */
  short BytePerBlock;        /* NbCanaux * BitsPerSample / 8 */
  short BitsPerSample;       /* Number of bit per audio sample : 8, 16 or 24 bits */

  /* Data block */
  char  DataBlockID[4];      /* "data" constant (0x64,0x61,0x74,0x61) */
  int   DataSize;            /* Number of audio sample in bytes */
} wav_t;


typedef struct
{
  uint8_t RFChannelType;
  uint8_t FunctionnalChannelType;
  uint8_t Option;
  uint8_t Direction;
  uint8_t CompleteLich; /* 7 bits of LICH without parity */
  uint8_t PreviousCompleteLich; /* 7 bits of previous LICH without parity */
} NxdnLich_t;


typedef struct
{
  uint8_t StructureField;
  uint8_t RAN;
  uint8_t Data[18];
  uint8_t CrcIsGood;
} NxdnSacchRawPart_t;


typedef struct
{
  uint8_t Data[80];
  uint8_t CrcIsGood;
} NxdnFacch1RawPart_t;


typedef struct
{
  uint8_t Data[184];
  uint8_t CrcIsGood;
} NxdnFacch2RawPart_t;


typedef struct
{
  uint8_t  F1;
  uint8_t  F2;
  uint8_t  MessageType;

  /****************************/
  /***** VCALL parameters *****/
  /****************************/
  uint8_t  CCOption;
  uint8_t  CallType;
  uint8_t  VoiceCallOption;
  uint16_t SourceUnitID;
  uint16_t DestinationID;  /* May be a Group ID or a Unit ID */
  uint8_t  CipherType;
  uint8_t  KeyID;
  uint8_t  VCallCrcIsGood;

  /*******************************/
  /***** VCALL_IV parameters *****/
  /*******************************/
  uint8_t  IV[8];
  uint8_t  VCallIvCrcIsGood;

  /*****************************/
  /***** Custom parameters *****/
  /*****************************/

  /* Specifies if the "CipherType" and the "KeyID" parameter are valid
   * 1 = Valid ; 0 = CRC error */
  uint8_t  CipherParameterValidity;

  /* Used on DES and AES encrypted frames */
  uint8_t  PartOfCurrentEncryptedFrame;  /* Could be 1 or 2 */
  uint8_t  PartOfNextEncryptedFrame;     /* Could be 1 or 2 */
  uint8_t  CurrentIVComputed[8];
  uint8_t  NextIVComputed[8];
} NxdnElementsContent_t;


typedef struct
{
  int onesymbol;
  char mbe_in_file[1024];
  FILE *mbe_in_f;
  int errorbars;
  int datascope;
  int symboltiming;
  int verbose;
  int p25enc;
  int p25lc;
  int p25status;
  int p25tg;
  int scoperate;
  char audio_in_dev[1024];
  int audio_in_fd;
  SNDFILE *audio_in_file;
  SF_INFO *audio_in_file_info;
#ifdef USE_PORTAUDIO
  PaStream* audio_in_pa_stream;
#endif
  int audio_in_type; // 0 for device, 1 for file, 2 for portaudio
  char audio_out_dev[1024];
  int audio_out_fd;
  SNDFILE *audio_out_file;
  SF_INFO *audio_out_file_info;
#ifdef USE_PORTAUDIO
  PaStream* audio_out_pa_stream;
#endif
  int audio_out_type; // 0 for device, 1 for file, 2 for portaudio
  int split;
  int playoffset;
  char mbe_out_dir[1024];
  char mbe_out_file[1024];
  char mbe_out_path[1024];
  FILE *mbe_out_f;
  float audio_gain;
  int audio_out;
  char wav_out_file[1024];
  SNDFILE *wav_out_f;
  //int wav_out_fd;
  int serial_baud;
  char serial_dev[1024];
  int serial_fd;
  int resume;
  int frame_dstar;
  int frame_x2tdma;
  int frame_p25p1;
  int frame_nxdn48;
  int frame_nxdn96;
  int frame_dmr;
  int frame_provoice;
  int frame_dpmr;
  int mod_c4fm;
  int mod_qpsk;
  int mod_gfsk;
  int uvquality;
  int inverted_x2tdma;
  int inverted_dmr;
  int inverted_dpmr;
  int mod_threshold;
  int ssize;
  int msize;
  int playfiles;
  int delay;
  int use_cosine_filter;
  int unmute_encrypted_p25;

  int output_channel;       /* 0 = mono/default, 1-N = output on specific channel (1-indexed) */
  int output_num_channels;  /* total output channels, 0 = auto-detect from device */
  short *mc_out_buf;        /* persistent multi-channel interleave buffer (heap allocated) */
  int mc_out_buf_size;      /* size of mc_out_buf in samples */

  unsigned int dPMR_curr_frame_is_encrypted;
  int dPMR_next_part_of_superframe;

  int EncryptionMode;


  int  SlotToOnlyDecode;  /* 1 = Decode only slot 1 ; 0 = Decode only slot 2 */

  char UseSpecificDmr49BitsAmbeKeyStream[49]; /* 49 bits of AMBE voice samples */
  int  UseSpecificDmr49BitsAmbeKeyStreamUsed;
  char UseSpecificDmrAmbeSuperFrameKeyStream[49 * 18]; /* 49 bits of AMBE voice sample x 18 DMR frames = 882 bits */
  int  UseSpecificDmrAmbeSuperFrameKeyStreamUsed;

  char UseSpecificNxdn49BitsAmbeKeyStream[49]; /* 49 bits of AMBE voice samples */
  int  UseSpecificNxdn49BitsAmbeKeyStreamUsed;
  char UseSpecificNxdnAmbeSuperFrameKeyStream[49 * 16]; /* 49 bits of AMBE voice sample x 16 NXDN frames = 784 bits */
  int  UseSpecificNxdnAmbeSuperFrameKeyStreamUsed;

  char UseSpecificdPMR49BitsAmbeKeyStream[49]; /* 49 bits of AMBE voice samples */
  int  UseSpecificdPMR49BitsAmbeKeyStreamUsed;
  char UseSpecificdPMRAmbeSuperFrameKeyStream[49 * 16]; /* 49 bits of AMBE voice sample x 16 NXDN frames = 784 bits */
  int  UseSpecificdPMRAmbeSuperFrameKeyStreamUsed;
} dsd_opts;

typedef struct
{
  int *dibit_buf;
  int *dibit_buf_p;
  int repeat;
  short *audio_out_buf;
  short *audio_out_buf_p;
  float *audio_out_float_buf;
  float *audio_out_float_buf_p;
  float audio_out_temp_buf[160];
  float *audio_out_temp_buf_p;
  int audio_out_idx;
  int audio_out_idx2;
  //int wav_out_bytes;
  int center;
  int jitter;
  int synctype;
  int min;
  int max;
  int lmid;
  int umid;
  int minref;
  int maxref;
  int lastsample;
  int sbuf[128];
  int sidx;
  int maxbuf[1024];
  int minbuf[1024];
  int midx;
  char err_str[64];
  char fsubtype[16];
  char ftype[16];
  unsigned int color_code;
  unsigned int color_code_ok;
  unsigned int PI;
  unsigned int PI_ok;
  unsigned int LCSS;
  unsigned int LCSS_ok;
  int symbolcnt;
  int rf_mod; /* 0=C4FM 1=QPSK 2=GFSK */
  int numflips;
  int lastsynctype;
  int lastp25type;
  int offset;
  int carrier;
  char tg[25][16];
  int tgcount;
  int lasttg;
  int lastsrc;
  int nac;
  int errs;
  int errs2;
  char ambe_ciphered[49];
  char ambe_deciphered[49];
  int mbe_file_type;
  int optind;
  int numtdulc;
  int firstframe;
  char slot1light[8];
  char slot2light[8];
  float aout_gain;
  float aout_max_buf[200];
  float *aout_max_buf_p;
  int aout_max_buf_idx;
  int samplesPerSymbol;
  int symbolCenter;     /* DMR=4 - NXDN96=2 - dPMR/NXDN48=10 */
  char algid[9];
  char keyid[17];
  int currentslot; /* 0 = Slot 1 ; 1 = Slot 2 */
  int directmode;
  mbe_parms *cur_mp;
  mbe_parms *prev_mp;
  mbe_parms *prev_mp_enhanced;
  int p25kid;

  unsigned int debug_audio_errors;
  unsigned int debug_header_errors;
  unsigned int debug_header_critical_errors;

  // Last dibit read
  int last_dibit;

  // Heuristics state data for +P5 signals
  P25Heuristics p25_heuristics;

  // Heuristics state data for -P5 signals
  P25Heuristics inv_p25_heuristics;

#ifdef TRACE_DSD
  char debug_prefix;
  char debug_prefix_2;
  unsigned int debug_sample_index;
  unsigned int debug_sample_left_edge;
  unsigned int debug_sample_right_edge;
  FILE* debug_label_file;
  FILE* debug_label_dibit_file;
  FILE* debug_label_imbe_file;
#endif

  int TimeYear;
  int TimeMonth;
  int TimeDay;
  int TimeHour;
  int TimeMinute;
  int TimeSecond;

  TimeSlotVoiceSuperFrame_t TS1SuperFrame;
  TimeSlotVoiceSuperFrame_t TS2SuperFrame;

  unsigned int CapacityPlusFlag;

  dPMRVoiceFS2Frame_t dPMRVoiceFS2Frame;

  /* These flags are used to known the DMR frame
   * printing format (hex or binary) */
  int printDMRRawVoiceFrameHex;
  int printDMRRawVoiceFrameBin;
  int printDMRRawDataFrameHex;
  int printDMRRawDataFrameBin;
  int printDMRAmbeVoiceSampleHex;
  int printDMRAmbeVoiceSampleBin;

  /* These flags are used to known the dPMR frame
   * printing format (hex or binary) */
  int printdPMRRawVoiceFrameHex;
  int printdPMRRawVoiceFrameBin;
  int printdPMRRawDataFrameHex;
  int printdPMRRawDataFrameBin;
  int printdPMRAmbeVoiceSampleHex;
  int printdPMRAmbeVoiceSampleBin;

  int special_display_format_enable;  // Format used to decrypt EP keys
  int display_raw_data;  // Data displayed unencrypted

  NxdnSacchRawPart_t  NxdnSacchRawPart[4];
  NxdnFacch1RawPart_t NxdnFacch1RawPart[2];
  NxdnFacch2RawPart_t NxdnFacch2RawPart;
  NxdnElementsContent_t NxdnElementsContent;
  NxdnLich_t NxdnLich;

  int printNXDNAmbeVoiceSampleHex;
} dsd_state;


/* Only for debug
 * Used to find frame sync pattern used on
 * Icom IDAS NXDN 4800 baud system
 */
#define SIZE_OF_SYNC_PATTERN_TO_SEARCH  18


/*
 * Frame sync patterns
 */
#define INV_P25P1_SYNC "333331331133111131311111"
#define P25P1_SYNC     "111113113311333313133333"

#define X2TDMA_BS_VOICE_SYNC "113131333331313331113311"
#define X2TDMA_BS_DATA_SYNC  "331313111113131113331133"
#define X2TDMA_MS_DATA_SYNC  "313113333111111133333313"
#define X2TDMA_MS_VOICE_SYNC "131331111333333311111131"

#define DSTAR_HD       "131313131333133113131111"
#define INV_DSTAR_HD   "313131313111311331313333"
#define DSTAR_SYNC     "313131313133131113313111"
#define INV_DSTAR_SYNC "131313131311313331131333"

#define NXDN_MS_DATA_SYNC         "313133113131111333"
#define INV_NXDN_MS_DATA_SYNC     "131311331313333111"
#define NXDN_MS_VOICE_SYNC        "313133113131113133"
#define INV_NXDN_MS_VOICE_SYNC    "131311331313331311"
#define INV_NXDN_BS_DATA_SYNC     "131311331313333131"
#define NXDN_BS_DATA_SYNC         "313133113131111313"
#define INV_NXDN_BS_VOICE_SYNC    "131311331313331331"
#define NXDN_BS_VOICE_SYNC        "313133113131113113"
#define NXDN_SYNC                 "3131331131"  // TODO : Only for test
#define INV_NXDN_SYNC             "1313113313"  // TODO : Only for test
#define NXDN_SYNC2                "1313113331"  // TODO : Only for test
#define INV_NXDN_SYNC2            "3131331113"  // TODO : Only for test
#define NXDN_SYNC3                "3313113331"  // TODO : Only for test
#define INV_NXDN_SYNC3            "1131331113"  // TODO : Only for test

#define NXDN_DOUBLE_SYNC                        "31313311313131331131"  // TODO : Only for test
#define NXDN_PREAMBLE_POST_FIELD_AND_FSW_SYNC   "3131133313131331131"   // TODO : Only for test


/* DMR Frame Sync Pattern - Base Station Sourced - Data
 * HEX    :    D    F    F    5    7    D    7    5    D    F    5    D
 * Binary : 1101 1111 1111 0101 0111 1101 0111 0101 1101 1111 0101 1101
 * Dibit  :  3 1  3 3  3 3  1 1  1 3  3 1  1 3  1 1  3 1  3 3  1 1  3 1 */
#define DMR_BS_DATA_SYNC  "313333111331131131331131"

/* DMR Frame Sync Pattern - Base Station Sourced - Voice
 * HEX    :    7    5    5    F    D    7    D    F    7    5    F    7
 * Binary : 0111 0101 0101 1111 1101 0111 1101 1111 0111 0101 1111 0111
 * Dibit  :  1 3  1 1  1 1  3 3  3 1  1 3  3 1  3 3  1 3  1 1  3 3  1 3 */
#define DMR_BS_VOICE_SYNC "131111333113313313113313"

/* DMR Frame Sync Pattern - Mobile Station Sourced - Data
 * HEX    :    D    5    D    7    F    7    7    F    D    7    5    7
 * Binary : 1101 0101 1101 0111 1111 0111 0111 1111 1101 0111 0101 0111
 * Dibit  :  3 1  1 1  3 1  1 3  3 3  1 3  1 3  3 3  3 1  1 3  1 1  1 3 */
#define DMR_MS_DATA_SYNC  "311131133313133331131113"

/* DMR Frame Sync Pattern - Mobile Station Sourced - Voice
 * HEX    :    7    F    7    D    5    D    D    5    7    D    F    D
 * Binary : 0111 1111 0111 1101 0101 1101 1101 0101 0111 1101 1111 1101
 * Dibit  :  1 3  3 3  1 3  3 1  1 1  3 1  3 1  1 1  1 3  3 1  3 3  3 1 */
#define DMR_MS_VOICE_SYNC "133313311131311113313331"

/* DMR Frame Sync Pattern - TDMA direct mode time slot 1 - Data
 * HEX    :    F    7    F    D    D    5    D    D    F    D    5    5
 * Binary : 1111 0111 1111 1101 1101 0101 1101 1101 1111 1101 0101 0101
 * Dibit  :  3 3  1 3  3 3  3 1  3 1  1 1  3 1  3 1  3 3  3 1  1 1  1 1 */
#define DMR_DIRECT_MODE_TS1_DATA_SYNC  "331333313111313133311111"

/* DMR Frame Sync Pattern - TDMA direct mode time slot 1 - Voice
 * HEX    :    5    D    5    7    7    F    7    7    5    7    F    F
 * Binary : 0101 1101 0101 0111 0111 1111 0111 0111 0101 0111 1111 1111
 * Dibit  :  1 1  3 1  1 1  1 3  1 3  3 3  1 3  1 3  1 1  1 3  3 3  3 3 */
#define DMR_DIRECT_MODE_TS1_VOICE_SYNC "113111131333131311133333"

/* DMR Frame Sync Pattern - TDMA direct mode time slot 2 - Data
 * HEX    :    D    7    5    5    7    F    5    F    F    7    F    5
 * Binary : 1101 0111 0101 0101 0111 1111 0101 1111 1111 0111 1111 0101
 * Dibit  :  3 1  1 3  1 1  1 1  1 3  3 3  1 1  3 3  3 3  1 3  3 3  1 1 */
#define DMR_DIRECT_MODE_TS2_DATA_SYNC  "311311111333113333133311"

/* DMR Frame Sync Pattern - TDMA direct mode time slot 2 - Voice
 * HEX    :    7    D    F    F    D    5    F    5    5    D    5    F
 * Binary : 0111 1101 1111 1111 1101 0101 1111 0101 0101 1101 0101 1111
 * Dibit  :  1 3  3 1  3 3  3 3  3 1  1 1  3 3  1 1  1 1  3 1  1 1  3 3 */
#define DMR_DIRECT_MODE_TS2_VOICE_SYNC "133133333111331111311133"




#define INV_PROVOICE_SYNC    "31313111333133133311331133113311"
#define PROVOICE_SYNC        "13131333111311311133113311331133"
#define INV_PROVOICE_EA_SYNC "13313133113113333311313133133311"
#define PROVOICE_EA_SYNC     "31131311331331111133131311311133"

/* dPMR Frame Sync 1 - 48 bits sequence
 * HEX    : 57 FF 5F 75 D5 77
 * Binary : 0101 0111 1111 1111 0101 1111 0111 0101 1101 0101 0111 0111
 * Dibit  :  1 1  1 3  3 3  3 3  1 1  3 3  1 3  1 1  3 1  1 1  1 3  1 3 */
#define DPMR_FRAME_SYNC_1     "111333331133131131111313"

/* dPMR Frame Sync 2 - 24 bits sequence
 * HEX    : 5F F7 7D
 * Binary : 0101 1111 1111 0111 0111 1101
 * Dibit  :  1 1  3 3  3 3  1 3  1 3  3 1 */
#define DPMR_FRAME_SYNC_2     "113333131331"

/* dPMR Frame Sync 3 - 24 bits sequence
 * HEX    : 7D DF F5
 * Binary : 0111 1101 1101 1111 1111 0101
 * Dibit  :  1 3  3 1  3 1  3 3  3 3  1 1 */
#define DPMR_FRAME_SYNC_3     "133131333311"

/* dPMR Frame Sync 4 - 48 bits sequence
 * HEX    : FD 55 F5 DF 7F DD
 * Binary : 1111 1101 0101 0101 1111 0101 1101 1111 0111 1111 1101 1101
 * Dibit  :  3 3  3 1  1 1  1 1  3 3  1 1  3 1  3 3  1 3  3 3  3 1  3 1 */
#define DPMR_FRAME_SYNC_4     "333111113311313313333131"

/* dPMR Frame Sync 1 to 4 - Inverted */
#define INV_DPMR_FRAME_SYNC_1 "333111113311313313333131"
#define INV_DPMR_FRAME_SYNC_2 "331111313113"
#define INV_DPMR_FRAME_SYNC_3 "311313111133"
#define INV_DPMR_FRAME_SYNC_4 "111333331133131131111313"




/* Sample per symbol constants */
#define SAMPLE_PER_SYMBOL_DMR     (SAMPLE_RATE_IN / 4800)
#define SAMPLE_PER_SYMBOL_P25     SAMPLE_PER_SYMBOL_DMR
#define SAMPLE_PER_SYMBOL_NXDN48  (SAMPLE_RATE_IN / 2400)
#define SAMPLE_PER_SYMBOL_NXDN96  (SAMPLE_RATE_IN / 9600)
#define SAMPLE_PER_SYMBOL_DPMR    SAMPLE_PER_SYMBOL_NXDN48


enum
{
  C4FM_MODE = 0,
  QPSK_MODE = 1,
  GFSK_MODE = 2,
  UNKNOWN_MODE
};


/*
 * function prototypes
 */
void processDMRdata (dsd_opts * opts, dsd_state * state);
void processDMRvoice (dsd_opts * opts, dsd_state * state);
void processdPMRvoice (dsd_opts * opts, dsd_state * state);
void processAudio (dsd_opts * opts, dsd_state * state);
void writeSynthesizedVoice (dsd_opts * opts, dsd_state * state);
void playSynthesizedVoice (dsd_opts * opts, dsd_state * state);
void openAudioOutDevice (dsd_opts * opts, int speed);
void openAudioInDevice (dsd_opts * opts);
void getCurrentTime (dsd_opts * opts, dsd_state * state);

int getDibit (dsd_opts * opts, dsd_state * state);
int get_dibit_and_analog_signal (dsd_opts * opts, dsd_state * state, int * out_analog_signal);

void skipDibit (dsd_opts * opts, dsd_state * state, int count);
void saveImbe4400Data (dsd_opts * opts, dsd_state * state, char *imbe_d);
void saveAmbe2450Data (dsd_opts * opts, dsd_state * state, char *ambe_d);
int readImbe4400Data (dsd_opts * opts, dsd_state * state, char *imbe_d);
int readAmbe2450Data (dsd_opts * opts, dsd_state * state, char *ambe_d);
void openMbeInFile (dsd_opts * opts, dsd_state * state);
void closeMbeOutFile (dsd_opts * opts, dsd_state * state);
void openMbeOutFile (dsd_opts * opts, dsd_state * state);
void openWavOutFile (dsd_opts * opts, dsd_state * state);
void closeWavOutFile (dsd_opts * opts, dsd_state * state);
void printFrameInfo (dsd_opts * opts, dsd_state * state);
void processFrame (dsd_opts * opts, dsd_state * state);
void printFrameSync (dsd_opts * opts, dsd_state * state, char *frametype, int offset, char *modulation);
int getFrameSync (dsd_opts * opts, dsd_state * state);
int comp (const void *a, const void *b);
void noCarrier (dsd_opts * opts, dsd_state * state);
void initOpts (dsd_opts * opts);
void initState (dsd_state * state);
void usage ();
void liveScanner (dsd_opts * opts, dsd_state * state);
void freeAllocatedMemory(dsd_opts * opts, dsd_state * state);
void cleanupAndExit (dsd_opts * opts, dsd_state * state);
void sigfun (int sig);
int main (int argc, char **argv);
void playMbeFiles (dsd_opts * opts, dsd_state * state, int argc, char **argv);
void processMbeFrame (dsd_opts * opts, dsd_state * state, char imbe_fr[8][23], char ambe_fr[4][24], char imbe7100_fr[7][24]);
void processMbeFrameEncrypted (dsd_opts * opts, dsd_state * state, char imbe_fr[8][23], char ambe_fr[4][24], char imbe7100_fr[7][24], char ambe_keystream[49], char imbe_keystream[88]);
void openSerial (dsd_opts * opts, dsd_state * state);
void resumeScan (dsd_opts * opts, dsd_state * state);
int getSymbol (dsd_opts * opts, dsd_state * state, int have_sync);
void upsample (dsd_state * state, float invalue);
void processDSTAR (dsd_opts * opts, dsd_state * state);
void processNXDNVoice (dsd_opts * opts, dsd_state * state);
void processNXDNData (dsd_opts * opts, dsd_state * state);
void processP25lcw (dsd_opts * opts, dsd_state * state, char *lcformat, char *mfid, char *lcinfo);
void processHDU (dsd_opts * opts, dsd_state * state);
void processLDU1 (dsd_opts * opts, dsd_state * state);
void processLDU2 (dsd_opts * opts, dsd_state * state);
void processTDU (dsd_opts * opts, dsd_state * state);
void processTDULC (dsd_opts * opts, dsd_state * state);
void processProVoice (dsd_opts * opts, dsd_state * state);
void processX2TDMAdata (dsd_opts * opts, dsd_state * state);
void processX2TDMAvoice (dsd_opts * opts, dsd_state * state);
void processDSTAR_HD (dsd_opts * opts, dsd_state * state);
short dmr_filter(short sample);
short nxdn_filter(short sample);

/* DMR print frame functions */
void printDMRRawVoiceFrame (dsd_opts * opts, dsd_state * state);
void printDMRRawDataFrame (dsd_opts * opts, dsd_state * state);
void printExtractedDMRDataFrame(dsd_opts * opts, dsd_state * state);
void printDMRAmbeVoiceSample(dsd_opts * opts, dsd_state * state);

/* dPMR print frame function */
void printdPMRAmbeVoiceSample(dsd_opts * opts, dsd_state * state);
void printdPMRRawVoiceFrame (dsd_opts * opts, dsd_state * state);

void DMRDataFrameProcess(dsd_opts * opts, dsd_state * state);
void DMRVoiceFrameProcess(dsd_opts * opts, dsd_state * state);
void dPMRVoiceFrameProcess(dsd_opts * opts, dsd_state * state);

void DisplaySpecialFormat(dsd_opts * opts, dsd_state * state);


/* dPMR functions */
void ScrambledPMRBit(uint32_t * LfsrValue, uint8_t * BufferIn, uint8_t * BufferOut, uint32_t NbOfBitToScramble);
void DeInterleave6x12DPmrBit(uint8_t * BufferIn, uint8_t * BufferOut);
uint8_t CRC7BitdPMR(uint8_t * BufferIn, uint32_t BitLength);
uint8_t CRC8BitdPMR(uint8_t * BufferIn, uint32_t BitLength);
void ConvertAirInterfaceID(uint32_t AI_ID, uint8_t ID[8]);
int32_t GetdPmrColorCode(uint8_t ChannelCodeBit[24]);



/* NXDN frame decoding functions */
void ProcessNXDNFrame(dsd_opts * opts, dsd_state * state, uint8_t Inverted);
void ProcessNxdnRCCHFrame(dsd_opts * opts, dsd_state * state, uint8_t Inverted);
void ProcessNxdnRTCHFrame(dsd_opts * opts, dsd_state * state, uint8_t Inverted);
void ProcessNxdnRDCHFrame(dsd_opts * opts, dsd_state * state, uint8_t Inverted);
void ProcessNxdnRTCH_C_Frame(dsd_opts * opts, dsd_state * state, uint8_t Inverted);
void ProcessNXDNIdleData (dsd_opts * opts, dsd_state * state, uint8_t Inverted);
void ProcessNXDNFacch1Data (dsd_opts * opts, dsd_state * state, uint8_t Inverted);
void ProcessNXDNUdchData (dsd_opts * opts, dsd_state * state, uint8_t Inverted);


/* NXDN functions */
void CNXDNConvolution_start(void);
void CNXDNConvolution_decode(uint8_t s0, uint8_t s1);
void CNXDNConvolution_chainback(unsigned char* out, unsigned int nBits);
void CNXDNConvolution_encode(const unsigned char* in, unsigned char* out, unsigned int nBits);
uint8_t NXDN_decode_LICH(uint8_t   InputLich[8],
                         uint8_t * RFChannelType,
                         uint8_t * FunctionnalChannelType,
                         uint8_t * Option,
                         uint8_t * Direction,
                         uint8_t * CompleteLichBinaryFormat,
                         uint8_t   Inverted);
uint8_t NXDN_SACCH_raw_part_decode(uint8_t * Input, uint8_t * Output);
void NXDN_SACCH_Full_decode(dsd_opts * opts, dsd_state * state);
uint8_t NXDN_FACCH1_decode(uint8_t * Input, uint8_t * Output);
uint8_t NXDN_UDCH_decode(uint8_t * Input, uint8_t * Output);
void NXDN_Elements_Content_decode(dsd_opts * opts, dsd_state * state,
                                  uint8_t CrcCorrect, uint8_t * ElementsContent);
void NXDN_decode_VCALL(dsd_opts * opts, dsd_state * state, uint8_t * Message);
void NXDN_decode_VCALL_IV(dsd_opts * opts, dsd_state * state, uint8_t * Message);
char * NXDN_Call_Type_To_Str(uint8_t CallType);
void NXDN_Voice_Call_Option_To_Str(uint8_t VoiceCallOption, uint8_t * Duplex, uint8_t * TransmissionMode);
char * NXDN_Cipher_Type_To_Str(uint8_t CipherType);
uint16_t CRC16BitNXDN(uint8_t * BufferIn, uint32_t BitLength);
uint16_t CRC15BitNXDN(uint8_t * BufferIn, uint32_t BitLength);
uint16_t CRC12BitNXDN(uint8_t * BufferIn, uint32_t BitLength);
uint8_t CRC6BitNXDN(uint8_t * BufferIn, uint32_t BitLength);
void ScrambledNXDNVoiceBit(int * LfsrValue, char * BufferIn, char * BufferOut, int NbOfBitToScramble);
void NxdnEncryptionStreamGeneration (dsd_opts* opts, dsd_state* state, uint8_t KeyStream[1664]);



/* DMR encryption functions */
void ProcessDMREncryption (dsd_opts * opts, dsd_state * state);


/* Global functions */
uint32_t ConvertBitIntoBytes(uint8_t * BufferIn, uint32_t BitLength);
uint32_t ConvertAsciiToByte(uint8_t AsciiMsbByte, uint8_t AsciiLsbByte, uint8_t * OutputByte);
void Convert49BitSampleInto7Bytes(char * InputBit, char * OutputByte);
void Convert7BytesInto49BitSample(char * InputByte, char * OutputBit);
int strncmperr(const char *s1, const char *s2, size_t size, int MaxErr);


/* FEC correction */
extern unsigned char Hamming_7_4_m_corr[8];             //!< single bit error correction by syndrome index
extern const unsigned char Hamming_7_4_m_G[7*4]; //!< Generator matrix of bits
extern const unsigned char Hamming_7_4_m_H[7*3]; //!< Parity check matrix of bits

extern unsigned char Hamming_12_8_m_corr[16];             //!< single bit error correction by syndrome index
extern const unsigned char Hamming_12_8_m_G[12*8]; //!< Generator matrix of bits
extern const unsigned char Hamming_12_8_m_H[12*4]; //!< Parity check matrix of bits

extern unsigned char Hamming_13_9_m_corr[16];             //!< single bit error correction by syndrome index
extern const unsigned char Hamming_13_9_m_G[13*9]; //!< Generator matrix of bits
extern const unsigned char Hamming_13_9_m_H[13*4]; //!< Parity check matrix of bits

extern unsigned char Hamming_15_11_m_corr[16];              //!< single bit error correction by syndrome index
extern const unsigned char Hamming_15_11_m_G[15*11]; //!< Generator matrix of bits
extern const unsigned char Hamming_15_11_m_H[15*4];  //!< Parity check matrix of bits

extern unsigned char Hamming_16_11_4_m_corr[32];              //!< single bit error correction by syndrome index
extern const unsigned char Hamming_16_11_4_m_G[16*11]; //!< Generator matrix of bits
extern const unsigned char Hamming_16_11_4_m_H[16*5];  //!< Parity check matrix of bits

extern unsigned char Golay_20_8_m_corr[4096][3];         //!< up to 3 bit error correction by syndrome index
extern const unsigned char Golay_20_8_m_G[20*8];  //!< Generator matrix of bits
extern const unsigned char Golay_20_8_m_H[20*12]; //!< Parity check matrix of bits

extern unsigned char Golay_23_12_m_corr[2048][3];         //!< up to 3 bit error correction by syndrome index
extern const unsigned char Golay_23_12_m_G[23*12]; //!< Generator matrix of bits
extern const unsigned char Golay_23_12_m_H[23*11]; //!< Parity check matrix of bits

extern unsigned char Golay_24_12_m_corr[4096][3];         //!< up to 3 bit error correction by syndrome index
extern const unsigned char Golay_24_12_m_G[24*12]; //!< Generator matrix of bits
extern const unsigned char Golay_24_12_m_H[24*12]; //!< Parity check matrix of bits

extern unsigned char QR_16_7_6_m_corr[512][2];          //!< up to 2 bit error correction by syndrome index
extern const unsigned char QR_16_7_6_m_G[16*7];  //!< Generator matrix of bits
extern const unsigned char QR_16_7_6_m_H[16*9];  //!< Parity check matrix of bits


void Hamming_7_4_init();
void Hamming_7_4_encode(unsigned char *origBits, unsigned char *encodedBits);
bool Hamming_7_4_decode(unsigned char *rxBits);

void Hamming_12_8_init();
void Hamming_12_8_encode(unsigned char *origBits, unsigned char *encodedBits);
bool Hamming_12_8_decode(unsigned char *rxBits, unsigned char *decodedBits, int nbCodewords);

void Hamming_13_9_init();
void Hamming_13_9_encode(unsigned char *origBits, unsigned char *encodedBits);
bool Hamming_13_9_decode(unsigned char *rxBits, unsigned char *decodedBits, int nbCodewords);

void Hamming_15_11_init();
void Hamming_15_11_encode(unsigned char *origBits, unsigned char *encodedBits);
bool Hamming_15_11_decode(unsigned char *rxBits, unsigned char *decodedBits, int nbCodewords);

void Hamming_16_11_4_init();
void Hamming_16_11_4_encode(unsigned char *origBits, unsigned char *encodedBits);
bool Hamming_16_11_4_decode(unsigned char *rxBits, unsigned char *decodedBits, int nbCodewords);

void Golay_20_8_init();
void Golay_20_8_encode(unsigned char *origBits, unsigned char *encodedBits);
bool Golay_20_8_decode(unsigned char *rxBits);

void Golay_23_12_init();
void Golay_23_12_encode(unsigned char *origBits, unsigned char *encodedBits);
bool Golay_23_12_decode(unsigned char *rxBits);

void Golay_24_12_init();
void Golay_24_12_encode(unsigned char *origBits, unsigned char *encodedBits);
bool Golay_24_12_decode(unsigned char *rxBits);

void QR_16_7_6_init();
void QR_16_7_6_encode(unsigned char *origBits, unsigned char *encodedBits);
bool QR_16_7_6_decode(unsigned char *rxBits);

void InitAllFecFunction(void);


/* BPTC (Block Product Turbo Code) variables */
extern const uint8_t BPTCInterleavingIndex[196];
extern const uint8_t BPTCDeInterleavingIndex[196];


/* BPTC (Block Product Turbo Code) functions */
void BPTCDeInterleaveDMRData(uint8_t * Input, uint8_t * Output);
uint32_t BPTC_196x96_Extract_Data(uint8_t InputDeInteleavedData[196], uint8_t DMRDataExtracted[96], uint8_t R[3]);
uint32_t BPTC_128x77_Extract_Data(uint8_t InputDataMatrix[8][16], uint8_t DMRDataExtracted[77]);
uint32_t BPTC_16x2_Extract_Data(uint8_t InputInterleavedData[32], uint8_t DMRDataExtracted[32], uint32_t ParityCheckTypeOdd);


/* SYNC DMR data extraction functions */
void ProcessDmrPiHeader(dsd_opts * opts, dsd_state * state, uint8_t info[196], uint8_t syncdata[48], uint8_t SlotType[20]);
void ProcessDmrVoiceLcHeader(dsd_opts * opts, dsd_state * state, uint8_t info[196], uint8_t syncdata[48], uint8_t SlotType[20]);
void ProcessDmrTerminaisonLC(dsd_opts * opts, dsd_state * state, uint8_t info[196], uint8_t syncdata[48], uint8_t SlotType[20]);
void ProcessVoiceBurstSync(dsd_opts * opts, dsd_state * state);
void ProcessDmrCSBK(dsd_opts * opts, dsd_state * state, uint8_t info[196], uint8_t syncdata[48], uint8_t SlotType[20]);
void ProcessDmrDataHeader(dsd_opts * opts, dsd_state * state, uint8_t info[196], uint8_t syncdata[48], uint8_t SlotType[20]);
void ProcessDmrRate12Data(dsd_opts * opts, dsd_state * state, uint8_t info[196], uint8_t syncdata[48], uint8_t SlotType[20]);
void ProcessDmrRate34Data(dsd_opts * opts, dsd_state * state, uint8_t tdibits[98], uint8_t syncdata[48], uint8_t SlotType[20]);
void ProcessDmrRate1Data(dsd_opts * opts, dsd_state * state, uint8_t info[196], uint8_t syncdata[48], uint8_t SlotType[20]);
uint16_t ComputeCrcCCITT(uint8_t * DMRData);
uint32_t ComputeAndCorrectFullLinkControlCrc(uint8_t * FullLinkControlDataBytes, uint32_t * CRCComputed, uint32_t CRCMask);
uint8_t ComputeCrc5Bit(uint8_t * DMRData);
uint16_t ComputeCrc9Bit(uint8_t * DMRData, uint32_t NbData);
uint32_t ComputeCrc32Bit(uint8_t * DMRData, uint32_t NbData);
uint8_t * DmrAlgIdToStr(uint8_t AlgID);
uint8_t * DmrAlgPrivacyModeToStr(uint32_t PrivacyMode);


/* DMR Link control (LC) management functions */
void DmrLinkControlInitLib(void);
void DmrFullLinkControlDecode(dsd_opts * opts, dsd_state * state, uint8_t InputLCDataBit[96], int VoiceCallInProgress, int CSBKContent);
void PrintDmrGpsPositionFromLinkControlData(unsigned int GPSLongitude,
                                           unsigned int GPSLatitude,
                                           unsigned int GPSPositionError);
void PrintDmrTalkerAliasFromLinkControlData(unsigned int  TalkerAliasHeaderDataFormat,
                                            unsigned int  TalkerAliasHeaderDataLength,
                                            unsigned int  TalkerAliasBlockNumber,
                                            unsigned char TalkerAliasHeaderData[49],
                                            unsigned char TalkerAliasBlock1Data[56],
                                            unsigned char TalkerAliasBlock2Data[56],
                                            unsigned char TalkerAliasBlock3Data[56]);
void DmrBackupLinkControlData(dsd_opts * opts, dsd_state * state);
void DmrRestoreLinkControlData(dsd_opts * opts, dsd_state * state);


/* DMR data (rate 1/2 , rate 3/4 , rate 1) management */
void DmrDataContentInitLib(void);
void DmrDataHeaderDecode(uint8_t InputDataBit[96], DMRDataPDU_t * DMRDataStruct);
void DmrClearPreviouslyReceivedData(DMRDataPDU_t * DMRDataStruct);
char * DmrDataServiceAccessPointIdentifierToStr(int ServiceAccessPointIdentifier);
void DmrBackupReceivedData(dsd_opts * opts, dsd_state * state);
void DmrRestoreReceivedData(dsd_opts * opts, dsd_state * state);


/* Read Solomon (12,9) functions */
void rs_12_9_calc_syndrome(rs_12_9_codeword_t *codeword, rs_12_9_poly_t *syndrome);
uint8_t rs_12_9_check_syndrome(rs_12_9_poly_t *syndrome);
rs_12_9_correct_errors_result_t rs_12_9_correct_errors(rs_12_9_codeword_t *codeword, rs_12_9_poly_t *syndrome, uint8_t *errors_found);
rs_12_9_checksum_t *rs_12_9_calc_checksum(rs_12_9_codeword_t *codeword);




// Trellis library
bool CDMRTrellis_decode(const unsigned char* data, unsigned char* payload);
void CDMRTrellis_encode(const unsigned char* payload, unsigned char* data);
void CDMRTrellis_deinterleave(const unsigned char* data, signed char* dibits);
void CDMRTrellis_interleave(const signed char* dibits, unsigned char* data);
void CDMRTrellis_dibitsToPoints(const signed char* dibits, unsigned char* points);
void CDMRTrellis_pointsToDibits(const unsigned char* points, signed char* dibits);
void CDMRTrellis_bitsToTribits(const unsigned char* payload, unsigned char* tribits);
void CDMRTrellis_tribitsToBits(const unsigned char* tribits, unsigned char* payload);
bool CDMRTrellis_fixCode(unsigned char* points, unsigned int failPos, unsigned char* payload);
unsigned int CDMRTrellis_checkCode(const unsigned char* points, unsigned char* tribits);


#ifdef __cplusplus
}
#endif

#endif /* DSD_H */
