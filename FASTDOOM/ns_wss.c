//
// WSS (Windows Sound System) driver for FastDoom
// Based on AD1848-compatible sound cards
//
// Uses ULTRA16 environment variable for configuration:
//   SET ULTRA16=<I/O hex>,<IRQ dec>,<DMA dec>
//   Example: SET ULTRA16=530,5,1
//

#include <dos.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ns_dpmi.h"
#include "ns_dma.h"
#include "ns_inter.h"
#include "ns_irq.h"
#include "ns_wss.h"
#include "ns_wssdef.h"
#include "ns_muldf.h"
#include "options.h"
#include "fastmath.h"

#define INVALID -1

static const unsigned char WSS_Interrupts[WSS_MaxIrq + 1] =
    {
        INVALID, INVALID, 0xa, 0xb,
        INVALID, 0xd, INVALID, 0xf,
        INVALID, INVALID, 0x72, 0x73,
        0x74, INVALID, INVALID, 0x77
    };

static void(__interrupt __far *WSS_OldInt)(void);

WSS_CONFIG WSS_Config =
    {
        WSS_DEFAULT_BASE, WSS_DEFAULT_IRQ, WSS_DEFAULT_DMA
    };

static int WSS_Installed = FALSE;
static int WSS_TransferLength = 0;
static int WSS_MixMode = WSS_DefaultMixMode;
static int WSS_SamplePacketSize = WSS_MONO_8BIT_SAMPLE_SIZE;
unsigned WSS_SampleRate = WSS_DefaultSampleRate;

volatile int WSS_SoundPlaying;
int WSS_DMAChannel;

static int WSS_IntController1Mask;
static int WSS_IntController2Mask;

static int WSS_OriginalPCMLeftVolume = 127;
static int WSS_OriginalPCMRightVolume = 127;

void (*WSS_CallBack)(void);

static char *WSS_DMABuffer;
static char *WSS_DMABufferEnd;
static char *WSS_CurrentDMABuffer;
static int WSS_TotalDMABufferSize;
static int WSS_NumDivisions;

static int WSS_PCMVolume = 255;
static int WSS_InterruptVector;

#define kStackSize 2048

static unsigned short StackSelector = 0;
static unsigned long StackPointer;

static unsigned short oldStackSelector;
static unsigned long oldStackPointer;

extern void GetStack(unsigned short *selptr, unsigned long *stackptr);
extern void SetStack(unsigned short selector, unsigned long stackptr);

#pragma aux GetStack = \
    "mov  [edi],esp"   \
    "mov  ax,ss"       \
    "mov  [esi],ax" parm[esi][edi] modify[eax esi edi];

#pragma aux SetStack = \
    "mov  ss,ax"       \
    "mov  esp,edx" parm[ax][edx] modify[eax edx];

#define WSS_TIMEOUT_LOOPS 100000

typedef struct
{
    unsigned sample_rate;
    unsigned char cfs;
    unsigned char css;
} WSS_SAMPLE_RATE_MAP;

static const WSS_SAMPLE_RATE_MAP WSS_SampleRateMap[] =
    {
        { 8000,   0,                     0              },
        {11025,   WSS_CSS_BIT,           WSS_CFS0_BIT   },
        {16000,   0,                     WSS_CFS0_BIT   },
        {22050,   WSS_CSS_BIT,           WSS_CFS1_BIT | WSS_CFS0_BIT },
        {32000,   0,                     WSS_CFS1_BIT | WSS_CFS0_BIT },
        {44100,   WSS_CSS_BIT,           WSS_CFS2_BIT | WSS_CFS0_BIT },
        {48000,   0,                     WSS_CFS2_BIT | WSS_CFS1_BIT  }
    };

static const WSS_SAMPLE_RATE_MAP *WSS_GetSampleRateConfig(unsigned sample_rate)
{
    int i;
    int num_rates = sizeof(WSS_SampleRateMap) / sizeof(WSS_SAMPLE_RATE_MAP);

    for (i = 0; i < num_rates; i++)
    {
        if (WSS_SampleRateMap[i].sample_rate == sample_rate)
        {
            return &WSS_SampleRateMap[i];
        }
    }
    return NULL;
}

