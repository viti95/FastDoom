#ifndef ___GUSWAVE_H
#define ___GUSWAVE_H

#define TRUE (1 == 1)
#define FALSE (!TRUE)

#define LOADDS _loadds

#define VOC_8BIT 0x0
#define VOC_CT4_ADPCM 0x1
#define VOC_CT3_ADPCM 0x2
#define VOC_CT2_ADPCM 0x3
#define VOC_16BIT 0x4
#define VOC_ALAW 0x6
#define VOC_MULAW 0x7
#define VOC_CREATIVE_ADPCM 0x200

#define MAX_BLOCK_LENGTH 0x8000

#define GF1BSIZE 896L /* size of buffer per wav on GUS */
//#define GF1BSIZE   512L   /* size of buffer per wav on GUS */

//#define VOICES     8      /* maximum amount of concurrent wav files */
#define VOICES 2      /* maximum amount of concurrent wav files */
#define MAX_VOICES 32 /* This should always be 32 */
#define MAX_VOLUME 4095
#define BUFFER 2048U /* size of DMA buffer for patch loading */

typedef enum
{
    NoMoreData,
    KeepPlaying,
    SoundDone
} playbackstatus;

typedef volatile struct VoiceNode
{
    struct VoiceNode *next;
    struct VoiceNode *prev;

    int bits;
    playbackstatus (*GetSound)(volatile struct VoiceNode *voice);

    int num;

    unsigned long mem; /* location in ultrasound memory */
    int Active;        /* this instance in use */
    int GF1voice;      /* handle to active voice */

    char *NextBlock;
    unsigned long BlockLength;

    unsigned char *sound;
    unsigned long length;
    unsigned long RateScale;
    int Playing;

    int handle;
    int priority;

    void (*DemandFeed)(char **ptr, unsigned long *length);

    int Volume;
    int Pan;
} VoiceNode;

typedef struct
{
    VoiceNode *start;
    VoiceNode *end;
} voicelist;

typedef volatile struct voicestatus
{
    VoiceNode *Voice;
    int playing;
} voicestatus;

#endif
