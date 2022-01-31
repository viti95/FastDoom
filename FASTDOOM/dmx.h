//
// Copyright (C) 2015 Alexey Khokholov (Nuke.YKT)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

#ifndef _DMX_H_
#define _DMX_H_

int AL_DetectFM(void);
int MPU_Init(int addr);
int GUS_Init(void);
void GUS_Shutdown(void);

void TSM_DelService(int unk1);
void TSM_Remove(void);
int MUS_RegisterSong(void *data);
int MUS_ChainSong(int handle, int next);
void MUS_PlaySong(int handle, int volume);
int SFX_PlayPatch(void *vdata, int sep, int vol);
void SFX_StopPatch(int handle);
int SFX_Playing(int handle);
void SFX_SetOrigin(int handle, int sep, int vol);
int GF1_Detect(void);
void GF1_SetMap(void *data, int len);
int SB_Detect(int *port, int *irq, int *dma, int *unk);
void SB_SetCard(int port, int irq, int dma);
int AL_Detect(int *port, int *unk);
void AL_SetCard(void *data);
int MPU_Detect(int *port, int *unk);
void MPU_SetCard(int port);
void ASS_Init(int rate, int maxsng, int mdev, int sdev);
void ASS_DeInit(void);
void WAV_PlayMode(int channels, int samplerate);
int CODEC_Detect(int *a, int *b);
int ENS_Detect(void);

#endif