static int WSS_Wait(void)
{
    unsigned timeout = WSS_TIMEOUT_LOOPS;

    while ((inp(WSS_Config.Address + WSS_INDEX_REG_OFFSET) & WSS_INIT_BIT) != 0)
    {
        if (timeout == 0)
        {
            return WSS_CardNotReady;
        }
        timeout--;
    }
    return WSS_Ok;
}

static int WSS_ReadIndirect(unsigned char addr, unsigned char *data)
{
    if (data == NULL)
    {
        return WSS_InvalidParameter;
    }

    outp(WSS_Config.Address + WSS_INDEX_REG_OFFSET, addr);
    *data = inp(WSS_Config.Address + WSS_IDATA_REG_OFFSET);

    return WSS_Ok;
}

static int WSS_WriteIndirect(unsigned char addr, unsigned char data)
{
    int status;

    outp(WSS_Config.Address + WSS_INDEX_REG_OFFSET, addr);
    status = WSS_Wait();
    if (status != WSS_Ok)
    {
        return status;
    }

    outp(WSS_Config.Address + WSS_IDATA_REG_OFFSET, data);
    return WSS_Wait();
}

static void WSS_EnableInterrupt(void)
{
    int irq = WSS_Config.Interrupt;
    int mask;

    if (irq < 8)
    {
        mask = inp(0x21) & ~(1 << irq);
        outp(0x21, mask);
    }
    else
    {
        mask = inp(0xA1) & ~(1 << (irq - 8));
        outp(0xA1, mask);

        mask = inp(0x21) & ~(1 << 2);
        outp(0x21, mask);
    }
}

static void WSS_DisableInterrupt(void)
{
    int irq = WSS_Config.Interrupt;
    int mask;

    if (irq < 8)
    {
        mask = inp(0x21) & ~(1 << irq);
        mask |= WSS_IntController1Mask & (1 << irq);
        outp(0x21, mask);
    }
    else
    {
        mask = inp(0x21) & ~(1 << 2);
        mask |= WSS_IntController1Mask & (1 << 2);
        outp(0x21, mask);

        mask = inp(0xA1) & ~(1 << (irq - 8));
        mask |= WSS_IntController2Mask & (1 << (irq - 8));
        outp(0xA1, mask);
    }
}

void __interrupt __far WSS_ServiceInterrupt(void)
{
    GetStack(&oldStackSelector, &oldStackPointer);
    SetStack(StackSelector, StackPointer);

    {
        unsigned char status = inp(WSS_Config.Address + WSS_STATUS_REG_OFFSET);
        if (!(status & 0x01))
        {
            SetStack(oldStackSelector, oldStackPointer);
            _chain_intr(WSS_OldInt);
            return;
        }
        outp(WSS_Config.Address + WSS_STATUS_REG_OFFSET, 0x00);
    }

    WSS_CurrentDMABuffer += WSS_TransferLength;
    if (WSS_CurrentDMABuffer >= WSS_DMABufferEnd)
    {
        WSS_CurrentDMABuffer = WSS_DMABuffer;
    }

    if (WSS_CallBack != NULL)
    {
        MV_ServiceVoc();
    }

    SetStack(oldStackSelector, oldStackPointer);

    if (WSS_Config.Interrupt > 7)
    {
        outp(0xA0, 0x20);
    }
    outp(0x20, 0x20);
}

int WSS_GetEnv(WSS_CONFIG *Config)
{
    char *ultra16;
    int parsed;
    unsigned int base;
    unsigned int irq;
    unsigned int dma;

    ultra16 = getenv("ULTRA16");
    if (ultra16 == NULL)
    {
        return WSS_EnvNotFound;
    }

    parsed = sscanf(ultra16, "%x,%u,%u", &base, &irq, &dma);
    if (parsed != 3)
    {
        return WSS_EnvNotFound;
    }

    if (Config != NULL)
    {
        Config->Address = base;
        Config->Interrupt = irq;
        Config->Dma = dma;
    }

    return WSS_Ok;
}

