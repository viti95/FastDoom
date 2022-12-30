/* CDPLAY Command-Line CD-Player Utility (C) 1995 by Edgar Swank  */
/* This program is shareware. If it's useful to you please send   */
/* $5 to Edgar Swank; 5515 Spinnaker Dr., #4; San Jose, CA 95123  */

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <conio.h>
#include <bios.h>
#include <string.h>

#include "ns_cd.h"

int device;
int i;
long HSldout;
int status;
int busy;

struct ReqHdr
{
    char rhlen;   /* Length in bytes of request header */
    char subu;    /* Subunit code for minor devices */
    char comc;    /* Command code field */
    int status;   /* Status */
    char rsvd[8]; /* Reserved */
};

#define rherr 0x8000
#define rhbusy 0x0020
#define rhdone 0x0010
#define rherm 0x00ff

struct SeekReq
{
    struct ReqHdr rh;
    char am;           /* 0 Addressing mode = High Sierra */
    long ta;           /* 0 Transfer address */
    int str;           /* 0 Number of sectors to read */
    unsigned long ssn; /*   Starting sector number */
};

struct PlayReq
{
    struct ReqHdr rh;
    char am;      /* 0 Addressing mode = High Sierra */
    long Strsect; /*   Starting sector number */
    long Nsect;   /*   Number of sectors to read */
};

static struct ReqHdr StopPlayReq = {13, 0, 133, 0};

/* The following values are valid command codes */

#define play_audio 132   /* PLAY AUDIO   */
#define stop_audio 133   /* STOP AUDIO   */
#define resume_audio 136 /* RESUME AUDIO */

struct IOCTLI
{
    struct ReqHdr rh;
    char mdb;         /* Media descriptor byte from BPB 0 */
    void far *tranad; /* Transfer address */
    int tranct;       /* Number of bytes to transfer */
    int ssn;          /* Starting sector number 0 */
    void far *volid;  /* DWORD ptr to requested vol ID if error 0FH */
};

struct IOCTLO
{
    struct ReqHdr rh;
    char mdb;         /* Media descriptor byte from BPB 0 */
    void far *tranad; /* Transfer address */
    int tranct;       /* Number of bytes to transfer */
    int ssn;          /* Starting sector number 0 */
    void far *volid;  /* DWORD ptr to requested vol ID if error 0FH */
};

/* Audio Channel Control */
struct icvc
{
    char ic;
    unsigned char vc;
};

static struct
{
    char cc; /* 3 Control block code */
    struct icvc z[4];
} AI = {3};

/* Return Volume Size */

static struct
{
    char cc;      /* Control block code */
    long volsize; /* Volume size */
} vs = {8};

int min;
float sec;
float secrem;
long Strtsect, Nsect;

/*Audio Disk Info*/

static struct
{
    char cc;                   /* 10  Control block code */
    unsigned char lotrak;      /* Lowest track number */
    unsigned char hitrak;      /* Highest track number */
    unsigned char ldouttrk[4]; /* Starting point of the lead-out track */
} di = {10};

/* Audio Track Info */

static struct
{
    char cc;                   /* 11 Control block code */
    unsigned char trakno;      /* Track number */
    unsigned char trakstrt[4]; /* Starting point of the track (Red Book)*/
    unsigned char tctl;        /* Track control information */
} ati = {11};

/* Audio Q-Channel Info */

static struct
{
    char cc;     /* 12 Control block code */
    char ctladr; /*    CONTROL and ADR byte */
    char tno;    /*    Track number (TNO)  BCD*/
    char point;  /*    (POINT) or Index (X) */
                 /* Running time within a track */
    char min;    /* (MIN)    */
    char sec;    /* (SEC)    */
    char frame;  /* (FRAME)  */
    char zero;   /* (ZERO)   */
                 /* Running time on the disk */
    char amin;   /* (AMIN) or (PMIN)         */
    char asec;   /* (ASEC) or (PSEC)         */
    char aframe; /* (AFRAME) or (PFRAME)     */
} Qi = {12};

/*int cdrck(int device);
void cdrreq(int device, void *cdrrh);
int ioctl_in(int device, void far *tranad, int tranct);
int ioctl_out(int device, void far *tranad, int tranct);
void ioctl_in_ck(int device, void far *tranad, int tranct);
void seek(int device, long track);
void play(int device, long Strsect, long Nsect);
void Stop_Audio(int device);
long HSSect(unsigned char RB[4]);*/

