#ifndef __BLASTER_H
#define __BLASTER_H

typedef struct
{
    unsigned Address;
    unsigned Type;
    unsigned Interrupt;
    unsigned Dma8;
    unsigned Dma16;
    unsigned Midi;
    unsigned Emu;
} BLASTER_CONFIG;

extern BLASTER_CONFIG BLASTER_Config;
extern int BLASTER_DMAChannel;
extern int BLASTER_Version;

#define UNDEFINED -1

enum BLASTER_ERRORS
{
    BLASTER_Warning = -2,
    BLASTER_Error = -1,
    BLASTER_Ok = 0,
    BLASTER_EnvNotFound,
    BLASTER_AddrNotSet,
    BLASTER_DMANotSet,
    BLASTER_DMA16NotSet,
    BLASTER_InvalidParameter,
    BLASTER_CardNotReady,
    BLASTER_NoSoundPlaying,
    BLASTER_InvalidIrq,
    BLASTER_UnableToSetIrq,
    BLASTER_DmaError,
    BLASTER_NoMixer,
    BLASTER_DPMI_Error,
    BLASTER_OutOfMemory
};

enum BLASTER_Types
{
    SB = 1,
    SBPro = 2,
    SB20 = 3,
    SBPro2 = 4,
    SB16 = 6
};

#define BLASTER_MinCardType SB
#define BLASTER_MaxCardType SB16

#define STEREO 1
#define SIXTEEN_BIT 2

#define MONO_8BIT 0
#define STEREO_8BIT (STEREO)
#define MONO_16BIT (SIXTEEN_BIT)
#define STEREO_16BIT (STEREO | SIXTEEN_BIT)

#define BLASTER_MaxMixMode STEREO_16BIT

#define MONO_8BIT_SAMPLE_SIZE 1
#define MONO_16BIT_SAMPLE_SIZE 2
#define STEREO_8BIT_SAMPLE_SIZE (2 * MONO_8BIT_SAMPLE_SIZE)
#define STEREO_16BIT_SAMPLE_SIZE (2 * MONO_16BIT_SAMPLE_SIZE)

#define BLASTER_DefaultSampleRate 11000
#define BLASTER_DefaultMixMode MONO_8BIT
#define BLASTER_MaxIrq 15

void BLASTER_EnableInterrupt(void);
void BLASTER_DisableInterrupt(void);
void BLASTER_WriteDSP(unsigned data);
int BLASTER_ReadDSP(void);
int BLASTER_ResetDSP(void);
int BLASTER_GetDSPVersion(void);
void BLASTER_SpeakerOn(void);
void BLASTER_SpeakerOff(void);
void BLASTER_SetPlaybackRate(unsigned rate);
unsigned BLASTER_GetPlaybackRate(void);
int BLASTER_SetMixMode(int mode);
void BLASTER_StopPlayback(void);
int BLASTER_SetupDMABuffer(char *BufferPtr, int BufferSize, int mode);
int BLASTER_GetCurrentPos(void);
int BLASTER_DSP1xx_BeginPlayback(int length);
int BLASTER_DSP2xx_BeginPlayback(int length);
int BLASTER_DSP4xx_BeginPlayback(int length);
int BLASTER_BeginBufferedPlayback(char *BufferStart,
                                  int BufferSize, int NumDivisions, unsigned SampleRate,
                                  int MixMode, void (*CallBackFunc)(void));
void BLASTER_WriteMixer(int reg, int data);
int BLASTER_ReadMixer(int reg);
int BLASTER_SetVoiceVolume(int volume);
int BLASTER_GetMidiVolume(void);
void BLASTER_SetMidiVolume(int volume);
int BLASTER_CardHasMixer(void);
void BLASTER_SaveVoiceVolume(void);
void BLASTER_RestoreVoiceVolume(void);
void BLASTER_SaveMidiVolume(void);
void BLASTER_RestoreMidiVolume(void);
int BLASTER_GetEnv(BLASTER_CONFIG *Config);
int BLASTER_SetCardSettings(BLASTER_CONFIG Config);
int BLASTER_GetCardSettings(BLASTER_CONFIG *Config);
int BLASTER_GetCardInfo(int *MaxSampleBits, int *MaxChannels);
void BLASTER_SetupWaveBlaster(void);
void BLASTER_ShutdownWaveBlaster(void);
int BLASTER_Init(void);
void BLASTER_Shutdown(void);

#endif