int WSS_SetCardSettings(WSS_CONFIG Config)
{
    if (Config.Address == 0 || Config.Interrupt == 0 || Config.Interrupt > 15 ||
        Config.Dma > 3)
    {
        return WSS_InvalidParameter;
    }

    WSS_Config.Address = Config.Address;
    WSS_Config.Interrupt = Config.Interrupt;
    WSS_Config.Dma = Config.Dma;

    return WSS_Ok;
}

int WSS_GetCardSettings(WSS_CONFIG *Config)
{
    if (Config == NULL)
    {
        return WSS_InvalidParameter;
    }

    *Config = WSS_Config;
    return WSS_Ok;
}

void WSS_SetPlaybackRate(unsigned rate)
{
    if (rate < WSS_MinSamplingRate)
    {
        rate = WSS_MinSamplingRate;
    }
    if (rate > WSS_MaxSamplingRate)
    {
        rate = WSS_MaxSamplingRate;
    }

    WSS_SampleRate = rate;
}

unsigned WSS_GetPlaybackRate(void)
{
    return WSS_SampleRate;
}

int WSS_SetMixMode(int mode)
{
    if (mode > WSS_STEREO_8BIT)
    {
        mode = WSS_STEREO_8BIT;
    }

    WSS_MixMode = mode;

    if (mode & WSS_STEREO)
    {
        WSS_SamplePacketSize = WSS_STEREO_8BIT_SAMPLE_SIZE;
    }
    else
    {
        WSS_SamplePacketSize = WSS_MONO_8BIT_SAMPLE_SIZE;
    }

    return WSS_Ok;
}

int WSS_SetPCMVolume(int volume)
{
    unsigned char reg;
    int status;

    if (volume < 0)
    {
        volume = 0;
    }
    if (volume > 255)
    {
        volume = 255;
    }

    WSS_PCMVolume = volume;

    if (volume == 0)
    {
        reg = WSS_DAC_MUTE_BIT;
    }
    else
    {
        reg = (unsigned char)(((255 - volume) * WSS_DAC_ATTEN_MAX_VAL) / 255);
    }

    status = WSS_WriteIndirect(WSS_LDAC_REG, reg);
    if (status != WSS_Ok)
    {
        return status;
    }

    status = WSS_WriteIndirect(WSS_RDAC_REG, reg);
    return status;
}

int WSS_GetCardInfo(int *MaxSampleBits, int *MaxChannels)
{
    if (MaxSampleBits != NULL)
    {
        *MaxSampleBits = 8;
    }
    if (MaxChannels != NULL)
    {
        *MaxChannels = 2;
    }
    return WSS_Ok;
}

void WSS_StopPlayback(void)
{
    unsigned char reg;
    int status;

    if (!WSS_Installed)
    {
        return;
    }

    WSS_SoundPlaying = FALSE;

    status = WSS_ReadIndirect(WSS_CONFIG_REG, &reg);
    if (status == WSS_Ok)
    {
        reg &= ~WSS_PEN_BIT;
        WSS_WriteIndirect(WSS_CONFIG_REG, reg);
    }

    DMA_EndTransfer(WSS_DMAChannel);
    WSS_DisableInterrupt();
}

