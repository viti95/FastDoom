#ifndef ___MULTIVC_H
#define ___MULTIVC_H

#define TRUE (1 == 1)
#define FALSE (!TRUE)

#define VOC_8BIT 0x0
#define VOC_CT4_ADPCM 0x1
#define VOC_CT3_ADPCM 0x2
#define VOC_CT2_ADPCM 0x3
#define VOC_16BIT 0x4
#define VOC_ALAW 0x6
#define VOC_MULAW 0x7
#define VOC_CREATIVE_ADPCM 0x200

#define T_SIXTEENBIT_STEREO 0
#define T_8BITS 1
#define T_MONO 2
#define T_16BITSOURCE 4
#define T_LEFTQUIET 8
#define T_RIGHTQUIET 16
#define T_ULTRASOUND 32
#define T_DEFAULT T_SIXTEENBIT_STEREO

#define MV_MaxPanPosition 31
#define MV_NumPanPositions (MV_MaxPanPosition + 1)
#define MV_MaxTotalVolume 255
#define MV_MaxVolume 63
#define MV_NumVoices 8

#define MIX_VOLUME(volume) ((max(0, min((volume), 255)) * (MV_MaxVolume + 1)) >> 8)

//#define SILENCE_16BIT     0x80008000
#define SILENCE_16BIT 0
#define SILENCE_8BIT 0x80808080
//#define SILENCE_16BIT_PAS 0

#define MixBufferSize 256

#define NumberOfBuffers 16
#define TotalBufferSize (MixBufferSize * NumberOfBuffers)

#define PI 3.1415926536

typedef enum
{
    Raw,
    VOC,
    DemandFeed,
    WAV
} wavedata;

typedef enum
{
    NoMoreData,
    KeepPlaying
} playbackstatus;

typedef struct VoiceNode
{
    struct VoiceNode *next;
    struct VoiceNode *prev;

    char bits;

    playbackstatus (*GetSound)(struct VoiceNode *voice);

    void (*mix)(unsigned long position, unsigned long rate,
                unsigned char *start, unsigned long length);

    unsigned char *NextBlock;
    unsigned long BlockLength;

    unsigned long FixedPointBufferSize;

    unsigned char *sound;
    unsigned long length;
    unsigned long RateScale;
    unsigned long position;
    int Playing;

    int handle;
    int priority;

    void (*DemandFeed)(char **ptr, unsigned long *length);

    short *LeftVolume;
    short *RightVolume;

} VoiceNode;

typedef struct
{
    VoiceNode *start;
    VoiceNode *end;
} VList;

typedef struct
{
    char left;
    char right;
} Pan;

typedef signed short MONO16;
typedef signed char MONO8;

typedef struct
{
    MONO16 left;
    MONO16 right;
    //   unsigned short left;
    //   unsigned short right;
} STEREO16;

typedef struct
{
    MONO16 left;
    MONO16 right;
} SIGNEDSTEREO16;

typedef struct
{
    //   MONO8 left;
    //   MONO8 right;
    char left;
    char right;
} STEREO8;

typedef MONO8 VOLUME8[256];
typedef MONO16 VOLUME16[256];

typedef char HARSH_CLIP_TABLE_8[MV_NumVoices * 256];

static void MV_Mix(VoiceNode *voice, int buffer);
static void MV_PlayVoice(VoiceNode *voice);
static void MV_StopVoice(VoiceNode *voice);
void MV_ServiceVoc(void);

static playbackstatus MV_GetNextDemandFeedBlock(VoiceNode *voice);
static playbackstatus MV_GetNextRawBlock(VoiceNode *voice);

VoiceNode *MV_GetVoice(int handle);
static VoiceNode *MV_AllocVoice(int priority);

static short *MV_GetVolumeTable(int vol);

static void MV_SetVoiceMixMode(VoiceNode *voice);

void MV_SetVoicePitch(VoiceNode *voice, unsigned long rate);
void MV_SetVoiceVolume(VoiceNode *voice, int vol, int left, int right);
static void MV_CalcVolume(int MaxLevel);

void ClearBuffer_DW(void *ptr, unsigned data, int length);

#pragma aux ClearBuffer_DW = \
    "cld",                   \
            "push   es",     \
            "push   ds",     \
            "pop    es",     \
            "rep    stosd",  \
            "pop    es",     \
            parm[edi][eax][ecx] modify exact[ecx edi];

void MV_Mix8BitMono(unsigned long position, unsigned long rate, unsigned char *start, unsigned long length);
void MV_Mix8BitStereo(unsigned long position, unsigned long rate, unsigned char *start, unsigned long length);
void MV_Mix8BitUltrasound(unsigned long position, unsigned long rate, unsigned char *start, unsigned long length);

#endif
