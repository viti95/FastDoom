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

typedef int bool;
typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef DWORD dword;
typedef WORD word;
typedef BYTE byte;
#define LOWORD(l)      ((WORD)((DWORD)(l)))
#define HIWORD(l)      ((WORD)(((DWORD)(l)>>16)&0xFFFF))
#define LOBYTE(w)      ((BYTE)(w))
#define HIBYTE(w)      ((BYTE)(((WORD)(w)>>8)&0xFF))

#define BIT0           1
#define BIT1           2
#define BIT2           4
#define BIT3           8
#define BIT4           0x10
#define BIT5           0x20
#define BIT6           0x40
#define BIT7           0x80
#define BIT8           0x100
#define BIT9           0x200
#define BIT10          0x400
#define BIT11          0x800
#define BIT12          0x1000
#define BIT13          0x2000
#define BIT14          0x4000
#define BIT15          0x8000
#define BIT16          0x10000
#define BIT17          0x20000
#define BIT18          0x40000
#define BIT19          0x80000
#define BIT20          0x100000
#define BIT21          0x200000
#define BIT22          0x400000
#define BIT23          0x800000
#define BIT24          0x1000000
#define BIT25          0x2000000
#define BIT26          0x4000000
#define BIT27          0x8000000
#define BIT28          0x10000000
#define BIT29          0x20000000
#define BIT30          0x40000000
#define BIT31          0x80000000

typedef enum {
	Raw, VOC, DemandFeed, WAV
} wavedata;

typedef enum {
	NoMoreData, KeepPlaying
} playbackstatus;

typedef struct VoiceNode {
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

typedef struct {
	VoiceNode *start;
	VoiceNode *end;
} VList;

typedef struct {
	char left;
	char right;
} Pan;

typedef signed short MONO16;
typedef signed char MONO8;

typedef struct {
	MONO16 left;
	MONO16 right;
	//   unsigned short left;
	//   unsigned short right;
} STEREO16;

typedef struct {
	MONO16 left;
	MONO16 right;
} SIGNEDSTEREO16;

typedef struct {
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

VoiceNode* MV_GetVoice(int handle);
static VoiceNode* MV_AllocVoice(int priority);

static short* MV_GetVolumeTable(int vol);

static void MV_SetVoiceMixMode(VoiceNode *voice);

void MV_SetVoicePitch(VoiceNode *voice, unsigned long rate);
void MV_SetVoiceVolume(VoiceNode *voice, int vol, int left, int right);
static void MV_CalcVolume(int MaxLevel);
static void MV_CalcPanTable(void);

void ClearBuffer_DW(void *ptr, unsigned data, int length);

#pragma aux ClearBuffer_DW = \
    "cld",                   \
            "push   es",     \
            "push   ds",     \
            "pop    es",     \
            "rep    stosd",  \
            "pop    es",     \
            parm[edi][eax][ecx] modify exact[ecx edi];

void MV_Mix8BitMono(unsigned long position, unsigned long rate,
		unsigned char *start, unsigned long length);
void MV_Mix8BitStereo(unsigned long position, unsigned long rate,
		unsigned char *start, unsigned long length);
void MV_Mix8BitUltrasound(unsigned long position, unsigned long rate,
		unsigned char *start, unsigned long length);

#endif