int CD_detectFirstCD(void)
{
    int i;

    for (i = 0; i < 26; i++)
    {
        if (CD_cdrck(i))
            return i;
    }

    return -1;
}

void rm_main(int argc, char *argv[])
{
    printf("CDPLAY (C) 1995 by Edgar Swank\n");
    if (argc < 2 || argv[1][0] == '?')
    {
        printf("One Operand Required. CDROM Device Letter\n");
        printf("0-4 Optional Operands, [+]Track to Play[Start]\n");
        printf("  Left Channel L|R|0  Right Channel L|R|0\n");
        printf("  xx.x-seconds to skip at head of track\n");
        printf("  xx.x-seconds to play (up to end of track[disk]\n");
        exit(8);
    }

    device = toupper(argv[1][0]);
    device = device - 'A';
    if (!CD_cdrck(device))
    {
        printf("Device %c is not a CD-ROM, or MSCDEX is not installed\n",
               device + 'A');
    }

    status = CD_ioctl_in(device, &Qi, sizeof(Qi));
    busy = status & 0x0200;
    if (busy)
    {
        printf("CD-Player %c is already playing. Play will continue\
 with other operands ignored.\n",
               device + 'A');
    }
    else /* not busy*/
    {
        CD_ioctl_in_ck(device, &vs, sizeof(vs));
        min = vs.volsize / (60 * 75);
        sec = ((float)(vs.volsize - ((long)min * 60 * 75))) / 75.;

        printf("Device %c is size %ld, or %d:%02.2f\n",
               device + 'A',
               vs.volsize,
               min, sec);

        CD_ioctl_in_ck(device, &di, sizeof(di));
        HSldout = CD_HSSect(di.ldouttrk);

        printf("Audio tracks %d to %d.  Lead-Out Track %2d:%02d %ld\n",
               (int)di.lotrak,
               (int)di.hitrak,
               di.ldouttrk[2], di.ldouttrk[1],
               HSldout);

        for (ati.trakno = di.lotrak; ati.trakno <= di.hitrak; ati.trakno++)
        {
            CD_ioctl_in_ck(device, &ati, sizeof(ati));
            printf("  Track %d  Address %2d:%02d %ld\n",
                   (int)ati.trakno,
                   ati.trakstrt[2], ati.trakstrt[1],
                   CD_HSSect(ati.trakstrt));
        }

        if (argc < 3)
            ati.trakno = 1;
        else
            ati.trakno = (char)atoi(argv[2]);

        if (ati.trakno < di.lotrak || ati.trakno > di.hitrak)
        {
            printf("Specified track %s is bad conversion or outside range.\n",
                   argv[2]);
            exit(8);
        }

        CD_ioctl_in_ck(device, &ati, sizeof(ati));
        Strtsect = CD_HSSect(ati.trakstrt);

        if (ati.trakno < di.hitrak && argc > 2 && argv[2][0] != '+')
        {
            ati.trakno++;
            CD_ioctl_in_ck(device, &ati, sizeof(ati));
            Nsect = CD_HSSect(ati.trakstrt) - Strtsect;
        }
        else
            Nsect = HSldout - Strtsect;

        AI.z[0].vc = 255;
        AI.z[1].vc = 255;
        AI.z[0].ic = 0;
        AI.z[1].ic = 1;
        printf("Audio Channel default: Normal Stereo\n");


        CD_ioctl_out(device, &AI, sizeof(AI));

        CD_seek(device, Strtsect);

        printf("Play from Sector %ld for %ld Sectors\n", Strtsect, Nsect);
        CD_play(device, Strtsect, Nsect);
    }

    {
        int n;
        for (n = 1; n < 12; n++)
            printf("\n");
    }
    
    printf("\nPress Esc to stop playing and exit, Space to keep playing and exit.\n");

    /*while (1)
    {
        char key = 0;
        if (bioskey(1))
        {
            key = bioskey(0);
            if (key == 0x1b)
                Stop_Audio(device);
            if (key == ' ')
            {
                {
                    int n;
                    for (n = 1; n < 15; n++)
                        printf("\n");
                }
                exit(0);
            }
        }
        status = ioctl_in(device, &Qi, sizeof(Qi));
        gotoxy(1, 14);
        printf("Track %2d: %02d:%02d:%02d  Disk: %02d:%02d:%02d\n\n",
               (int)(Qi.tno & 0x0f) + 10 * ((Qi.tno & 0xf0) >> 4),
               (int)Qi.min, (int)Qi.sec, (int)Qi.frame,
               (int)Qi.amin, (int)Qi.asec, (int)Qi.aframe);

        busy = status & 0x0200;
        printf("Status: %s", busy ? "Busy" : "Idle");
        if (!busy)
            break;
    }*/

}

