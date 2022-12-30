#ifndef __AUDIOCD_H
#define __AUDIOCD_H

int CD_cdrck(int device);
void CD_cdrreq(int device, void *cdrrh);
int CD_ioctl_in(int device, void far *tranad, int tranct);
int CD_ioctl_out(int device, void far *tranad, int tranct);
void CD_ioctl_in_ck(int device, void far *tranad, int tranct);
void CD_seek(int device, long track);
void CD_play(int device, long Strsect, long Nsect);
void CD_Stop_Audio(int device);
long CD_HSSect(unsigned char RB[4]);
int CD_detectFirstCD(void);

#endif