int WSS_BeginBufferedPlayback(char *BufferStart, int BufferSize, int NumDivisions,
                               unsigned SampleRate, int MixMode, void (*CallBackFunc)(void))
{
    int status;
    unsigned char dma_count_lo, dma_count_hi;
    const WSS_SAMPLE_RATE_MAP *rate_cfg;
    int channels;
    int dma_transfer_length;

    rate_cfg = WSS_GetSampleRateConfig(SampleRate);
    if (rate_cfg == NULL)
    {
        rate_cfg = &WSS_SampleRateMap[1];
        SampleRate = 11025;
    }

    WSS_SampleRate = SampleRate;
    WSS_MixMode = MixMode;
    WSS_CallBack = CallBackFunc;
    WSS_NumDivisions = NumDivisions;

    channels = (MixMode & WSS_STEREO) ? 2 : 1;
    WSS_SamplePacketSize = channels;

    WSS_DMAChannel = WSS_Config.Dma;

    WSS_TransferLength = BufferSize / NumDivisions;
    dma_transfer_length = WSS_TransferLength;

    WSS_SetPCMVolume(WSS_PCMVolume);

    WSS_WriteIndirect(WSS_LAUX1_REG, WSS_AUX_MUTE_BIT);
    WSS_WriteIndirect(WSS_RAUX1_REG, WSS_AUX_MUTE_BIT);
    WSS_WriteIndirect(WSS_LAUX2_REG, WSS_AUX_MUTE_BIT);
    WSS_WriteIndirect(WSS_RAUX2_REG, WSS_AUX_MUTE_BIT);

    WSS_WriteIndirect(WSS_LADC_REG, 0x00);
    WSS_WriteIndirect(WSS_RADC_REG, 0x00);

    WSS_WriteIndirect(WSS_MIX_REG, 0x00);
    WSS_WriteIndirect(WSS_PIN_REG, WSS_IEN_BIT);

    {
        unsigned short dma_count = (unsigned short)(dma_transfer_length / channels) - 1;
        dma_count_lo = (unsigned char)(dma_count & 0xFF);
        dma_count_hi = (unsigned char)((dma_count >> 8) & 0xFF);
    }

    status = WSS_WriteIndirect(WSS_PLAYBACK_LCNT_REG, dma_count_lo);
    if (status != WSS_Ok) return status;
    status = WSS_WriteIndirect(WSS_PLAYBACK_UCNT_REG, dma_count_hi);
    if (status != WSS_Ok) return status;

    /* Configure format register with MCE */
    {
        unsigned char format_reg = rate_cfg->cfs | rate_cfg->css;
        if (MixMode & WSS_STEREO)
        {
            format_reg |= WSS_S_M_BIT;
        }

        status = WSS_WriteIndirect(WSS_FORMAT_REG | WSS_MCE_BIT, format_reg);
        if (status != WSS_Ok) return status;

        /* Exit MCE state */
        outp(WSS_Config.Address + WSS_INDEX_REG_OFFSET, 0x00);
    }

    /* Enable playback with auto-calibration (standard CONFIG, no MCE) */
    {
        unsigned char config_val = WSS_PEN_BIT | WSS_ACAL_BIT;
        status = WSS_WriteIndirect(WSS_CONFIG_REG, config_val);
        if (status != WSS_Ok) return status;
    }

    /* Wait for auto-calibration to complete */
    {
        unsigned int wait_count = 0;
        outp(WSS_Config.Address + WSS_INDEX_REG_OFFSET, WSS_TEST_INIT_REG);
        while ((inp(WSS_Config.Address + WSS_IDATA_REG_OFFSET) & WSS_ACI_BIT) != 0)
        {
            wait_count++;
            if (wait_count > 100000)
            {
                break;
            }
        }
    }

    status = DMA_SetupTransfer(WSS_DMAChannel, BufferStart, BufferSize, DMA_AutoInitRead);
    if (status != DMA_Ok)
    {
        return WSS_DmaError;
    }

    WSS_DMABuffer = BufferStart;
    WSS_DMABufferEnd = BufferStart + BufferSize;
    WSS_CurrentDMABuffer = BufferStart;
    WSS_TotalDMABufferSize = BufferSize;

    WSS_EnableInterrupt();

    WSS_SoundPlaying = TRUE;

    return WSS_Ok;
}

static int WSS_Detect(void)
{
    unsigned char test_val;
    unsigned char read_val;
    int status;

    status = WSS_ReadIndirect(WSS_CONFIG_REG, &test_val);
    if (status != WSS_Ok)
    {
        return WSS_Error;
    }

    status = WSS_WriteIndirect(WSS_CONFIG_REG, test_val | WSS_PEN_BIT);
    if (status != WSS_Ok)
    {
        return WSS_Error;
    }

    status = WSS_ReadIndirect(WSS_CONFIG_REG, &read_val);
    if (status != WSS_Ok)
    {
        return WSS_Error;
    }

    WSS_WriteIndirect(WSS_CONFIG_REG, test_val);

    return WSS_Ok;
}