void CD_ioctl_in_ck(int device, void far *tranad, int tranct)
{
    int status;
    status = CD_ioctl_in(device, tranad, tranct);
    if (status & rherr)
        printf("IOCTL_IN returned error %x\n", status & rherm);
    if (status & rhbusy)
        printf("IOCTL_IN returned busy\n");
}

int CD_ioctl_in(int device, void far *tranad, int tranct)
{
    struct IOCTLI ioc;

    ioc.mdb = 0;
    ioc.ssn = 0;
    ioc.volid = NULL;
    ioc.rh.rhlen = 13;
    ioc.rh.comc = 3;
    ioc.rh.status = 0;
    ioc.tranad = tranad;
    ioc.tranct = tranct;
    CD_cdrreq(device, &ioc);
    return (ioc.rh.status);
}

int CD_ioctl_out(int device, void far *tranad, int tranct)
{
    struct IOCTLO ioc;

    ioc.mdb = 0;
    ioc.ssn = 0;
    ioc.volid = NULL;
    ioc.rh.rhlen = 13;
    ioc.rh.comc = 12;
    ioc.rh.status = 0;
    ioc.tranad = tranad;
    ioc.tranct = tranct;
    CD_cdrreq(device, &ioc);
    if (ioc.rh.status & rherr)
        printf("IOCTL_OUT returned error %x\n", ioc.rh.status & rherm);
    if (ioc.rh.status & rhbusy)
        printf("IOCTL_OUT returned busy\n");
    return (ioc.rh.status);
}

void CD_play(int device, long Strsect, long Nsect)
{
    struct PlayReq prq;
    prq.am = 0; /*High Sierra Addressing*/
    prq.Strsect = Strsect;
    prq.Nsect = Nsect;
    prq.rh.rhlen = 13;
    prq.rh.comc = 132; /*Play*/
    prq.rh.status = 0;
    CD_cdrreq(device, &prq);
    if (prq.rh.status & rherr)
        printf("PLAY returned error %x\n", prq.rh.status & rherm);
    if (prq.rh.status & rhbusy)
        printf("PLAY returned busy\n");
}

void CD_Stop_Audio(int device)
{
    CD_cdrreq(device, &StopPlayReq);
}

void CD_seek(int device, long track)
{
    struct SeekReq skr;
    skr.am = 0; /*High Sierra Addressing*/
    skr.ta = 0;
    skr.str = 0;
    skr.ssn = track;
    skr.rh.rhlen = 13;
    skr.rh.comc = 131; /*seek*/
    skr.rh.status = 0;
    CD_cdrreq(device, &skr);
    if (skr.rh.status & rherr)
        printf("SEEK returned error %x\n", skr.rh.status & rherm);
    if (skr.rh.status & rhbusy)
        printf("SEEK returned busy\n");
}

/*CDROM Device Driver Direct Request*/
void CD_cdrreq(int device, void *cdrrh)
{
    union REGS inr, outr;
    struct SREGS seg;
    seg.es = FP_SEG(cdrrh);
    inr.w.bx = FP_OFF(cdrrh);
    inr.w.cx = device;
    inr.w.ax = 0x1510;
    int386x(0x2f, &inr, &outr, &seg);
}
/*CDROM Drive Check*/
int CD_cdrck(int device)
{
    union REGS inr, outr;
    inr.w.bx = 0;
    inr.w.cx = device;
    inr.w.ax = 0x150b;
    int386(0x2f, &inr, &outr);
    if (outr.w.bx == 0xadad)
        return (outr.w.ax);
    else
        return (0);
}

long CD_HSSect(unsigned char RB[4])
{
    long sector;
    int min, sec, frame;
    frame = RB[0]; /*little-Endian*/
    sec = RB[1];
    min = RB[2];
    sector = ((long)min * 60 + (long)sec) * 75 + frame - 150;
    return (sector);
}
