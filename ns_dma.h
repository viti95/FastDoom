#ifndef __DMA_H
#define __DMA_H

enum DMA_ERRORS
{
    DMA_Error = -1,
    DMA_Ok = 0,
    DMA_ChannelOutOfRange,
    DMA_InvalidChannel
};

enum DMA_Modes
{
    DMA_SingleShotRead,
    DMA_SingleShotWrite,
    DMA_AutoInitRead,
    DMA_AutoInitWrite
};

char *DMA_ErrorString(
    int ErrorNumber);

int DMA_VerifyChannel(
    int channel);

int DMA_SetupTransfer(
    int channel,
    char *address,
    int length,
    int mode);

int DMA_EndTransfer(
    int channel);

char *DMA_GetCurrentPos(
    int channel);

int DMA_GetTransferCount(
    int channel);

#endif