int WSS_Init(void)
{
    WSS_CONFIG config;
    int status;

    if (WSS_Installed)
    {
        WSS_Shutdown();
    }

    status = WSS_GetEnv(&config);
    if (status == WSS_Ok)
    {
        WSS_Config.Address = config.Address;
        WSS_Config.Interrupt = config.Interrupt;
        WSS_Config.Dma = config.Dma;
    }
    else
    {
        WSS_Config.Address = WSS_DEFAULT_BASE;
        WSS_Config.Interrupt = WSS_DEFAULT_IRQ;
        WSS_Config.Dma = WSS_DEFAULT_DMA;
    }

    if (WSS_Config.Dma > 3)
    {
        WSS_Config.Dma = WSS_DEFAULT_DMA;
    }

    if (WSS_Config.Interrupt < 8)
    {
        WSS_InterruptVector = 0x08 + WSS_Config.Interrupt;
    }
    else
    {
        WSS_InterruptVector = 0x70 + (WSS_Config.Interrupt - 8);
    }

    status = WSS_Detect();
    if (status != WSS_Ok)
    {
        return WSS_Error;
    }

    if (WSS_Config.Interrupt < 8)
    {
        WSS_IntController1Mask = inp(0x21);
    }
    else
    {
        WSS_IntController1Mask = inp(0x21);
        WSS_IntController2Mask = inp(0xA1);
    }

    status = WSS_ReadIndirect(WSS_LDAC_REG, (unsigned char *)&WSS_OriginalPCMLeftVolume);
    if (status == WSS_Ok)
    {
        WSS_ReadIndirect(WSS_RDAC_REG, (unsigned char *)&WSS_OriginalPCMRightVolume);
    }

    WSS_OldInt = _dos_getvect(WSS_InterruptVector);
    _dos_setvect(WSS_InterruptVector, WSS_ServiceInterrupt);

    {
        union REGS regs;
        memset(&regs, 0, sizeof(regs));
        regs.w.ax = 0x100;
        regs.w.bx = (kStackSize + 15) / 16;
        int386(0x31, &regs, &regs);
        if (regs.w.cflag)
        {
            _dos_setvect(WSS_InterruptVector, WSS_OldInt);
            return WSS_DPMI_Error;
        }
        StackSelector = regs.w.dx;
        StackPointer = kStackSize - sizeof(long);
    }

    WSS_PCMVolume = 255;

    WSS_Installed = TRUE;

    return WSS_Ok;
}

void WSS_Shutdown(void)
{
    if (!WSS_Installed)
    {
        return;
    }

    if (WSS_SoundPlaying)
    {
        WSS_StopPlayback();
    }

    {
        int irq = WSS_Config.Interrupt;
        if (irq < 8)
        {
            outp(0x21, WSS_IntController1Mask);
        }
        else
        {
            outp(0xA1, WSS_IntController2Mask);
            outp(0x21, WSS_IntController1Mask);
        }
    }

    _dos_setvect(WSS_InterruptVector, WSS_OldInt);

    WSS_WriteIndirect(WSS_LDAC_REG, (unsigned char)WSS_OriginalPCMLeftVolume);
    WSS_WriteIndirect(WSS_RDAC_REG, (unsigned char)WSS_OriginalPCMRightVolume);

    if (StackSelector != 0)
    {
        union REGS regs;
        memset(&regs, 0, sizeof(regs));
        regs.w.ax = 0x101;
        regs.w.dx = StackSelector;
        int386(0x31, &regs, &regs);
        StackPointer = 0;
        StackSelector = 0;
    }

    WSS_Installed = FALSE;
    WSS_SoundPlaying = FALSE;
}
