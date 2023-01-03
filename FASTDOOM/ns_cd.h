#include <i86.h>
#include <string.h>

#define EJECT_TRAY 0
#define RESET 2
#define CLOSE_TRAY 5
#define DATA_TRACK 64
#define LOCK 1
#define UNLOCK 0
#define BUSY 512
#define MEDIA_CHANGE 9
#define TRACK_MASK 208

struct CD_Cdrom_data
{
    unsigned short Drives;
    unsigned char First_drive;
    unsigned short Current_track;
    unsigned long Track_position;
    unsigned char Track_type;
    unsigned char Low_audio;
    unsigned char High_audio;
    unsigned char Disk_length_min;
    unsigned char Disk_length_sec;
    unsigned char Disk_length_frames;
    unsigned long Endofdisk;
    unsigned char UPC[7];
    unsigned char DiskID[6];
    unsigned long Status;
    unsigned short Error;
};

typedef struct CD_Volumeinfo
{
    unsigned char Mode;
    unsigned char Input0;
    unsigned char Volume0;
    unsigned char Input1;
    unsigned char Volume1;
    unsigned char Input2;
    unsigned char Volume2;
    unsigned char Input3;
    unsigned char Volume3;
};

extern struct CD_Cdrom_data CD_Cdrom_data;
extern struct CD_Volumeinfo CD_Volumeinfo;

// ------------------------------------------------------------------------
//                      CDROM Prototypes
// ------------------------------------------------------------------------
unsigned long CD_HeadPosition(void);
void CD_GetVolume(void);
void CD_SetVolume(unsigned char vol);
short CD_GetUPC(void);
void CD_GetAudioInfo(void);
void CD_SetTrack(short Tracknum);
void CD_TrackLength(short Tracknum, unsigned char *min, unsigned char *sec, unsigned char *frame);
void CD_Status(void);
void CD_Seek(unsigned long Location);
void CD_PlayAudio(unsigned long Begin, unsigned long End);
void CD_StopAudio(void);
void CD_ResumeAudio(void);
void CD_CMD(unsigned char mode);
void CD_Getpos(void);
short CD_Installed(void);
short CD_DonePlay(void);
short CD_Mediach(void);
void CD_Lock(unsigned char Doormode);
int CD_Init(void);
void CD_DeInit(void);
void CD_Exit(void);

// ------------------------------------------------------------------------
//                          DPMI Support Functions..
// ------------------------------------------------------------------------
void DPMI_AllocDOSMem(short int paras, struct DPMI_PTR *p);
void DPMI_FreeDOSMem(struct DPMI_PTR *p);
// ------------------------------------------------------------------------
